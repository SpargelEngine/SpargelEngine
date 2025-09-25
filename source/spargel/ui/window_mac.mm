#include "spargel/ui/window_mac.h"
#include "spargel/base/allocator.h"
#include "spargel/base/check.h"
#include "spargel/base/inline_array.h"
#include "spargel/base/meta.h"
#include "spargel/base/unique_ptr.h"
#include "spargel/base/vector.h"
#include "spargel/logging/logging.h"
#include "spargel/render/ui_renderer_metal.h"

//
#include <time.h>

//
#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#import <GameController/GameController.h>
#import <QuartzCore/QuartzCore.h>

// TODO:
// - The NSWindow is owned by WindowAppKit.
//   When the NSWindow is closed, WindowAppKit needs to release it.
//   Thus WindowAppKit should have a closed state.
//   In this case the window delegate should be notified and it should delete
//   the WindowAppKit.
// - Make WindowAppKit lightweight, i.e. implement main logic in objc.
//   Reason: It's easier to deal with Cocoa objects in objc, for example calling
//   super methods.
//

@implementation SpargelHostView {
    CADisplayLink* _link;
    CAMetalLayer* _layer;
    NSTrackingArea* _tracking;
    spargel::ui::WindowAppKit* _bridge;
}
- (instancetype)initWithBridge:(spargel::ui::WindowAppKit*)bridge
                    metalLayer:(CAMetalLayer*)layer {
    self = [super init];
    if (self) {
        _bridge = bridge;
        _layer = layer;
        _tracking = nil;
        self.layer = _layer;
        self.wantsLayer = YES;  // layer-hosting view

        [self recreateTrackingArea];

        // TODO: Remove.
        [[NSNotificationCenter defaultCenter]
            addObserver:self
               selector:@selector(keyboardWasConnected:)
                   name:GCKeyboardDidConnectNotification
                 object:nil];
    }
    return self;
}
// TODO: Remove.
- (void)keyboardWasConnected:(NSNotification*)notification {
    auto keyboard = (GCKeyboard*)notification.object;
    spargel_check(keyboard);

    auto keyboard_input = [keyboard keyboardInput];
    spargel_check(keyboard_input);
    auto key = [keyboard_input buttonForKeyCode:GCKeyCodeKeyA];
    spargel_check(key);
    key.pressedChangedHandler =
        ^(GCControllerButtonInput*, float, bool pressed) {
          spargel_log_info("GC Callback Key A: %s",
                           pressed ? "pressed" : "released");
        };
}
- (void)recreateTrackingArea {
    if (_tracking != nil) {
        [self removeTrackingArea:_tracking];
        [_tracking release];
    }

    NSTrackingAreaOptions options =
        NSTrackingActiveAlways | NSTrackingInVisibleRect |
        NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved;
    _tracking = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                             options:options
                                               owner:self
                                            userInfo:nil];
    [self addTrackingArea:_tracking];
}
- (void)mouseMoved:(NSEvent*)event {
    auto loc = [event locationInWindow];
    _bridge->emitMouseMoved(
        {(float)loc.x, (float)loc.y, (float)event.deltaX, (float)event.deltaX});
}
- (void)mouseDragged:(NSEvent*)event {
    _bridge->_bridgeMouseDragged((float)event.deltaX, (float)event.deltaY);
}
- (void)mouseDown:(NSEvent*)event {
    auto loc = [event locationInWindow];
    _bridge->_bridgeMouseDown((float)loc.x, (float)loc.y);
}
- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window == nil) {
        // move off a window
        [_link invalidate];
        _link = nil;
        return;
    }
    [self setupDisplayLink:self.window];
    [_link addToRunLoop:[NSRunLoop currentRunLoop]
                forMode:NSRunLoopCommonModes];
    [self resizeDrawable:self.window.screen.backingScaleFactor];
}
- (void)setupDisplayLink:(NSWindow*)window {
    [_link invalidate];
    _link = [window displayLinkWithTarget:self selector:@selector(render:)];
}
- (void)setAnimating:(bool)animating {
    _link.paused = !animating;
}
- (void)render:(CADisplayLink*)sender {
    // The Application Kit creates an autorelease pool on the main thread at the
    // beginning of every cycle of the event loop, and drains it at the end,
    // thereby releasing any autoreleased objects generated while processing an
    // event.
    //
    _bridge->_bridge_render();
}
- (void)viewDidChangeBackingProperties {
    [super viewDidChangeBackingProperties];
    [self resizeDrawable:self.window.screen.backingScaleFactor];
}
- (void)setFrameSize:(NSSize)size {
    [super setFrameSize:size];
    _bridge->_updateSize((float)size.width, (float)size.height);
    [self resizeDrawable:self.window.screen.backingScaleFactor];
}
- (void)setBoundsSize:(NSSize)size {
    [super setBoundsSize:size];
    _bridge->_updateSize((float)size.width, (float)size.height);
    [self resizeDrawable:self.window.screen.backingScaleFactor];
}
- (void)resizeDrawable:(CGFloat)scaleFactor {
    CGSize newSize = self.bounds.size;
    newSize.width *= scaleFactor;
    newSize.height *= scaleFactor;

    if (newSize.width <= 0 || newSize.width <= 0) {
        return;
    }
    if (newSize.width == _layer.drawableSize.width &&
        newSize.height == _layer.drawableSize.height) {
        return;
    }
    _layer.drawableSize = newSize;

    // _bridge->_setDrawableSize((float)newSize.width, (float)newSize.height);
}
// Note: Please read Chromium's docs on integrating with Cocoa's input
// management system.
// https://chromium.googlesource.com/chromium/src/+/main/docs/ui/input_event/index.md#MacViews
// https://chromium.googlesource.com/chromium/src/+/main/components/remote_cocoa/app_shim/bridged_content_view.mm
- (void)keyDown:(NSEvent*)event {
    [self interpretKeyEvents:@[ event ]];
    auto code = [event keyCode];
    _bridge->_bridge_key_down(code, event.type);
}
- (BOOL)acceptsFirstResponder {
    return YES;
}

