#include "spargel/runtime/ui/window_appkit.h"
#include "spargel/runtime/logging.h"

@implementation SpargelApplicationDelegate
- (bool)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return true;
}
@end

@implementation SpargelMainView {
    CADisplayLink* display_link_;
    CAMetalLayer* metal_layer_;
    NSTrackingArea* tracking_area_;
    spargel::runtime::ui::WindowAppKit* bridge_;
}

- (instancetype)initWithBridge:(spargel::runtime::ui::WindowAppKit*)bridge {
    self = [super init];
    if (self) {
        bridge_ = bridge;
        metal_layer_ = bridge_->metal_layer();
        self.layer = metal_layer_;
        self.wantsLayer = true;

        [self recreateTrackingArea];
    }
    return self;
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

- (void)render:(CADisplayLink*)sender {
    // The Application Kit creates an autorelease pool on the main thread at the
    // beginning of every cycle of the event loop, and drains it at the end,
    // thereby releasing any autoreleased objects generated while processing an
    // event.
    //
    if (bridge_->delegate()) {
        bridge_->delegate()->on_render();
    }
}

- (void)setupDisplayLink:(NSWindow*)window {
    [display_link_ invalidate];
    display_link_ = [window displayLinkWithTarget:self
                                         selector:@selector(render:)];
    display_link_.paused = false;
}
- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    // move off a window
    if (self.window == nullptr) {
        [display_link_ invalidate];
        display_link_ = nullptr;
        return;
    }
    [self setupDisplayLink:self.window];
    [display_link_ addToRunLoop:[NSRunLoop currentRunLoop]
                        forMode:NSRunLoopCommonModes];
    [self resizeDrawable:self.window.screen.backingScaleFactor];
}

- (void)resizeDrawable:(CGFloat)scaleFactor {
    CGSize newSize = self.bounds.size;
    newSize.width *= scaleFactor;
    newSize.height *= scaleFactor;

    if (newSize.width <= 0 || newSize.width <= 0) {
        return;
    }
    if (newSize.width == metal_layer_.drawableSize.width &&
        newSize.height == metal_layer_.drawableSize.height) {
        return;
    }
    metal_layer_.drawableSize = newSize;
}
- (void)viewDidChangeBackingProperties {
    [super viewDidChangeBackingProperties];
    [self resizeDrawable:self.window.screen.backingScaleFactor];
}
- (void)setFrameSize:(NSSize)size {
    [super setFrameSize:size];
    [self resizeDrawable:self.window.screen.backingScaleFactor];
}
- (void)setBoundsSize:(NSSize)size {
    [super setBoundsSize:size];
    [self resizeDrawable:self.window.screen.backingScaleFactor];
}

@end

namespace spargel::runtime::ui {
    void WindowAppKit::set_title(char const* title) {
        ns_window_.title = [NSString stringWithUTF8String:title];
    }

    void WindowAppKit::init_app() {
        ns_app_ = [NSApplication sharedApplication];
        nsapp_delegate_ = [[SpargelApplicationDelegate alloc] init];
        ns_app_.delegate = nsapp_delegate_;
        [ns_app_ setActivationPolicy:NSApplicationActivationPolicyRegular];
    }

    void WindowAppKit::init_window() {
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

        // create metal layer before creating the main view
        metal_layer_ = [[CAMetalLayer alloc] init];
        main_view_ = [[SpargelMainView alloc] initWithBridge:this];
        ns_window_.contentView = main_view_;
    }

    Window* Window::create() { return new WindowAppKit; }
}  // namespace spargel::runtime::ui
