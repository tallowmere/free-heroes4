#include "h4/platform/sdl3/sdl_web_sign_in.hpp"

#import <WebKit/WebKit.h>
#include <TargetConditionals.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>

#include "h4/platform/sdl3/sdl_uikit.hpp"
#else
#import <AppKit/AppKit.h>
#endif

#include <objc/runtime.h>

#include <functional>
#include <string>
#include <utility>

namespace {

constexpr double kWidth  = 480;
constexpr double kHeight = 720;

const char kWatcherKey = 0;

}

@interface H4SignInNavigator : NSObject <WKNavigationDelegate, WKUIDelegate>
- (instancetype)initWithPrefix:(NSString*)prefix
                         close:(std::function<void()>)close
                        finish:(std::function<void(std::string)>)finish;
- (void)open;
- (void)end:(std::string)landing;
@end

@implementation H4SignInNavigator {
    NSString*                        prefix_;
    std::function<void()>            close_;
    std::function<void(std::string)> finish_;
    H4SignInNavigator*               alive_;
    BOOL                             ended_;
}

- (instancetype)initWithPrefix:(NSString*)prefix
                         close:(std::function<void()>)close
                        finish:(std::function<void(std::string)>)finish {
    if((self = [super init])) {
        prefix_ = prefix;
        close_  = std::move(close);
        finish_ = std::move(finish);
    }
    return self;
}

- (void)open {
    if(!ended_) alive_ = self;
}

- (void)end:(std::string)landing {
    if(ended_) return;
    ended_ = YES;
    finish_(std::move(landing));
    close_();
    alive_ = nil;
}

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                    decisionHandler:(void (^)(WKNavigationActionPolicy))decide {
    NSString* url = action.request.URL.absoluteString;
    if(url != nil && [url hasPrefix:prefix_]) {
        decide(WKNavigationActionPolicyCancel);
        [self end:std::string{ url.UTF8String }];
        return;
    }
    decide(WKNavigationActionPolicyAllow);
}

- (WKWebView*)webView:(WKWebView*)webView
    createWebViewWithConfiguration:(WKWebViewConfiguration*)configuration
               forNavigationAction:(WKNavigationAction*)action
                    windowFeatures:(WKWindowFeatures*)features {
    [webView loadRequest:action.request];
    return nil;
}

- (void)webViewDidClose:(WKWebView*)webView {
    [self cancelSignIn];
}

- (void)cancelSignIn {
    [self end:std::string{}];
}

@end

#if TARGET_OS_IPHONE

@interface H4SheetWatcher : NSObject <UIAdaptivePresentationControllerDelegate>
@property(nonatomic, weak) H4SignInNavigator* navigator;
@end

@implementation H4SheetWatcher
- (void)presentationControllerDidDismiss:(UIPresentationController*)controller {
    [self.navigator cancelSignIn];
}
@end

#else

@interface H4WindowWatcher : NSObject <NSWindowDelegate>
@property(nonatomic, weak) H4SignInNavigator* navigator;
@end

@implementation H4WindowWatcher
- (void)windowWillClose:(NSNotification*)notification {
    [self.navigator cancelSignIn];
}
@end

#endif

namespace h4::platform::sdl3 {

namespace {

[[nodiscard]] WKWebView* make_web_view(H4SignInNavigator* navigator, NSString* url) {
    WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
    config.websiteDataStore        = WKWebsiteDataStore.nonPersistentDataStore;
    WKWebView* view = [[WKWebView alloc] initWithFrame:CGRectMake(0, 0, kWidth, kHeight)
                                         configuration:config];
    view.navigationDelegate = navigator;
    view.UIDelegate         = navigator;
    [view loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:url]]];
    return view;
}

}

bool WebSignIn::offered() noexcept {
    return true;
}

void WebSignIn::open(SDL_Window* window, std::string_view url, std::string_view landing_prefix) {
    const std::string url_z{ url };
    const std::string prefix_z{ landing_prefix };
    NSString*         page   = [NSString stringWithUTF8String:url_z.c_str()];
    NSString*         prefix = [NSString stringWithUTF8String:prefix_z.c_str()];
    auto              finish = [inbox = inbox_](std::string landing) {
        inbox->push(std::move(landing));
    };
    if(page == nil || [NSURL URLWithString:page] == nil) {
        finish({});
        return;
    }

#if TARGET_OS_IPHONE
    UIViewController* over = top_view_controller(window);
    if(over == nil) {
        finish({});
        return;
    }
    UIViewController*         content      = [[UIViewController alloc] init];
    __weak UIViewController*  weak_content = content;
    H4SheetWatcher*           watcher      = [[H4SheetWatcher alloc] init];
    H4SignInNavigator*        navigator    = [[H4SignInNavigator alloc]
        initWithPrefix:prefix
                 close:[weak_content] {
                     [weak_content dismissViewControllerAnimated:YES completion:nil];
                 }
                finish:finish];
    watcher.navigator = navigator;
    objc_setAssociatedObject(navigator, &kWatcherKey, watcher, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

    content.view  = make_web_view(navigator, page);
    content.title = @"Sign in with GOG";
    content.navigationItem.leftBarButtonItem =
        [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                      target:navigator
                                                      action:@selector(cancelSignIn)];
    UINavigationController* sheet =
        [[UINavigationController alloc] initWithRootViewController:content];
    sheet.modalPresentationStyle          = UIModalPresentationFormSheet;
    sheet.presentationController.delegate = watcher;
    [navigator open];
    [over presentViewController:sheet animated:YES completion:nil];
#else
    (void)window;
    NSWindow* panel = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, kWidth, kHeight)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                            NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    panel.releasedWhenClosed = NO;
    panel.title              = @"Sign in with GOG";
    H4WindowWatcher*   watcher   = [[H4WindowWatcher alloc] init];
    H4SignInNavigator* navigator = [[H4SignInNavigator alloc]
        initWithPrefix:prefix
                 close:[panel] {
                     panel.delegate = nil;
                     [panel orderOut:nil];
                 }
                finish:finish];
    watcher.navigator = navigator;
    objc_setAssociatedObject(navigator, &kWatcherKey, watcher, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    panel.delegate    = watcher;
    panel.contentView = make_web_view(navigator, page);
    [navigator open];
    [panel center];
    [panel makeKeyAndOrderFront:nil];
#endif
}

}