// TODO: Return nil only if the views system doesn't have an input client.
// Otherwise return [super inputContext];
- (NSTextInputContext*)inputContext {
    if (!_bridge->text_focus()) {
        return nullptr;
    }
    return [super inputContext];
}

// protocol NSTextInputClient

- (BOOL)hasMarkedText {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return false;
    }
    return delegate->hasMarkedText();
}
- (NSRange)markedRange {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return NSMakeRange(NSNotFound, 0);
    }
    return delegate->getMarkedRange();
}
- (NSRange)selectedRange {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return NSMakeRange(NSNotFound, 0);
    }
    return delegate->getSelectedRange();
}
- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return;
    }
    delegate->setMarkedText(string, selectedRange, replacementRange);
}
- (void)unmarkText {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return;
    }
    delegate->unmarkText();
}
- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return nil;
    }
    return delegate->validAttributesForMarkedText();
}
- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:
                                                   (NSRangePointer)actualRange {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return nil;
    }
    return delegate->attributedSubstringForProposedRange(range, actualRange);
}
- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return;
    }
    delegate->insertText(string, replacementRange);
}
- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return NSNotFound;
    }
    return delegate->characterIndexForPoint(point);
}
- (NSRect)firstRectForCharacterRange:(NSRange)range
                         actualRange:(NSRangePointer)actualRange {
    auto delegate = _bridge->getTextDelegate();
    if (!delegate) {
        return NSMakeRect(0, 0, 0, 0);
    }
    return delegate->firstRectForCharacterRange(range, actualRange);
}

// NSResponder
- (void)magnifyWithEvent:(NSEvent*)event {
    auto m = [event magnification];
    spargel_log_info("mag: %.3f", m);
}

@end

@implementation SpargelWindowDelegate {
    spargel::ui::WindowAppKit* bridge_;
}
- (instancetype)initWithBridge:(spargel::ui::WindowAppKit*)bridge {
    self = [super init];
    if (self) {
        spargel_check(bridge);
        bridge_ = bridge;
        // If we cache NSWindow in the constructor, then the delegate need to be
        // created after the window. window_ = bridge_->getNSWindow();
    }
    return self;
}
- (void)windowDidChangeOcclusionState:(NSNotification*)notification {
    auto window = static_cast<NSWindow*>(notification.object);
    auto state = [window occlusionState];
    if (state & NSWindowOcclusionStateVisible) {
        spargel_log_info("window becomes visible");
    } else {
        spargel_log_info("window becomes occluded");
    }
}
@end

namespace spargel::ui {
    namespace {
        PhysicalKey translatePhysicalKey(int code) {
            switch (code) {
#define PHYSICAL_KEY_MAP(mapped, original) \
    case original:                         \
        return PhysicalKey::mapped;
#include "spargel/ui/physical_key_mapping_mac.inc"
            default:
                return PhysicalKey::unknown;
            }
        }
    }  // namespace

