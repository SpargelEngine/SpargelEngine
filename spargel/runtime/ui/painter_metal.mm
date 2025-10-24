#include "spargel/runtime/ui/painter.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>

#include "spargel/runtime/logging.h"
#include "spargel/runtime/ui/window_appkit.h"

namespace spargel::runtime::ui {
namespace {
void check_error(NSError* error, char const* msg) {
    if (error) {
        auto info = error.localizedDescription;
        LOG_FATAL("%s:\n%s", msg, info.UTF8String);
        throw;
    }
}
class PainterMetal final : public Painter {
public:
    PainterMetal() : vertex_buffer_{this}, index_buffer_{this} {
        device_ = MTLCreateSystemDefaultDevice();
        queue_ = [device_ newCommandQueue];
        init_texture();
        init_pipelines();
    }

    struct UniformData {
        Vec2f viewport_size;
    };

    void render(CommandList const& cmdlist) override {
        vertex_buffer_.request(cmdlist.vertex_count() * 24);
        index_buffer_.request(cmdlist.index_count() * 4);

        memcpy(vertex_buffer_.object.contents, cmdlist.vertex_data(),
               cmdlist.vertex_count() * 24);
        memcpy(index_buffer_.object.contents, cmdlist.index_data(),
               cmdlist.index_count() * 4);

        auto width = view_.bounds.size.width;
        auto height = view_.bounds.size.height;

        auto command_buffer = [queue_ commandBuffer];
        {
            auto pass_desc = view_.currentRenderPassDescriptor;
            UniformData uniform = {{float(width), float(height)}};
            auto encoder =
                [command_buffer renderCommandEncoderWithDescriptor:pass_desc];
            [encoder setRenderPipelineState:render_pipeline_];
            [encoder setVertexBytes:&uniform length:sizeof(uniform) atIndex:0];
            [encoder setVertexBuffer:vertex_buffer_.object offset:0 atIndex:1];
            [encoder setFragmentTexture:texture_ atIndex:0];
            [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                indexCount:cmdlist.index_count()
                                 indexType:MTLIndexTypeUInt32
                               indexBuffer:index_buffer_.object
                         indexBufferOffset:0];
            [encoder endEncoding];
        }
        auto drawable = view_.currentDrawable;
        [command_buffer presentDrawable:drawable];
        [command_buffer commit];
    }

    void bind_window(Window* window) override {
        auto appkit_window = static_cast<WindowAppKit*>(window);
        view_ = appkit_window->mtk_view();
        view_.device = device_;
        view_.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    }

private:
    struct GrowingBuffer {
        GrowingBuffer(PainterMetal* p) : painter{p} {}

        void request(size_t length) {
            if (length == 0) {
                return;
            }
            if (!object) {
                object = painter->create_buffer(length);
                return;
            }
            if (length <= object.length) {
                return;
            }
            object = painter->create_buffer(length);
        }

        PainterMetal* painter = nullptr;
        id<MTLBuffer> object = nullptr;
    };

    id<MTLBuffer> create_buffer(size_t size) {
        return [device_ newBufferWithLength:size
                                    options:MTLResourceStorageModeShared];
    }

    static constexpr size_t TEXTURE_SIZE = 16;

    void init_texture() {
        auto desc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatA8Unorm
                                         width:TEXTURE_SIZE
                                        height:TEXTURE_SIZE
                                     mipmapped:false];
        desc.storageMode = MTLStorageModeShared;
        texture_ = [device_ newTextureWithDescriptor:desc];
        uint8_t data = 255;
        [texture_ replaceRegion:MTLRegionMake2D(0, 0, 1, 1)
                    mipmapLevel:0
                      withBytes:&data
                    bytesPerRow:1];
    }

    static constexpr char const* SHADER_SOURCE = R"METAL(
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
    half4 color;
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
    out.color = half4(in.color) / 255.0h;
    return out;
}
[[fragment]]
half4 fragment_main(
    VertexOut in [[stage_in]],
    texture2d<half, access::sample> tex [[texture(0)]]
) {
    constexpr sampler
        linear_sampler(coord::normalized,
                       min_filter::linear,
                       mag_filter::linear,
                       mip_filter::linear);
    half4 tex_color = tex.sample(linear_sampler, in.uv);
    return in.color * tex_color.a;
}
    )METAL";

    void init_pipelines() {
        NSError* error = nullptr;
        library_ = [device_
            newLibraryWithSource:[NSString stringWithUTF8String:SHADER_SOURCE]
                         options:nullptr
                           error:&error];
        check_error(error, "cannot create pipeline");
        vertex_shader_ = [library_ newFunctionWithName:@"vertex_main"];
        fragment_shader_ = [library_ newFunctionWithName:@"fragment_main"];
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
            ppl_desc.colorAttachments[0].pixelFormat =
                MTLPixelFormatBGRA8Unorm_sRGB;
            ppl_desc.vertexFunction = vertex_shader_;
            ppl_desc.fragmentFunction = fragment_shader_;
            ppl_desc.vertexDescriptor = vtx_desc;

            render_pipeline_ =
                [device_ newRenderPipelineStateWithDescriptor:ppl_desc
                                                        error:&error];
            check_error(error, "cannot build pipeline");
        }
        {
            pass_desc_ = [[MTLRenderPassDescriptor alloc] init];
            pass_desc_.colorAttachments[0].loadAction = MTLLoadActionClear;
            pass_desc_.colorAttachments[0].storeAction = MTLStoreActionStore;
            pass_desc_.colorAttachments[0].clearColor =
                MTLClearColorMake(0, 1, 1, 1);
        }
    }

    id<MTLDevice> device_ = nullptr;
    id<MTLCommandQueue> queue_ = nullptr;
    MTKView* view_ = nullptr;

    id<MTLLibrary> library_ = nullptr;
    id<MTLFunction> vertex_shader_ = nullptr;
    id<MTLFunction> fragment_shader_ = nullptr;
    id<MTLRenderPipelineState> render_pipeline_ = nullptr;
    MTLRenderPassDescriptor* pass_desc_ = nullptr;

    // the default texture is used to render various things
    id<MTLTexture> texture_ = nullptr;

    GrowingBuffer vertex_buffer_ = nullptr;
    GrowingBuffer index_buffer_ = nullptr;
};
}  // namespace
Painter* Painter::create() { return new PainterMetal; }
}  // namespace spargel::runtime::ui
