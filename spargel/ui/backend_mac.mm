#include "spargel/ui/backend_mac.h"

#include "spargel/apple/bridge_cast.h"
#include "spargel/apple/retain_ptr.h"
#include "spargel/base/check.h"
#include "spargel/base/logging.h"
#include "spargel/ui/context.h"

@implementation SpargelApplicationDelegate
- (bool)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return true;
}
@end

@implementation SpargelViewController {
  spargel::ui::Context* spargel_context_;
  spargel::ui::BackendMac* spargel_backend_;
  MTKView* view_;
  NSTrackingArea* tracking_area_;
}
- (nonnull instancetype)initWithSpargel:
    (spargel::ui::BackendMac*)spargel_window {
  self = [super init];
  if (self) {
    spargel_context_ = &spargel::ui::Context::get();
    spargel_backend_ = spargel_window;
    view_ = nullptr;
    tracking_area_ = nullptr;
  }
  return self;
}
// override: NSViewController
- (void)loadView {
  view_ = [[MTKView alloc] init];
  view_.delegate = self;
  [self updateTrackingArea];
  self.view = view_;
}
- (void)viewDidLoad {
  [super viewDidLoad];
  spargel_backend_->set_mtk_view(view_);
}
- (void)viewDidAppear {
  [self.view.window makeFirstResponder:self];
}
// override: NSResponder
- (bool)acceptsFirstResponder {
  return true;
}
- (void)keyDown:(NSEvent*)event {
  // TODO(tianjiao): why?
  // [self interpretKeyEvents:@[ event ]];
  auto code = [event keyCode];
  LOG_INFO("key down: %u", code);
}
- (void)mouseDown:(NSEvent*)event {
  auto loc = [event locationInWindow];
  auto& state = spargel_context_->input_state();
  state.mouse_down = true;
  state.mouse_position.x = (float)loc.x;
  auto height = view_.bounds.size.height;
  state.mouse_position.y = float(height - loc.y);
}
- (void)mouseUp:(NSEvent*)event {
  auto& state = spargel_context_->input_state();
  state.mouse_down = false;
  state.mouse_released = true;
}
- (void)mouseMoved:(NSEvent*)event {
  auto loc = [event locationInWindow];
  auto& state = spargel_context_->input_state();
  state.mouse_position.x = float(loc.x);
  auto height = view_.bounds.size.height;
  state.mouse_position.y = float(height - loc.y);
}
- (void)mouseDragged:(NSEvent*)event {
  auto loc = [event locationInWindow];
  auto& state = spargel_context_->input_state();
  state.mouse_position.x = float(loc.x);
  auto height = view_.bounds.size.height;
  state.mouse_position.y = float(height - loc.y);
}
// override: MTKViewDelegate
- (void)drawInMTKView:(MTKView*)view {
  CHECK(spargel_backend_);
  spargel_backend_->render();
}
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
}
// tracking area
- (void)updateTrackingArea {
  if (tracking_area_) {
    [view_ removeTrackingArea:tracking_area_];
    tracking_area_ = nullptr;
  }
  NSTrackingAreaOptions options =
      NSTrackingActiveAlways | NSTrackingInVisibleRect |
      NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved;
  tracking_area_ = [[NSTrackingArea alloc] initWithRect:view_.bounds
                                                options:options
                                                  owner:self
                                               userInfo:nullptr];
  [view_ addTrackingArea:tracking_area_];
}
@end

