#pragma once

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "spargel/gpu/context_metal.h"
#include "spargel/ui/context.h"

@interface SpargelApplicationDelegate : NSObject <NSApplicationDelegate>
@end

@interface SpargelWindowDelegate : NSObject <NSWindowDelegate>
@end

@interface SpargelViewController : NSViewController <MTKViewDelegate>
@end

namespace spargel::ui {

class BackendMac final : public Backend {
public:
  ~BackendMac();

  char const* name() const override;
  void init() override;
  void set_title(char const* title) override;
  void start_with(RenderDelegate* delegate) override;
  void upload_bitmap(TexturePart part, text::Bitmap const& bitmap) override;

  // called from SpargelViewController
  void set_mtk_view(MTKView* view);
  MTKView* mtk_view();
  void render();

private:
  class GrowingBuffer {
  public:
    ~GrowingBuffer();

    void set_device(id<MTLDevice> device);
    id<MTLBuffer> object();
    void request(size_t length);

  private:
    id<MTLBuffer> create_buffer(size_t size);

    id<MTLDevice> device_ = nullptr;
    id<MTLBuffer> object_ = nullptr;
  };

  void init_app();
  void init_window();
  void init_gpu();
  void create_pipeline();
  void create_texture();

  NSApplication* nsapp_ = nullptr;
  SpargelApplicationDelegate* nsapp_delegate_ = nullptr;
  NSWindow* ns_window_ = nullptr;
  SpargelViewController* view_controller_ = nullptr;
  MTKView* view_ = nullptr;
  gpu::ContextMetal* gpu_ctx_ = nullptr;
  id<MTLRenderPipelineState> render_pipeline_ = nullptr;
  id<MTLTexture> texture_ = nullptr;
  GrowingBuffer vertex_buffer_;
  GrowingBuffer index_buffer_;
  RenderDelegate* delegate_ = nullptr;
};

}  // namespace spargel::ui