    WindowAppKit::WindowAppKit(u32 width, u32 height)
        : width_{static_cast<float>(width)},
          height_{static_cast<float>(height)} {
        auto screen = [NSScreen mainScreen];

        NSWindowStyleMask style =
            NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
            NSWindowStyleMaskResizable | NSWindowStyleMaskTitled;

        auto rect = NSMakeRect(0, 0, width, height);
        // center the window
        rect.origin.x = (screen.frame.size.width - width_) / 2;
        rect.origin.y = (screen.frame.size.height - height_) / 2;

        ns_window_ =
            [[NSWindow alloc] initWithContentRect:rect
                                        styleMask:style
                                          backing:NSBackingStoreBuffered
                                            defer:NO
                                           screen:screen];

        // Create the delegate after the window, as the delegate stores a
        // pointer to the window.
        ns_window_delegate_ =
            [[SpargelWindowDelegate alloc] initWithBridge:this];
        // This is a weak reference.
        ns_window_.delegate = ns_window_delegate_;

        // The window is owned by the C++ side.
        ns_window_.releasedWhenClosed = NO;
        // TODO: fix this
        ns_window_.minSize = NSMakeSize(200, 200);

        metal_layer_ = [[CAMetalLayer alloc] init];

        ns_view_ = [[SpargelHostView alloc] initWithBridge:this
                                                metalLayer:metal_layer_];

        auto _text_cursor = [[NSTextInsertionIndicator alloc]
            initWithFrame:NSMakeRect(0, 0, 0, 0)];
        [ns_view_ addSubview:_text_cursor];

        _text_cursor.displayMode = NSTextInsertionIndicatorDisplayModeAutomatic;

        // strong reference
        ns_window_.contentView = ns_view_;

        [ns_window_ makeKeyAndOrderFront:nil];
    }

    void WindowAppKit::_enable_text_cursor() {
        // _text_cursor.displayMode =
        // NSTextInsertionIndicatorDisplayModeAutomatic;
    }

    WindowAppKit::~WindowAppKit() {
        [ns_view_ release];
        [ns_window_ release];
        [ns_window_delegate_ release];
    }

    void WindowAppKit::setTitle(char const* title) {
        ns_window_.title = [NSString stringWithUTF8String:title];
    }

    void WindowAppKit::setAnimating(bool animating) {
        if (animating_ != animating) {
            [ns_view_ setAnimating:animating];
        }
        animating_ = animating;
    }

    void WindowAppKit::requestRedraw() { _bridge_render(); }

    void WindowAppKit::_setDrawableSize(float width, float height) {
        drawable_width_ = width;
        drawable_height_ = height;
    }

    WindowHandle WindowAppKit::getHandle() {
        WindowHandle handle;
        handle.value.apple.layer = metal_layer_;
        return handle;
    }

    namespace {
        struct FrameTimeReporter {
            static constexpr usize N = 120;
            base::InlineArray<u64, N> data;
            usize index = 0;
            u64 last_report = 0;
            u64 start = 0;
            u64 getTimestamp() {
                return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
            }
            void beginFrame() { start = getTimestamp(); }
            void endFrame() {
                auto now = getTimestamp();
                data[index] = now - start;
                index = (index + 1) % N;
                // Report every one second.
                if (now - last_report > 1'000'000'000) {
                    report();
                    last_report = now;
                }
            }
            void report() {
                u64 sum = 0;
                for (auto d : data) {
                    sum += d;
                }
                spargel_log_info("wall frame time: %.3fms",
                                 (double)sum / (1e6 * N));
            }
        };
    }  // namespace

    namespace {
        inline constexpr bool REPORT_FRAME_TIME = false;
    }

    void WindowAppKit::_bridge_render() {
        static FrameTimeReporter reporter;
        if (auto d = delegate()) {
            if constexpr (REPORT_FRAME_TIME) {
                reporter.beginFrame();
            }
            d->onRender();
            if constexpr (REPORT_FRAME_TIME) {
                reporter.endFrame();
            }
        }
    }

    void WindowAppKit::_bridge_key_down(int key, NSEventType type) {
        KeyboardEvent e;
        e.key = translatePhysicalKey(key);
        if (type == NSEventTypeKeyDown) {
            e.action = KeyboardAction::press;
        } else if (type == NSEventTypeKeyUp) {
            e.action = KeyboardAction::release;
        } else {
            spargel_panic_here();
        }
        if (auto d = delegate()) {
            d->onKeyboard(e);
        }
    }

    // Coordinate of AppKit:
    //
    //   ^ y
    //   |
    //   |
    //   +----> x
    //
    void WindowAppKit::_bridgeMouseDown(float x, float y) {
        if (auto d = delegate()) {
            d->onMouseDown(MouseDownEvent{x, height_ - y});
        }
    }

    void WindowAppKit::_bridgeMouseDragged(float dx, float dy) {
        if (getDelegate() != nullptr) {
            getDelegate()->onMouseDragged(dx, dy);
        }
    }

    void WindowAppKit::bindRenderer(render::UIRenderer* renderer) {
        auto metal_renderer = static_cast<render::UIRendererMetal*>(renderer);
        metal_renderer->setLayer(metal_layer_);
        renderer->setScaleFactor(scaleFactor());
    }

    float WindowAppKit::scaleFactor() {
        return static_cast<float>([ns_window_ backingScaleFactor]);
    }

    float WindowAppKit::width() {
        return (float)ns_window_.contentView.frame.size.width;
    }
    float WindowAppKit::height() {
        return (float)ns_window_.contentView.frame.size.height;
    }

    void WindowAppKit::emitMouseMoved(const MouseMovedEvent& e) {
        if (delegate()) {
            delegate()->onMouseMoved(e);
        }
    }
}  // namespace spargel::ui
