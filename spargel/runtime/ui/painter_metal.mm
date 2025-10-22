#include "spargel/runtime/ui/painter.h"

#import <Metal/Metal.h>
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
}  // namespace
class PainterMetal final : public Painter {
public:
    PainterMetal() {
        device_ = MTLCreateSystemDefaultDevice();
        queue_ = [device_ newCommandQueue];

        // strategy_ = new TiledSplitStrategy(this);
        strategy_ = new CompactTileStrategy(this);
        LOG_INFO("using render strategy: %s", strategy_->name());
    }

    void render(CommandList const& cmdlist) override {
        auto drawable = [layer_ nextDrawable];
        strategy_->render(cmdlist, drawable);
    }

    void bind_window(Window* window) override {
        auto appkit_window = static_cast<WindowAppKit*>(window);
        layer_ = appkit_window->metal_layer();
        layer_.device = device_;
        layer_.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
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

    class RenderStrategy {
    public:
        RenderStrategy(PainterMetal* painter, char const* name)
            : name_{name}, device_{painter->device_}, queue_{painter->queue_} {}

        char const* name() const { return name_; }
        id<MTLDevice> device() { return device_; }
        id<MTLCommandQueue> queue() { return queue_; }

        virtual void render(CommandList const& cmdlist,
                            id<CAMetalDrawable> drawable) = 0;

    private:
        char const* name_ = nullptr;
        id<MTLDevice> device_ = nullptr;
        id<MTLCommandQueue> queue_ = nullptr;
    };

    class CompactTileStrategy final : public RenderStrategy {
    public:
        CompactTileStrategy(PainterMetal* painter)
            : RenderStrategy{painter, "CompactTileStrategy"} {
            init_pipelines();
        }

        void render(CommandList const& cmdlist,
                    id<CAMetalDrawable> drawable) override {
            throw "not implemented";
        }

    private:
        static constexpr char const* SHADER_SOURCE = R"METAL(
#include <metal_stdlib>
using namespace metal;

enum {
    CMD_FILL_RECT = 0,
    CMD_FILL_CIRCLE,
    CMD_FILL_ROUNDED_RECT,
    CMD_STROKE_SEGMENT,
    CMD_STROKE_CIRCLE,
    CMD_SET_CLIP,
    CMD_CLEAR_CLIP,
    CMD_SAMPLE_TEXTURE,
    CMD_DUMP,
};

struct UniformData {
    uint cmd_count;
    uint2 tile_count;
};

struct Command {
    uchar cmd;
    float4 clip;
    float data[8];
};
static_assert(sizeof(Command) == 64, "bad size");

struct CommandInfo {
    uint cmd_id;
    bool intersects_clip;
    float4 bbox;
};

bool bboxIntersect(float4 a, float4 b) {
    return (abs((a.x + a.z / 2) - (b.x + b.z / 2)) * 2 < (a.z + b.z)) &&
           (abs((a.y + a.w / 2) - (b.y + b.w / 2)) * 2 < (a.w + b.w));
}

// one thread per command
[[kernel]]
void build_info(
    ushort tid [[thread_position_in_grid]],
    constant UniformData const& uniform [[buffer(0)]],
    constant Command const* cmds [[buffer(1)]],
    device CommandInfo* infos [[buffer(2)]]
) {
    if (tid >= uniform.cmd_count) {
        return;
    }
    Command cmd = cmds[tid];
    float4 bbox = 0;
    if (cmd.cmd == CMD_FILL_CIRCLE) {
        float2 center = float2(cmd.data[0], cmd.data[1]);
        float radius = cmd.data[2];
        radius += 0.5;
        bbox.xy = center - radius;
        bbox.zw = float2(radius) * 2;
    } else if (cmd.cmd == CMD_FILL_ROUNDED_RECT) {
        float2 origin = float2(cmd.data[0], cmd.data[1]);
        float2 size = float2(cmd.data[2], cmd.data[3]);
        bbox.xy = origin - 0.5;
        bbox.zw = size + 1;
    } else if (cmd.cmd == CMD_STROKE_SEGMENT) {
        float2 start = float2(cmd.data[0], cmd.data[1]);
        float2 end = float2(cmd.data[2], cmd.data[3]);
        bbox.xy = min(start, end) - 0.5;
        bbox.zw = abs(end - start) + 1;
    } else if (cmd.cmd == CMD_STROKE_CIRCLE) {
        float2 center = float2(cmd.data[0], cmd.data[1]);
        float radius = cmd.data[2];
        radius += 0.5;
        bbox.xy = center - radius;
        bbox.zw = float2(radius) * 2;
    } else if (cmd.cmd == CMD_SAMPLE_TEXTURE) {
        float2 origin = float2(cmd.data[0], cmd.data[1]);
        float2 size = float2(cmd.data[2], cmd.data[3]);
        bbox.xy = origin - 0.5;
        bbox.zw = size + 1;
    } else if (cmd.cmd == CMD_DUMP) {
    } else {
        // do nothing
    }
    device CommandInfo& info = infos[tid];
    info.cmd_id = tid;
    info.intersects_clip = bboxIntersect(bbox, cmd.clip);
    info.bbox = bbox;
}

// count how many tiles each command intersects
// one thread per command
[[kernel]]
void binning_count(
    ushort tid [[thread_position_in_grid]],
    constant UniformData const& uniform [[buffer(0)]],
    constant CommandInfo const* cmds [[buffer(1)]],
    device atomic_uint* tile_cmd_counts [[buffer(2)]]
) {
    if (tid >= uniform.cmd_count) {
        return;
    }
    float4 bbox = cmds[tid].bbox;
    // (origin, size) -> (p_min, p_max)
    bbox.z += bbox.x;
    bbox.w += bbox.y;

    uint2 tmin = uint2(floor(bbox.xy / 8.0));
    uint2 tmax = uint2(ceil(bbox.zw / 8.0));

    for (uint i = tmin.x; i <= tmax.x; i++) {
        for (uint j = tmin.y; j <= tmax.y; j++) {
            atomic_fetch_add_explicit(
                &tile_cmd_counts[i + j * uniform.tile_count.x],
                1,
                memory_order_relaxed
            );
        }
    }
}

// prefix sum
// we shall implement chained scan with decoupled lookback
[[kernel]]
void prefix_sum(
    constant UniformData const& uniform [[buffer(0)]],
    constant uint const* counts [[buffer(1)]]
) {
    // TODO
}
            )METAL";

        void init_pipelines() {
            NSError* error = nullptr;
            {
                library_ = [device()
                    newLibraryWithSource:[NSString
                                             stringWithUTF8String:SHADER_SOURCE]
                                 options:nullptr
                                   error:&error];
                check_error(error, "cannot compiler shader");
                build_info_func_ = [library_ newFunctionWithName:@"build_info"];
                binning_count_func_ =
                    [library_ newFunctionWithName:@"binning_count"];
            }
        }

        id<MTLLibrary> library_ = nullptr;
        id<MTLFunction> build_info_func_ = nullptr;
        id<MTLFunction> binning_count_func_ = nullptr;
    };

