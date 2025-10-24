#include "spargel/runtime/ui/window_appkit.h"
#include "spargel/runtime/logging.h"

@implementation SpargelApplicationDelegate
- (bool)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return true;
}
@end

@implementation SpargelViewController {
    spargel::runtime::ui::WindowAppKit* spargel_window_;
    MTKView* view_;
}
- (nonnull instancetype)initWithSpargel:
    (spargel::runtime::ui::WindowAppKit*)spargel_window {
    self = [super init];
    if (self) {
        spargel_window_ = spargel_window;
    }
    return self;
}
// override: NSViewController
- (void)loadView {
    view_ = [[MTKView alloc] init];
    view_.delegate = self;
    self.view = view_;
}
// override: NSViewController
- (void)viewDidLoad {
    [super viewDidLoad];
    spargel_window_->set_view(view_);
}
// override: NSViewController
- (void)viewDidAppear {
    [self.view.window makeFirstResponder:self];
}
// override: NSResponder
- (bool)acceptsFirstResponder {
    return true;
}
// override: NSResponder
- (void)keyDown:(NSEvent*)event {
    // TODO(tianjiao): why?
    // [self interpretKeyEvents:@[ event ]];
    auto code = [event keyCode];
    LOG_INFO("key down: %u", code);
}
// override: NSResponder
- (void)mouseDown:(NSEvent*)event {
    auto loc = [event locationInWindow];
    LOG_INFO("mouse down: %.3f %.3f", loc.x, loc.y);
}
// override: MTKViewDelegate
- (void)drawInMTKView:(MTKView*)view {
    auto delegate = spargel_window_->delegate();
    if (delegate) {
        delegate->on_render();
    }
}
// override: MTKViewDelegate
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
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
    NSWindowStyleMask style =
        NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable | NSWindowStyleMaskTitled;
    auto screen = [NSScreen mainScreen];
    ns_window_ = [[NSWindow alloc] initWithContentRect:NSZeroRect
                                             styleMask:style
                                               backing:NSBackingStoreBuffered
                                                 defer:false
                                                screen:screen];

    view_controller_ = [[SpargelViewController alloc] initWithSpargel:this];

    const auto width = 500.0;
    const auto height = 500.0;
    view_controller_.view.frame = NSMakeRect(0, 0, width, height);

    // > Setting contentViewController causes the window to resize based on the
    // > current size of the contentViewController.
    ns_window_.contentViewController = view_controller_;
    [ns_window_ makeKeyAndOrderFront:nullptr];
}

Window* Window::create() { return new WindowAppKit; }
}  // namespace spargel::runtime::ui
