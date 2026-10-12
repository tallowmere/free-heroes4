#include "h4/net/apple/apple_http.hpp"

#import <Foundation/Foundation.h>

#include <chrono>
#include <cstdio>
#include <format>
#include <string>

namespace {

constexpr NSTimeInterval kTimeout = 60;

[[nodiscard]] NSString* to_ns(const std::string& text) {
    return [NSString stringWithUTF8String:text.c_str()];
}

[[nodiscard]] std::string from_ns(NSString* text) {
    return text == nil ? std::string{} : std::string{ text.UTF8String };
}

[[nodiscard]] NSMutableURLRequest* request_for(const std::string& url, const std::string& bearer) {
    NSURL* target = [NSURL URLWithString:to_ns(url)];
    if(target == nil) return nil;
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:target];
    request.timeoutInterval      = kTimeout;
    if(!bearer.empty())
        [request setValue:to_ns("Bearer " + bearer) forHTTPHeaderField:@"Authorization"];
    return request;
}

[[nodiscard]] NSURLSessionConfiguration* configuration() {
    NSURLSessionConfiguration* config = NSURLSessionConfiguration.ephemeralSessionConfiguration;
    config.timeoutIntervalForRequest  = kTimeout;
    return config;
}

[[nodiscard]] std::unexpected<h4::core::Error> failed(std::string what, NSError* error) {
    return h4::core::fail(h4::core::ErrorCode::io,
                          error == nil ? what
                                       : std::format("{}: {}", what,
                                                     from_ns(error.localizedDescription)));
}

}

@interface H4DownloadDelegate : NSObject <NSURLSessionDataDelegate>
- (instancetype)initWithFile:(std::FILE*)file counter:(std::atomic<std::uint64_t>*)counter;
@property(nonatomic, readonly) dispatch_semaphore_t done;
@property(nonatomic, readonly) NSInteger            status;
@property(nonatomic, readonly) NSError*             error;
@property(nonatomic, readonly) BOOL                 writeFailed;
@end

@implementation H4DownloadDelegate {
    std::FILE*                  file_;
    std::atomic<std::uint64_t>* counter_;
}

- (instancetype)initWithFile:(std::FILE*)file counter:(std::atomic<std::uint64_t>*)counter {
    if((self = [super init])) {
        file_    = file;
        counter_ = counter;
        _done    = dispatch_semaphore_create(0);
    }
    return self;
}

- (void)URLSession:(NSURLSession*)session
              dataTask:(NSURLSessionDataTask*)task
    didReceiveResponse:(NSURLResponse*)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition))completion {
    const NSHTTPURLResponse* http =
        [response isKindOfClass:NSHTTPURLResponse.class]
            ? static_cast<NSHTTPURLResponse*>(response)
            : nil;
    _status = http != nil ? http.statusCode : 0;
    completion(_status == 200 ? NSURLSessionResponseAllow : NSURLSessionResponseCancel);
}

- (void)URLSession:(NSURLSession*)session
          dataTask:(NSURLSessionDataTask*)task
    didReceiveData:(NSData*)data {
    [data enumerateByteRangesUsingBlock:^(const void* bytes, NSRange range, BOOL* stop) {
        if(std::fwrite(bytes, 1, range.length, file_) != range.length) {
            _writeFailed = YES;
            *stop        = YES;
            [task cancel];
            return;
        }
        *counter_ += range.length;
    }];
}

- (void)URLSession:(NSURLSession*)session
                    task:(NSURLSessionTask*)task
    didCompleteWithError:(NSError*)error {
    _error = error;
    dispatch_semaphore_signal(_done);
}

@end

namespace h4::net::apple {

namespace {

class AppleHttpClient final : public HttpClient {
public:
    core::Result<HttpResponse> get(const std::string& url, const std::string& bearer,
                                   std::stop_token stop) override {
        @autoreleasepool {
            NSMutableURLRequest* request = request_for(url, bearer);
            if(request == nil)
                return core::fail(core::ErrorCode::invalid_argument, "not a URL: " + url);
            NSURLSession* session = [NSURLSession sessionWithConfiguration:configuration()];
            dispatch_semaphore_t done   = dispatch_semaphore_create(0);
            __block HttpResponse answer;
            __block NSError*     error  = nil;
            NSURLSessionDataTask* task  = [session
                dataTaskWithRequest:request
                  completionHandler:^(NSData* data, NSURLResponse* response, NSError* failure) {
                      error = failure;
                      if([response isKindOfClass:NSHTTPURLResponse.class])
                          answer.status = static_cast<int>(
                              static_cast<NSHTTPURLResponse*>(response).statusCode);
                      if(data != nil)
                          answer.body.assign(static_cast<const char*>(data.bytes), data.length);
                      dispatch_semaphore_signal(done);
                  }];
            [task resume];
            wait(done, task, stop);
            [session finishTasksAndInvalidate];
            if(stop.stop_requested()) return cancelled();
            if(error != nil) return failed("no answer from " + host_of(url), error);
            return answer;
        }
    }

    core::Result<void> download(const std::string& url, const std::filesystem::path& to,
                                std::atomic<std::uint64_t>& bytes_done,
                                std::stop_token             stop) override {
        @autoreleasepool {
            NSMutableURLRequest* request = request_for(url, "");
            if(request == nil)
                return core::fail(core::ErrorCode::invalid_argument, "not a URL: " + url);
            std::FILE* file = std::fopen(to.c_str(), "wb");
            if(file == nullptr)
                return core::fail(core::ErrorCode::io, "cannot write " + to.string());
            H4DownloadDelegate* delegate =
                [[H4DownloadDelegate alloc] initWithFile:file counter:&bytes_done];
            NSURLSession* session = [NSURLSession sessionWithConfiguration:configuration()
                                                                  delegate:delegate
                                                             delegateQueue:nil];
            NSURLSessionDataTask* task = [session dataTaskWithRequest:request];
            [task resume];
            wait(delegate.done, task, stop);
            [session finishTasksAndInvalidate];
            const bool closed = std::fclose(file) == 0;

            if(stop.stop_requested()) return cancelled();
            if(delegate.writeFailed || !closed)
                return core::fail(core::ErrorCode::io, "cannot write " + to.string());
            if(delegate.status != 200 && delegate.status != 0)
                return core::fail(core::ErrorCode::io,
                                  std::format("{} answered {}", host_of(url), delegate.status));
            if(delegate.error != nil) return failed("download from " + host_of(url), delegate.error);
            return {};
        }
    }

private:
    static void wait(dispatch_semaphore_t done, NSURLSessionTask* task,
                     const std::stop_token& stop) {
        while(dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW,
                                                          100 * NSEC_PER_MSEC)) != 0)
            if(stop.stop_requested()) [task cancel];
    }

    [[nodiscard]] static std::string host_of(const std::string& url) {
        NSURL* parsed = [NSURL URLWithString:to_ns(url)];
        return parsed != nil && parsed.host != nil ? from_ns(parsed.host) : url;
    }
};

}

std::unique_ptr<HttpClient> make_http_client() {
    return std::make_unique<AppleHttpClient>();
}

}