    class TiledSplitStrategy final : public RenderStrategy {
    public:
        TiledSplitStrategy(PainterMetal* painter)
            : RenderStrategy{painter, "TiledSplitStrategy"},
              scene_commands_buffer_{painter},
              bin_slots_buffer_{painter},
              bin_alloc_buffer_{painter} {
            init_pipelines();
        }

        void render(CommandList const& cmdlist,
                    id<CAMetalDrawable> drawable) override {
            auto target = drawable.texture;
            size_t tile_count_x = target.width / 8 + 1;
            size_t tile_count_y = target.height / 8 + 1;
            size_t tile_count = tile_count_x * tile_count_y;

            size_t command_count = cmdlist.count();
            size_t commands_size = sizeof(DrawCommand) * command_count;
            size_t max_slot = cmdlist.estimate_slots() + tile_count;

            BinControl bin_control{uint32_t(tile_count_x),
                                   uint32_t(tile_count_y),
                                   uint32_t(command_count), uint32_t(max_slot)};

            scene_commands_buffer_.request(commands_size);
            bin_slots_buffer_.request(sizeof(BinSlot) * max_slot);
            bin_alloc_buffer_.request(sizeof(BinAlloc));

            memcpy([scene_commands_buffer_.object contents], cmdlist.data(),
                   commands_size);

            BinAlloc alloc_initial{uint32_t(tile_count), 0};
            memcpy([bin_alloc_buffer_.object contents], &alloc_initial,
                   sizeof(BinAlloc));

            auto command_buffer = [queue() commandBuffer];

            {
                auto encoder = [command_buffer computeCommandEncoder];
                [encoder setComputePipelineState:binning_pipeline_];
                [encoder setBytes:&bin_control
                           length:sizeof(BinControl)
                          atIndex:0];
                [encoder setBuffer:scene_commands_buffer_.object
                            offset:0
                           atIndex:1];
                [encoder setBuffer:bin_slots_buffer_.object offset:0 atIndex:2];
                [encoder setBuffer:bin_alloc_buffer_.object offset:0 atIndex:3];
                [encoder
                     dispatchThreadgroups:MTLSizeMake(tile_count_x / 8 + 1,
                                                      tile_count_y / 8 + 1, 1)
                    threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
                [encoder endEncoding];
            }
            {
                pass_desc_.colorAttachments[0].texture = target;
                auto encoder = [command_buffer
                    renderCommandEncoderWithDescriptor:pass_desc_];
                [encoder setRenderPipelineState:render_pipeline_];
                [encoder setFragmentBytes:&bin_control
                                   length:sizeof(BinControl)
                                  atIndex:0];
                [encoder setFragmentBuffer:scene_commands_buffer_.object
                                    offset:0
                                   atIndex:1];
                [encoder setFragmentBuffer:bin_slots_buffer_.object
                                    offset:0
                                   atIndex:2];
                // [encoder setFragmentTexture:nullptr atIndex:0];
                [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                            vertexStart:0
                            vertexCount:3];
                [encoder endEncoding];
            }
            [command_buffer presentDrawable:drawable];
            [command_buffer commit];
            [command_buffer waitUntilCompleted];
        }

    private:
        static constexpr char const* SHADER_SOURCE = R"METAL(
#include <metal_stdlib>
using namespace metal;

#define ATLAS_SIZE 2048

enum {
    CMD_FILL_RECT = 0,
    CMD_FILL_CIRCLE,
    CMD_FILL_ROUNDED_RECT,
    CMD_STROKE_SEGMENT,
    CMD_STROKE_CIRCLE,
    CMD_SET_CLIP,
    CMD_CLEAR_CLIP,
    CMD_SAMPLE_TEXTURE,
    CMD_DUMP,
};

struct UniformData {
    uint cmd_count;
};

struct Sdf_VOut {
    float4 position [[position]];
};

float sdf_circle_fill(float2 p, float r) {
    return length(p) - r;
}

float sdf_circle_stroke(float2 p, float r) {
    return abs(length(p) - r);
}

float sdf_segment(float2 p, float2 a, float2 b) {
    float2 ap = p - a;
    float2 ab = b - a;
    float t = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    return length(ap - ab * t);
}

// centered at origin, s is the size.
float sdf_rrect(float2 p, float2 s, float r) {
    float2 q = abs(p) - (s / 2.0 - r);
    return min(max(q.x, q.y), 0.0f) + length(max(q, 0.0f)) - r;
}


struct Command2 {
    uchar cmd;
    float4 clip;
    float data[8];
};
static_assert(sizeof(Command2) == 64, "size does not match");

// One command for the tile.
struct BinSlot {
    // Index of the command in the command buffer.
    uint command_index;
    // Index of the next slot.
    uint next_slot;
};
static_assert(sizeof(BinSlot) == 8, "size error");

struct BinControl {
    uint tile_count_x;
    uint tile_count_y;
    uint cmd_count;
    // The capacity of the BinSlot buffer.
    uint max_slot;
};

struct BinAlloc {
    // Set to zero externally.
    // NOTE: Offset keeps increasing even if the buffer is out of space.
    atomic_uint offset;
    bool out_of_space;
};
static_assert(sizeof(BinAlloc) == 8, "size error");


constant float4 VERTICES[] = {
    {-1.0,  1.0, 0.0, 1.0},
    {-1.0, -3.0, 0.0, 1.0},
    { 3.0,  1.0, 0.0, 1.0},
};

[[vertex]]
float4 sdf_vert(uint vid [[vertex_id]]) {
    return VERTICES[vid];
}

bool bboxIntersect(float4 a, float4 b) {
    return (abs((a.x + a.z / 2) - (b.x + b.z / 2)) * 2 < (a.z + b.z)) &&
           (abs((a.y + a.w / 2) - (b.y + b.w / 2)) * 2 < (a.w + b.w));
}

// Each thread computes the commands of a 8x8 tile.
//
// The first slot is reserved.
[[kernel]]
void sdf_binning(
    ushort2 tid [[thread_position_in_grid]],
    // ushort2 grid_size [[threads_per_grid]],
    constant BinControl const& uniform [[buffer(0)]],
    constant Command2 const* cmds [[buffer(1)]],
    device BinSlot* slots [[buffer(2)]],
    device BinAlloc& alloc [[buffer(3)]]
) {
    if (tid.x >= uniform.tile_count_x || tid.y >= uniform.tile_count_y) {
        return;
    }

    float4 tile;
    tile.xy = float2(tid) * 8.0;
    tile.zw = 8.0;

    uint slot_id = tid.x * uniform.tile_count_y + tid.y;

    for (uint i = 0; i < uniform.cmd_count; i++) {
        threadgroup_barrier(mem_flags::mem_none);

        Command2 cmd = cmds[i];

        // Compute the bounding box of the command.

        float4 bbox = 0;
        if (cmd.cmd == CMD_FILL_CIRCLE) {
            float2 center = float2(cmd.data[0], cmd.data[1]);
            float radius = cmd.data[2];
            radius += 0.5;
            bbox.xy = center - radius;
            bbox.zw = float2(radius) * 2;
        } else if (cmd.cmd == CMD_FILL_ROUNDED_RECT) {
            float2 origin = float2(cmd.data[0], cmd.data[1]);
            float2 size = float2(cmd.data[2], cmd.data[3]);
            bbox.xy = origin - 0.5;
            bbox.zw = size + 1;
        } else if (cmd.cmd == CMD_STROKE_SEGMENT) {
            float2 start = float2(cmd.data[0], cmd.data[1]);
            float2 end = float2(cmd.data[2], cmd.data[3]);
            bbox.xy = min(start, end) - 0.5;
            bbox.zw = abs(end - start) + 1;
        } else if (cmd.cmd == CMD_STROKE_CIRCLE) {
            float2 center = float2(cmd.data[0], cmd.data[1]);
            float radius = cmd.data[2];
            radius += 0.5;
            bbox.xy = center - radius;
            bbox.zw = float2(radius) * 2;
        } else if (cmd.cmd == CMD_SAMPLE_TEXTURE) {
            float2 origin = float2(cmd.data[0], cmd.data[1]);
            float2 size = float2(cmd.data[2], cmd.data[3]);
            bbox.xy = origin - 0.5;
            bbox.zw = size + 1;
        } else if (cmd.cmd == CMD_DUMP) {
        } else {
            // do nothing
        }

        // Check intersection.
        if (!bboxIntersect(tile, bbox)) {
            continue;
        }

        // Allocate a new slot.

        uint new_slot = atomic_fetch_add_explicit(&alloc.offset, 1, memory_order_relaxed);

        if (new_slot >= uniform.max_slot) {
            alloc.out_of_space = true;
            continue;
        }

        slots[slot_id].next_slot = new_slot;
        slots[slot_id].command_index = i;
        slot_id = new_slot;
    }
    slots[slot_id].next_slot = -1;
    slots[slot_id].command_index = -1;
}

[[fragment]]
float4 sdf_frag(
    Sdf_VOut in [[stage_in]],
    constant BinControl const& uniform [[buffer(0)]],
    constant Command2 const* cmds [[buffer(1)]],
    constant BinSlot* slots [[buffer(2)]] /*,
    texture2d<float> glyph_texture [[texture(0)]]*/
) {
    //constexpr sampler texture_sampler(mag_filter::linear, min_filter::linear);
    constexpr sampler texture_sampler;

    float2 p = in.position.xy;
    float4 col = float4(0, 0, 0, 1);
    float4 clip = float4(0, 0, 0, 0);

    uint2 tile = uint2(p) / 8;
    uint slot_id = tile.x * uniform.tile_count_y + tile.y;

    //for (uint i = 0; i < uniform.cmd_count; i++) {
    //    uint8_t cmd = cmds[i];
    while (true) {
        threadgroup_barrier(mem_flags::mem_none);

        uint cmd_id = slots[slot_id].command_index;
        uint next_slot = slots[slot_id].next_slot;

        if (cmd_id >= uniform.cmd_count || next_slot >= uniform.max_slot) {
            break;
        }

        slot_id = next_slot;

        // TODO: Put command in the threadgroup memory.
        Command2 cmd = cmds[cmd_id];

        float d = 1.0;
        float4 c1 = float4(1, 0, 1, 1);
        clip = cmd.clip;

        if (cmd.cmd == CMD_FILL_CIRCLE) {
            float2 center = float2(cmd.data[0], cmd.data[1]);
            float radius = cmd.data[2];
            c1 = float4(as_type<uchar4>(cmd.data[3])) / 255.0;
            d = sdf_circle_fill(p - center, radius);
        } else if (cmd.cmd == CMD_FILL_ROUNDED_RECT) {
            float2 origin = float2(cmd.data[0], cmd.data[1]);
            float2 size = float2(cmd.data[2], cmd.data[3]);
            float radius = cmd.data[4];
            c1 = float4(as_type<uchar4>(cmd.data[5])) / 255.0;
            d = sdf_rrect(p - origin - size / 2.0, size, radius);
        } else if (cmd.cmd == CMD_STROKE_SEGMENT) {
            float2 start = float2(cmd.data[0], cmd.data[1]);
            float2 end = float2(cmd.data[2], cmd.data[3]);
            c1 = float4(as_type<uchar4>(cmd.data[4])) / 255.0;
            d = sdf_segment(p, start, end);
        } else if (cmd.cmd == CMD_STROKE_CIRCLE) {
            float2 center = float2(cmd.data[0], cmd.data[1]);
            float radius = cmd.data[2];
            c1 = float4(as_type<uchar4>(cmd.data[3])) / 255.0;
            d = sdf_circle_stroke(p - center, radius);
        } /* else if (cmd.cmd == CMD_SAMPLE_TEXTURE) {
            float2 origin = float2(cmd.data[0], cmd.data[1]);
            float2 size = float2(cmd.data[2], cmd.data[3]);
            float2 tex_uv0 = float2(as_type<ushort2>(cmd.data[4])) / ATLAS_SIZE;
            float2 tex_uvs = float2(as_type<ushort2>(cmd.data[5])) / ATLAS_SIZE;
            c1 = float4(as_type<uchar4>(cmd.data[6])) / 255.0;

            if (p.x >= origin.x && p.y >= origin.y && p.x <= (origin.x + size.x) && p.y <= (origin.y + size.y)) {
                float2 dp = p - origin;
                float2 uv = (dp / size) * tex_uvs + tex_uv0;
                c1.a *= glyph_texture.sample(texture_sampler, uv).a;
                d = 0.0;
            }
        } else if (cmd.cmd == CMD_DUMP) {
            // TODO:
            float2 uv = p / 1000;
            c1.a *= glyph_texture.sample(texture_sampler, uv).a;
            d = 0.0;
        } */ else {
            // do nothing
        }
        if (p.x >= clip.x && p.y >= clip.y && p.x <= (clip.x + clip.z) && p.y <= (clip.y + clip.w)) {
            // TODO: The mixing model is not good.
            // col = mix(col, c1, clamp(1.0 - d, 0.0, 1.0));
            col.xyz = mix(col.xyz, c1.xyz, clamp(1.0 - d, 0.0, 1.0) * c1.a);
        }
    }
    return float4(col.xyz, 1.0);
}
            )METAL";