namespace spargel::ui {
Backend* Context::create_backend() { return new BackendMac; }

BackendMac::~BackendMac() {
  render_pipeline_ = nullptr;
  view_ = nullptr;
  view_controller_ = nullptr;
  ns_window_ = nullptr;
  nsapp_delegate_ = nullptr;
  nsapp_ = nullptr;
}
char const* BackendMac::name() const { return "AppKit + Metal"; }
void BackendMac::init() {
  init_app();
  init_window();
  init_gpu();
}
void BackendMac::set_title(char const* title) {
  ns_window_.title = [NSString stringWithUTF8String:title];
}
void BackendMac::start_with(RenderDelegate* delegate) {
  delegate_ = delegate;
  [nsapp_ run];
}
void BackendMac::set_mtk_view(MTKView* view) { view_ = view; }
MTKView* BackendMac::mtk_view() { return view_; }
namespace {
struct UniformData {
  math::Vec2f viewport_size;
};
}  // namespace
void BackendMac::render() {
  if (delegate_) {
    delegate_->render();
  }

  auto& cmdlist = Context::get().command_list();
  auto width = view_.bounds.size.width;
  auto height = view_.bounds.size.height;

  {
    vertex_buffer_.request(cmdlist.vertex_count() * 24);
    index_buffer_.request(cmdlist.index_count() * 4);

    memcpy(vertex_buffer_.object().contents, cmdlist.vertex_data(),
           cmdlist.vertex_count() * 24);
    memcpy(index_buffer_.object().contents, cmdlist.index_data(),
           cmdlist.index_count() * 4);
  }

  auto command_buffer = [gpu_ctx_->queue() commandBuffer];

  {
    auto pass_desc = view_.currentRenderPassDescriptor;
    UniformData uniform = {{float(width), float(height)}};
    auto encoder =
        [command_buffer renderCommandEncoderWithDescriptor:pass_desc];
    [encoder setRenderPipelineState:render_pipeline_];
    [encoder setVertexBytes:&uniform length:sizeof(uniform) atIndex:0];
    [encoder setVertexBuffer:vertex_buffer_.object() offset:0 atIndex:1];
    [encoder setFragmentTexture:texture_ atIndex:0];
    [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                        indexCount:cmdlist.index_count()
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:index_buffer_.object()
                 indexBufferOffset:0];
    [encoder endEncoding];
  }

  auto drawable = view_.currentDrawable;
  [command_buffer presentDrawable:drawable];
  [command_buffer commit];

  // TODO: this is hacky
  auto& ctx = Context::get();
  ctx.input_state().end_frame();
}
void BackendMac::init_app() {
  nsapp_ = [NSApplication sharedApplication];
  nsapp_delegate_ = [[SpargelApplicationDelegate alloc] init];
  nsapp_.delegate = nsapp_delegate_;
  [nsapp_ setActivationPolicy:NSApplicationActivationPolicyRegular];
}
void BackendMac::init_window() {
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
void BackendMac::init_gpu() {
  gpu_ctx_ = static_cast<gpu::ContextMetal*>(&gpu::Context::get());
  CHECK(view_);
  auto device = gpu_ctx_->device();
  view_.device = device;
  view_.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
  create_pipeline();
  create_texture();
  vertex_buffer_.set_device(device);
  index_buffer_.set_device(device);
}
namespace {
void check_error(NSError* error, char const* msg) {
  if (error) {
    auto info = error.localizedDescription;
    LOG_FATAL("%s:\n%s", msg, info.UTF8String);
    throw;
  }
}
constexpr char const* SHADER_SOURCE = R"METAL(
#include <metal_stdlib>
using namespace metal;
struct UniformData {
    float2 viewport_size;
};
struct DrawVertex {
    float2 pos [[attribute(0)]];
    float2 uv [[attribute(1)]];
    uchar4 color [[attribute(2)]];
};
static_assert(sizeof(DrawVertex) == 24, "size issue");
struct VertexOut {
    float4 pos [[position]];
    float2 uv;
    float4 color;
};
[[vertex]]
VertexOut vertex_main(
    constant UniformData const& uniforms [[buffer(0)]],
    DrawVertex in [[stage_in]] /* [[buffer(1)]] */
) {
    VertexOut out;
    out.pos = float4(in.pos, 0.0, 1.0);
    out.pos.xy /= uniforms.viewport_size / 2.0;
    out.pos.xy -= 1.0;
    out.pos.y = -out.pos.y;
    out.uv = in.uv;
    out.color = float4(in.color) / 255.0f;
    return out;
}
[[fragment]]
float4 fragment_main(
    VertexOut in [[stage_in]],
    texture2d<float, access::sample> tex [[texture(0)]]
) {
    constexpr sampler
        linear_sampler(coord::normalized,
                       min_filter::linear,
                       mag_filter::linear,
                       mip_filter::linear);
    float alpha = tex.sample(linear_sampler, in.uv).a;
    return in.color * alpha;
}
    )METAL";
}  // namespace
void BackendMac::create_pipeline() {
  auto device = gpu_ctx_->device();
  NSError* error = nullptr;
  auto library =
      [device newLibraryWithSource:[NSString stringWithUTF8String:SHADER_SOURCE]
                           options:nullptr
                             error:&error];
  check_error(error, "cannot create pipeline");
  auto vertex_shader_ = [library newFunctionWithName:@"vertex_main"];
  auto fragment_shader_ = [library newFunctionWithName:@"fragment_main"];
  {
    auto vtx_desc = [MTLVertexDescriptor vertexDescriptor];
    // float2 pos
    vtx_desc.attributes[0].bufferIndex = 1;
    vtx_desc.attributes[0].offset = 0;
    vtx_desc.attributes[0].format = MTLVertexFormatFloat2;
    // float2 uv
    vtx_desc.attributes[1].bufferIndex = 1;
    vtx_desc.attributes[1].offset = 8;
    vtx_desc.attributes[1].format = MTLVertexFormatFloat2;
    // uchar4 color
    vtx_desc.attributes[2].bufferIndex = 1;
    vtx_desc.attributes[2].offset = 16;
    vtx_desc.attributes[2].format = MTLVertexFormatUChar4;

    vtx_desc.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
    vtx_desc.layouts[1].stepRate = 1;
    vtx_desc.layouts[1].stride = 24;

    auto ppl_desc = [[MTLRenderPipelineDescriptor alloc] init];
    ppl_desc.vertexFunction = vertex_shader_;
    ppl_desc.fragmentFunction = fragment_shader_;
    ppl_desc.vertexDescriptor = vtx_desc;
    ppl_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    ppl_desc.colorAttachments[0].blendingEnabled = true;
    ppl_desc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    ppl_desc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    ppl_desc.colorAttachments[0].sourceRGBBlendFactor =
        MTLBlendFactorSourceAlpha;
    ppl_desc.colorAttachments[0].sourceAlphaBlendFactor =
        MTLBlendFactorSourceAlpha;
    ppl_desc.colorAttachments[0].destinationRGBBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
    ppl_desc.colorAttachments[0].destinationAlphaBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;

    render_pipeline_ = [device newRenderPipelineStateWithDescriptor:ppl_desc
                                                              error:&error];
    check_error(error, "cannot build pipeline");
  }
}
void BackendMac::create_texture() {
  constexpr size_t TEXTURE_SIZE = 256;
  auto device = gpu_ctx_->device();
  auto desc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatA8Unorm
                                   width:TEXTURE_SIZE
                                  height:TEXTURE_SIZE
                               mipmapped:false];
  desc.storageMode = MTLStorageModeShared;
  texture_ = [device newTextureWithDescriptor:desc];
  static const uint8_t data = 255;
  [texture_ replaceRegion:MTLRegionMake2D(0, 0, 1, 1)
              mipmapLevel:0
                withBytes:&data
              bytesPerRow:1];
}
BackendMac::GrowingBuffer::~GrowingBuffer() {
  object_ = nullptr;
  device_ = nullptr;
}
id<MTLBuffer> BackendMac::GrowingBuffer::object() { return object_; }
void BackendMac::GrowingBuffer::set_device(id<MTLDevice> device) {
  device_ = device;
}
void BackendMac::GrowingBuffer::request(size_t length) {
  if (length == 0) {
    return;
  }
  if (!object_) {
    object_ = create_buffer(length);
    return;
  }
  if (length <= object_.length) {
    return;
  }
  object_ = create_buffer(length);
}
id<MTLBuffer> BackendMac::GrowingBuffer::create_buffer(size_t size) {
  return [device_ newBufferWithLength:size
                              options:MTLResourceStorageModeShared];
}
void BackendMac::upload_bitmap(TexturePart part, text::Bitmap const& bitmap) {
  [texture_
      replaceRegion:MTLRegionMake2D(part.x, part.y, part.width, part.height)
        mipmapLevel:0
          withBytes:bitmap.data.data()
        bytesPerRow:bitmap.width];
}
}  // namespace spargel::ui
