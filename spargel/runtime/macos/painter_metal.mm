#include "spargel/runtime/painter.h"

#import <Metal/Metal.h>
#import <dispatch/dispatch.h>

#include "spargel/runtime/logging.h"

namespace spargel::runtime {
    class PainterMetal final : public Painter {
    public:
        PainterMetal() {
            device_ = MTLCreateSystemDefaultDevice();
            init_pipelines();
        }

    private:
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
            struct VertexOut {
                float4 pos [[position]];
                float2 uv;
                float4 color;
            };
            [[vertex]]
            VertexOut vertex_main(
                constant UniformData const& uniforms [[buffer(0)]],
                DrawVertex in [[stage_in]] /* [[buffer(1)]] */) {
                VertexOut out;
                out.pos = float4(in.pos, 0.0, 1.0);
                out.pos.xy /= uniforms.viewport_size / 2.0;
                out.uv = in.uv;
                out.color = float4(in.color) / 255.0;
                return out;
            }
            [[fragment]]
            float4 fragment_main(
                VertexOut in [[stage_in]],
                texture2d<float, access::sample> tex [[texture(0)]]) {
                constexpr sampler
                    linear_sampler(coord::normalized,
                                   min_filter::linear,
                                   mag_filter::linear,
                                   mip_filter::linear);
                float4 tex_color = tex.sample(linear_sampler, in.uv);
                return in.color * tex_color;
            }
        )METAL";

        void init_pipelines() {
            NSError* error = nullptr;
            library_ = [device_
                newLibraryWithSource:[NSString
                                         stringWithUTF8String:SHADER_SOURCE]
                             options:nullptr
                               error:&error];
            if (error) {
                LOG_FATAL("cannot compile shader");
                throw;
            }
            vertex_func_ = [library_ newFunctionWithName:@"vertex_main"];
            fragment_func_ = [library_ newFunctionWithName:@"fragment_main"];
            // TODO(tianjiao): create pipeline
        }

        id<MTLDevice> device_ = nullptr;
        id<MTLLibrary> library_ = nullptr;
        id<MTLFunction> vertex_func_ = nullptr;
        id<MTLFunction> fragment_func_ = nullptr;
    };

    Painter* Painter::create() { return new PainterMetal; }
}  // namespace spargel::runtime