        void init_pipelines() {
            NSError* error = nullptr;
            {
                library_ = [device()
                    newLibraryWithSource:[NSString
                                             stringWithUTF8String:SHADER_SOURCE]
                                 options:nullptr
                                   error:&error];
                check_error(error, "cannot compiler shader");
                vertex_func_ = [library_ newFunctionWithName:@"sdf_vert"];
                fragment_func_ = [library_ newFunctionWithName:@"sdf_frag"];
                binning_func_ = [library_ newFunctionWithName:@"sdf_binning"];
            }
            {
                auto ppl_desc = [[MTLRenderPipelineDescriptor alloc] init];
                ppl_desc.colorAttachments[0].pixelFormat =
                    MTLPixelFormatBGRA8Unorm_sRGB;
                ppl_desc.vertexFunction = vertex_func_;
                ppl_desc.fragmentFunction = fragment_func_;
                render_pipeline_ =
                    [device() newRenderPipelineStateWithDescriptor:ppl_desc
                                                             error:&error];
                check_error(error, "cannot build pipeline");
            }
            {
                binning_pipeline_ =
                    [device() newComputePipelineStateWithFunction:binning_func_
                                                            error:&error];
                check_error(error, "cannot build pipeline");
            }
            {
                pass_desc_ = [[MTLRenderPassDescriptor alloc] init];
                pass_desc_.colorAttachments[0].loadAction = MTLLoadActionClear;
                pass_desc_.colorAttachments[0].storeAction =
                    MTLStoreActionStore;
                pass_desc_.colorAttachments[0].clearColor =
                    MTLClearColorMake(0, 1, 1, 1);
            }
        }

