#include "spargel/ui/window_mac.h"

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#import <MetalKit/MetalKit.h>

#include "spargel/base/logging.h"
#include "spargel/ui/canvas.h"

// objc ------------------------------------------------------------------------

@implementation SpargelApplication
- (bool)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return true;
}
- (void)checkForUpdates:(id)sender {
  LOG_ERROR("check for updates: not implemented");
}
@end

// there are several modes for the view system
// 1. everything is painted using system canvas
// 2. everything is painted using spargel's gpu-based vector renderer
// 3.
@implementation SpargelRootView {
  spargel::ui::WindowMac* spargel_window_;
  MTKView* metal_view_;
}
- (nonnull instancetype)initWithSpargel:
    (spargel::ui::WindowMac*)spargel_window {
  self = [super init];
  if (self) {
    spargel_window_ = spargel_window;
    metal_view_ = [[MTKView alloc] init];
    [self addSubview:metal_view_];
    self.wantsLayer = true;
  }
  return self;
}
- (void)drawRect:(NSRect)dirtyRect {
  [@"Platform Canvas" drawAtPoint:CGPointMake(100, 100)
                   withAttributes:@{
                     NSForegroundColorAttributeName : NSColor.redColor,
                     NSFontAttributeName : [NSFont systemFontOfSize:20],
                   }];
  auto ctx = NSGraphicsContext.currentContext.CGContext;
  spargel_window_->paint(ctx);
}
@end

// -----------------------------------------------------------------------------

namespace spargel::ui {
namespace {
class CanvasCG final : public PlatformCanvas {
public:
  CanvasCG(CGContextRef ctx) : ctx_{ctx} {
    CFRetain(ctx_);
    font_ = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 0.0, nullptr);
  }
  ~CanvasCG() {
    CFRelease(font_);
    CFRelease(ctx_);
  }
  void draw_text(char const* text, float x, float y) override {
    CFStringRef string =
        CFStringCreateWithCString(nullptr, text, kCFStringEncodingASCII);
    void const* keys[] = {kCTFontAttributeName,
                          kCTForegroundColorAttributeName};
    void const* values[] = {font_, CGColorGetConstantColor(kCGColorWhite)};
    CFDictionaryRef attrs =
        CFDictionaryCreate(nullptr, keys, values, 2, nullptr, nullptr);
    CFAttributedStringRef attr_string =
        CFAttributedStringCreate(nullptr, string, attrs);
    CTLineRef line = CTLineCreateWithAttributedString(attr_string);
    CGContextSetTextPosition(ctx_, x, y);
    CTLineDraw(line, ctx_);
    CFRelease(line);
    CFRelease(attr_string);
    CFRelease(attrs);
    CFRelease(string);
  }

private:
  CGContextRef ctx_ = nullptr;
  CTFontRef font_ = nullptr;
};
}  // namespace

WindowMac::WindowMac() {
  NSWindowStyleMask style =
      NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
      NSWindowStyleMaskResizable | NSWindowStyleMaskTitled;
  auto screen = [NSScreen mainScreen];
  const auto width = 500.0;
  const auto height = 500.0;
  auto rect = NSMakeRect(0, 0, width, height);
  // center the window
  rect.origin.x = (screen.frame.size.width - width) / 2;
  rect.origin.y = (screen.frame.size.height - height) / 2;
  window_ = [[NSWindow alloc] initWithContentRect:rect
                                        styleMask:style
                                          backing:NSBackingStoreBuffered
                                            defer:false
                                           screen:screen];
  window_.contentView = [[SpargelRootView alloc] initWithSpargel:this];
  [window_ makeKeyAndOrderFront:nullptr];
}

WindowMac::~WindowMac() { window_ = nullptr; }

void WindowMac::set_title(char const* title) {
  window_.title = [NSString stringWithUTF8String:title];
}

void WindowMac::paint(CGContextRef ctx) {
  CanvasCG canvas{ctx};
  if (delegate()) {
    delegate()->paint(&canvas);
  }
}

UIManagerMac::UIManagerMac() {
  app_ = NSApplication.sharedApplication;
  app_.activationPolicy = NSApplicationActivationPolicyRegular;
  app_delegate_ = [[SpargelApplication alloc] init];
  app_.delegate = app_delegate_;
  init_menu();
}

UIManagerMac::~UIManagerMac() {
  app_delegate_ = nullptr;
  app_ = nullptr;
}

Window* UIManagerMac::create_window() { return new WindowMac; }

void UIManagerMac::start() { [app_ run]; }

void UIManagerMac::init_menu() {
  NSMenu* menu_bar = [[NSMenu alloc] init];

  NSMenu* app_menu = [[NSMenu alloc] init];
  [app_menu addItemWithTitle:@"About Spargel"
                      action:@selector(orderFrontStandardAboutPanel:)
               keyEquivalent:@""];
  NSMenuItem* check_for_updates =
      [[NSMenuItem alloc] initWithTitle:@"Check for Updates"
                                 action:@selector(checkForUpdates:)
                          keyEquivalent:@""];
  // TODO(tianjiao): extract to a helper function
  check_for_updates.image =
      [NSImage imageWithSystemSymbolName:@"square.and.arrow.down"
                accessibilityDescription:check_for_updates.title];
  [app_menu addItem:check_for_updates];
  [app_menu addItem:[NSMenuItem separatorItem]];
  [app_menu addItemWithTitle:@"Quit Spargel"
                      action:@selector(terminate:)
               keyEquivalent:@"q"];
  NSMenuItem* app_menu_item = [[NSMenuItem alloc] init];
  [app_menu_item setSubmenu:app_menu];
  [menu_bar addItem:app_menu_item];

  NSMenu* window_menu = [[NSMenu alloc] initWithTitle:@"Window"];
  NSMenuItem* window_menu_item = [[NSMenuItem alloc] init];
  [window_menu_item setSubmenu:window_menu];
  [menu_bar addItem:window_menu_item];

  app_.mainMenu = menu_bar;
  app_.windowsMenu = window_menu;
}

UIManager* UIManager::create() { return new UIManagerMac; }

}  // namespace spargel::ui
