#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#include "spargel/runtime/ui/window.h"

@interface SpargelApplicationDelegate : NSObject <NSApplicationDelegate>
@end

@implementation SpargelApplicationDelegate
- (bool)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return true;
}
@end

@interface SpargelWindowDelegate : NSObject <NSWindowDelegate>
@end

// TODO(tianjiao): implement <NSTextInputClient>
@interface SpargelMainView : NSView
@end

@implementation SpargelMainView {
    CADisplayLink* display_link_;
    CAMetalLayer* metal_layer_;
    NSTrackingArea* tracking_area_;
}
- (void)recreateTrackingArea {
    if (tracking_area_) {
        [self removeTrackingArea:tracking_area_];
    }

    NSTrackingAreaOptions options =
        NSTrackingActiveAlways | NSTrackingInVisibleRect |
        NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved;
    tracking_area_ = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                  options:options
                                                    owner:self
                                                 userInfo:nil];
    [self addTrackingArea:tracking_area_];
}
@end

namespace spargel::runtime::ui {
    class WindowAppKit final : public Window {
    public:
        WindowAppKit() {
            init_app();
            init_window();
        }

        void set_title(char const* title) override {
            ns_window_.title = [NSString stringWithUTF8String:title];
        }
        void show() override { [NSApp run]; }

    private:
        void init_app() {
            ns_app_ = [NSApplication sharedApplication];
            nsapp_delegate_ = [[SpargelApplicationDelegate alloc] init];
            ns_app_.delegate = nsapp_delegate_;
            ns_app_.activationPolicy = NSApplicationActivationPolicyRegular;
        }

        void init_window() {
            auto width = 500.0;
            auto height = 500.0;

            auto screen = [NSScreen mainScreen];

            NSWindowStyleMask style =
                NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                NSWindowStyleMaskResizable | NSWindowStyleMaskTitled;

            auto rect = NSMakeRect(0, 0, width, height);
            // center the window
            rect.origin.x = (screen.frame.size.width - width) / 2;
            rect.origin.y = (screen.frame.size.height - height) / 2;

            ns_window_ =
                [[NSWindow alloc] initWithContentRect:rect
                                            styleMask:style
                                              backing:NSBackingStoreBuffered
                                                defer:false
                                               screen:screen];
            [ns_window_ makeKeyAndOrderFront:nullptr];
        }

        NSApplication* ns_app_ = nullptr;
        SpargelApplicationDelegate* nsapp_delegate_ = nullptr;
        NSWindow* ns_window_ = nullptr;
    };

    Window* Window::create() { return new WindowAppKit; }
}  // namespace spargel::runtime::ui