        id<MTLLibrary> library_ = nullptr;
        id<MTLFunction> vertex_func_ = nullptr;
        id<MTLFunction> fragment_func_ = nullptr;
        id<MTLFunction> binning_func_ = nullptr;
        id<MTLRenderPipelineState> render_pipeline_ = nullptr;
        id<MTLComputePipelineState> binning_pipeline_ = nullptr;
        MTLRenderPassDescriptor* pass_desc_ = nullptr;

        GrowingBuffer scene_commands_buffer_;
        GrowingBuffer bin_slots_buffer_;
        GrowingBuffer bin_alloc_buffer_;
    };

    static constexpr uint32_t ATLAS_SIZE = 2048;

    // void init_texture() {
    //     auto desc = [MTLTextureDescriptor
    //         texture2DDescriptorWithPixelFormat:MTLPixelFormatA8Unorm
    //                                      width:ATLAS_SIZE
    //                                     height:ATLAS_SIZE
    //                                  mipmapped:false];
    //     desc.storageMode = MTLStorageModeShared;
    //     texture_ = [device_ newTextureWithDescriptor:desc];
    // }

    struct BinAlloc {
        uint32_t offset;
        bool out_of_space;
    };
    static_assert(sizeof(BinAlloc) == 8);
    struct BinControl {
        uint32_t tile_count_x;
        uint32_t tile_count_y;
        uint32_t cmd_count;
        uint32_t max_slot;
    };
    static_assert(sizeof(BinControl) == 16);
    struct BinSlot {
        uint32_t next_slot;
        uint32_t command_index;
    };
    static_assert(sizeof(BinSlot) == 8);

    id<MTLBuffer> create_buffer(size_t size) {
        return [device_ newBufferWithLength:size
                                    options:MTLResourceStorageModeShared];
    }

    id<MTLDevice> device_ = nullptr;
    id<MTLCommandQueue> queue_ = nullptr;
    CAMetalLayer* layer_ = nullptr;
    RenderStrategy* strategy_ = nullptr;
};

Painter* Painter::create() { return new PainterMetal; }
}  // namespace spargel::runtime::ui
