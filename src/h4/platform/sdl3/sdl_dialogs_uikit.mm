#include "h4/platform/sdl3/sdl_dialogs.hpp"
#include "h4/platform/sdl3/sdl_uikit.hpp"

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <objc/runtime.h>

#include <functional>
#include <string>
#include <utility>

namespace {

const char kDelegateKey = 0;

}

@interface H4FolderPickerDelegate : NSObject <UIDocumentPickerDelegate>
- (instancetype)initWithDeliver:(std::function<void(NSURL*)>)deliver;
@end

@implementation H4FolderPickerDelegate {
    std::function<void(NSURL*)> deliver_;
}

- (instancetype)initWithDeliver:(std::function<void(NSURL*)>)deliver {
    if((self = [super init])) deliver_ = std::move(deliver);
    return self;
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller
    didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    for(NSURL* url in urls) deliver_(url);
}

@end

namespace h4::platform::sdl3 {

void FileDialogs::open(SDL_Window* window, std::string_view ,
                       std::string_view ) {
    UIViewController* over = top_view_controller(window);
    if(over == nil) return;

    UIDocumentPickerViewController* picker =
        [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeFolder ]];
    picker.directoryURL = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
                                                               inDomains:NSUserDomainMask]
                              .firstObject;
    H4FolderPickerDelegate* delegate =
        [[H4FolderPickerDelegate alloc] initWithDeliver:[inbox = inbox_](NSURL* url) {
            if([url startAccessingSecurityScopedResource])
                inbox->keep({ CFBridgingRetain(url), [](const void* held) { CFRelease(held); } });
            inbox->push(std::string{ url.fileSystemRepresentation });
        }];
    picker.delegate = delegate;
    objc_setAssociatedObject(picker, &kDelegateKey, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [over presentViewController:picker animated:YES completion:nil];
}

}
