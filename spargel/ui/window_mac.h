#pragma once

#import <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>

#include "spargel/ui/window.h"

// objc ------------------------------------------------------------------------

@interface SpargelApplication : NSObject <NSApplicationDelegate>
@end

@interface SpargelRootView : NSView
@end

// -----------------------------------------------------------------------------

namespace spargel::ui {

class WindowMac final : public Window {
public:
  WindowMac();
  ~WindowMac();

  void set_title(char const* title) override;

  void paint(CGContextRef ctx);

private:
  NSWindow* window_ = nullptr;
};

class UIManagerMac final : public UIManager {
public:
  UIManagerMac();
  ~UIManagerMac();

  Window* create_window() override;
  void start() override;

private:
  void init_menu();

  NSApplication* app_ = nullptr;
  SpargelApplication* app_delegate_ = nullptr;
};

}  // namespace spargel::ui
