#include "spargel/render/ui_renderer_metal.h"
#include "spargel/base/check.h"
#include "spargel/base/checked_convert.h"
#include "spargel/base/types.h"
#include "spargel/logging/logging.h"
#include "spargel/render/metal_shaders.h"
#include "spargel/render/ui_scene.h"

//
#include <time.h>

namespace spargel::render {
    base::UniquePtr<UIRenderer> makeMetalUIRenderer(gpu::GPUContext* context,
                                                    text::TextShaper* shaper) {
        return base::makeUnique<UIRendererMetal>(context, shaper);
    }

    UIRendererMetal::UIRendererMetal(gpu::GPUContext* context,
                                     text::TextShaper* text_shaper)
        : UIRenderer{context, text_shaper},
          device_{metal_context()->device()},
          queue_{metal_context()->queue()},
          buffer_pool_{metal_context()} {
        initPipeline();
        initTexture();
        scene_commands_buffer_.context = metal_context();
        scene_data_buffer_.context = metal_context();
        bin_slots_buffer_.context = metal_context();
        bin_alloc_buffer_.context = metal_context();
    }
    void UIRendererMetal::initPipeline() {
        NSError* error = nullptr;

        // Load shaders.
        // auto shaders_resource = resource_manager_->open(
        //     resource::ResourceId("source/spargel/render/ui_shaders.metallib"));
        // spargel_check(shaders_resource.hasValue());
        // auto shaders_data = shaders_resource.value()->getSpan();
        auto shaders_data = METAL_SHADERS.asSpan();
        auto shaders_dispatch_data = dispatch_data_create(
            shaders_data.data(), shaders_data.count(),
            dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);

        // Create shader library.
        library_ = [device_ newLibraryWithData:shaders_dispatch_data
                                         error:&error];
        spargel_check(!error);
        dispatch_release(shaders_dispatch_data);

        // Create functions.
        sdf_vert_ = [library_ newFunctionWithName:@"sdf_vert"];
        sdf_frag_ = [library_ newFunctionWithName:@"sdf_frag"];
        sdf_frag2_ = [library_ newFunctionWithName:@"sdf_frag2"];
        sdf_comp_ = [library_ newFunctionWithName:@"sdf_comp"];
        sdf_comp_v2_ = [library_ newFunctionWithName:@"sdf_comp_v2"];
        sdf_binning_ = [library_ newFunctionWithName:@"sdf_binning"];
        spargel_check(sdf_vert_);
        spargel_check(sdf_frag_);
        spargel_check(sdf_comp_);
        spargel_check(sdf_comp_v2_);
        spargel_check(sdf_binning_);

        // Create pipeline.
        auto ppl_desc = [[MTLRenderPipelineDescriptor alloc] init];
        // ppl_desc.colorAttachments[0].pixelFormat =
        // MTLPixelFormatBGRA8Unorm_sRGB;
        // TODO: Revert to above or find a better way to deal with the target
        // format. srgb is required for presenting, and non-srgb (i.e. linear)
        // is for headless rendering.
        ppl_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        // ppl_desc.colorAttachments[0].blendingEnabled = true;
        // ppl_desc.colorAttachments[0].rgbBlendOperation =
        // MTLBlendOperationAdd;
        // ppl_desc.colorAttachments[0].alphaBlendOperation =
        // MTLBlendOperationAdd;
        // ppl_desc.colorAttachments[0].sourceRGBBlendFactor =
        // MTLBlendFactorSourceAlpha;
        // ppl_desc.colorAttachments[0].sourceAlphaBlendFactor =
        // MTLBlendFactorOne;
        // ppl_desc.colorAttachments[0].destinationRGBBlendFactor =
        // MTLBlendFactorOneMinusSourceAlpha;
        // ppl_desc.colorAttachments[0].destinationAlphaBlendFactor =
        // MTLBlendFactorOne;

        ppl_desc.vertexFunction = sdf_vert_;
        ppl_desc.fragmentFunction = sdf_frag_;
        sdf_pipeline_ = [device_ newRenderPipelineStateWithDescriptor:ppl_desc
                                                                error:&error];
        spargel_check(!error);

        ppl_desc.fragmentFunction = sdf_frag2_;
        sdf_pipeline2_ = [device_ newRenderPipelineStateWithDescriptor:ppl_desc
                                                                 error:&error];
        spargel_check(!error);
        [ppl_desc release];

        // Compute pipeline
        sdf_comp_pipeline_ =
            [device_ newComputePipelineStateWithFunction:sdf_comp_
                                                   error:&error];
        spargel_check(!error);
        sdf_comp_v2_pipeline_ =
            [device_ newComputePipelineStateWithFunction:sdf_comp_v2_
                                                   error:&error];
        spargel_check(!error);
        sdf_binning_pipeline_ =
            [device_ newComputePipelineStateWithFunction:sdf_binning_
                                                   error:&error];
        spargel_check(!error);

        // Prepare render pass descriptor.
        pass_desc_ = [[MTLRenderPassDescriptor alloc] init];
        pass_desc_.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass_desc_.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass_desc_.colorAttachments[0].clearColor =
            MTLClearColorMake(0, 1, 1, 1);
    }
    void UIRendererMetal::initTexture() {
        auto desc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatA8Unorm
                                         width:ATLAS_SIZE
                                        height:ATLAS_SIZE
                                     mipmapped:false];
        desc.storageMode = MTLStorageModeShared;
        texture_ = [device_ newTextureWithDescriptor:desc];
    }
    id<MTLCommandBuffer> UIRendererMetal::renderToTextureRender(
        UIScene const& scene, id<MTLTexture> texture) {
        auto commands_bytes = scene.commands().asBytes();
        auto data_bytes = scene.data().asBytes();

        scene_commands_buffer_.request(commands_bytes.count());
        scene_data_buffer_.request(data_bytes.count());

        memcpy(scene_commands_buffer_.object.contents, commands_bytes.data(),
               commands_bytes.count());
        memcpy(scene_data_buffer_.object.contents, data_bytes.data(),
               data_bytes.count());
        pass_desc_.colorAttachments[0].texture = texture;

        auto command_buffer = [queue_ commandBuffer];
        auto encoder =
            [command_buffer renderCommandEncoderWithDescriptor:pass_desc_];

        struct {
            u32 cmd_count;
        } uniform_data = {base::checkedConvert<u32>(scene.commands().count())};

        [encoder setRenderPipelineState:sdf_pipeline_];
        [encoder setFragmentBytes:&uniform_data
                           length:sizeof(uniform_data)
                          atIndex:0];

        id<MTLBuffer> buffers[] = {scene_commands_buffer_.object,
                                   scene_data_buffer_.object};
        usize offsets[] = {0, 0};
        [encoder setFragmentBuffers:buffers
                            offsets:offsets
                          withRange:NSMakeRange(1, 2)];
        [encoder setFragmentTexture:texture_ atIndex:0];

        // we use one triangle
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0
                    vertexCount:3];

        [encoder endEncoding];

        return command_buffer;
    }
    id<MTLCommandBuffer> UIRendererMetal::renderToTextureCompute(
        UIScene const& scene, id<MTLTexture> target) {
        auto commands_bytes = scene.commands().asBytes();
        auto data_bytes = scene.data().asBytes();

        scene_commands_buffer_.request(commands_bytes.count());
        scene_data_buffer_.request(data_bytes.count());

        memcpy(scene_commands_buffer_.object.contents, commands_bytes.data(),
               commands_bytes.count());
        memcpy(scene_data_buffer_.object.contents, data_bytes.data(),
               data_bytes.count());

        auto command_buffer = [queue_ commandBuffer];
        auto encoder = [command_buffer computeCommandEncoder];

        struct {
            u32 cmd_count;
        } uniform_data = {base::checkedConvert<u32>(scene.commands().count())};

        [encoder setComputePipelineState:sdf_comp_pipeline_];
        [encoder setBytes:&uniform_data length:sizeof(uniform_data) atIndex:0];

        id<MTLBuffer> buffers[] = {scene_commands_buffer_.object,
                                   scene_data_buffer_.object};
        usize offsets[] = {0, 0};
        [encoder setBuffers:buffers
                    offsets:offsets
                  withRange:NSMakeRange(1, 2)];
        [encoder setTexture:target atIndex:0];
        [encoder setTexture:texture_ atIndex:1];

        [encoder dispatchThreadgroups:MTLSizeMake(target.width / 8 + 1,
                                                  target.height / 8 + 1, 1)
                threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

        [encoder endEncoding];

        return command_buffer;
    }
    struct BinAlloc {
        u32 offset;
        bool out_of_space;
    };
    static_assert(sizeof(BinAlloc) == 8);
    struct BinControl {
        u32 tile_count_x;
        u32 tile_count_y;
        u32 cmd_count;
        u32 max_slot;
    };
    static_assert(sizeof(BinControl) == 16);
    struct BinSlot {
        u32 next_slot;
        u32 command_index;
    };
    static_assert(sizeof(BinSlot) == 8);
    id<MTLCommandBuffer> UIRendererMetal::renderToTextureComputeV2(
        UIScene const& scene, id<MTLTexture> target) {
        usize tile_count_x = target.width / 8 + 1;
        usize tile_count_y = target.height / 8 + 1;
        usize tile_count = tile_count_x * tile_count_y;
        usize command_count = scene.commands2_count();

        auto commands_bytes = scene.commands2_bytes();

        scene_commands_buffer_.request(commands_bytes.count());

        // How to estimate?
        // sizeof(BinSlot) is 8.
        // Let's say each commands intersects 200 tiles on average.
        //
        // Screen size: (say) 4000x2000 px
        // => tile_count = 501 * 251 = 125751
        //
        // Crash:
        //   max_slot = 82711
        //   command_count = 7 (demo_ui)
        //   tile_count = 79211 = 379 * 209
        usize max_slot = command_count * 500 + tile_count;
        bin_slots_buffer_.request(sizeof(BinSlot) * max_slot);
        bin_alloc_buffer_.request(sizeof(BinAlloc));
        // spargel_log_info("max_slot: %zu", max_slot);
        // spargel_log_info("tile_count: %zu", tile_count);

        memcpy(scene_commands_buffer_.object.contents, commands_bytes.data(),
               commands_bytes.count());

        BinAlloc alloc_initial{base::checkedConvert<u32>(tile_count), 0};
        memcpy(bin_alloc_buffer_.object.contents, &alloc_initial,
               sizeof(BinAlloc));

        auto command_buffer = [queue_ commandBuffer];
        auto encoder = [command_buffer computeCommandEncoder];

        // Binning!
        BinControl bin_control{base::checkedConvert<u32>(tile_count_x),
                               base::checkedConvert<u32>(tile_count_y),
                               base::checkedConvert<u32>(command_count),
                               base::checkedConvert<u32>(max_slot)};

        [encoder setComputePipelineState:sdf_binning_pipeline_];
        [encoder setBytes:&bin_control length:sizeof(BinControl) atIndex:0];
        [encoder setBuffer:scene_commands_buffer_.object offset:0 atIndex:1];
        [encoder setBuffer:bin_slots_buffer_.object offset:0 atIndex:2];
        [encoder setBuffer:bin_alloc_buffer_.object offset:0 atIndex:3];

        [encoder dispatchThreadgroups:MTLSizeMake(tile_count_x / 8 + 1,
                                                  tile_count_y / 8 + 1, 1)
                threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
        //[encoder dispatchThreadgroups:MTLSizeMake(1, 1, 1)
        //        threadsPerThreadgroup:MTLSizeMake(tile_count_x, tile_count_y,
        //        1)];

        // Render!

        id<MTLResource> resource = bin_slots_buffer_.object;
        [encoder memoryBarrierWithResources:&resource count:1];

        // struct {
        //     u32 cmd_count;
        // } uniform_data =
        // {base::checkedConvert<u32>(scene.commands().count())};

        [encoder setComputePipelineState:sdf_comp_v2_pipeline_];
        //[encoder setBytes:&uniform_data length:sizeof(uniform_data)
        // atIndex:0]; [encoder setBuffer:scene_commands_buffer_.object offset:0
        // atIndex:1];
        [encoder setTexture:target atIndex:0];
        [encoder setTexture:texture_ atIndex:1];

        [encoder dispatchThreadgroups:MTLSizeMake(tile_count_x, tile_count_y, 1)
                threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

        [encoder endEncoding];

        [command_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
          auto alloc = (BinAlloc*)bin_alloc_buffer_.object.contents;
          if (alloc->out_of_space) {
              spargel_log_warn("bin slots overflow (offset: %u)",
                               alloc->offset);
          }
        }];

        return command_buffer;
    }
    id<MTLCommandBuffer> UIRendererMetal::renderToTextureRender2(
        UIScene const& scene, id<MTLTexture> target) {
        usize tile_count_x = target.width / 8 + 1;
        usize tile_count_y = target.height / 8 + 1;
        usize tile_count = tile_count_x * tile_count_y;
        usize command_count = scene.commands2_count();

        auto commands_bytes = scene.commands2_bytes();

        scene_commands_buffer_.request(commands_bytes.count());

        // How to estimate?
        // sizeof(BinSlot) is 4.
        // Let's say each commands intersects 200 tiles on average.
        //
        // Screen size: (say) 4000x2000 px
        // => tile_count = 501 * 251 = 125751
        //
        // Crash:
        //   max_slot = 82711
        //   command_count = 7 (demo_ui)
        //   tile_count = 79211 = 379 * 209
        // usize max_slot = command_count * 500 + tile_count;
        // TODO: Round before area.
        usize max_slot = scene.estimatedSlots() + tile_count;
        bin_slots_buffer_.request(sizeof(BinSlot) * max_slot);
        bin_alloc_buffer_.request(sizeof(BinAlloc));
        // spargel_log_info("max_slot: %zu", max_slot);
        // spargel_log_info("tile_count: %zu", tile_count);

        memcpy(scene_commands_buffer_.object.contents, commands_bytes.data(),
               commands_bytes.count());

        BinAlloc alloc_initial{base::checkedConvert<u32>(tile_count), 0};
        memcpy(bin_alloc_buffer_.object.contents, &alloc_initial,
               sizeof(BinAlloc));

        auto command_buffer = [queue_ commandBuffer];
        auto encoder = [command_buffer computeCommandEncoder];

        // Binning!
        BinControl bin_control{base::checkedConvert<u32>(tile_count_x),
                               base::checkedConvert<u32>(tile_count_y),
                               base::checkedConvert<u32>(command_count),
                               base::checkedConvert<u32>(max_slot)};

        [encoder setComputePipelineState:sdf_binning_pipeline_];
        [encoder setBytes:&bin_control length:sizeof(BinControl) atIndex:0];
        [encoder setBuffer:scene_commands_buffer_.object offset:0 atIndex:1];
        [encoder setBuffer:bin_slots_buffer_.object offset:0 atIndex:2];
        [encoder setBuffer:bin_alloc_buffer_.object offset:0 atIndex:3];

        [encoder dispatchThreadgroups:MTLSizeMake(tile_count_x / 8 + 1,
                                                  tile_count_y / 8 + 1, 1)
                threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
        //[encoder dispatchThreadgroups:MTLSizeMake(1, 1, 1)
        //        threadsPerThreadgroup:MTLSizeMake(tile_count_x, tile_count_y,
        //        1)];

        [encoder endEncoding];

        // Render!

        pass_desc_.colorAttachments[0].texture = target;
        auto encoder2 =
            [command_buffer renderCommandEncoderWithDescriptor:pass_desc_];

        [encoder2 setRenderPipelineState:sdf_pipeline2_];
        [encoder2 setFragmentBytes:&bin_control
                            length:sizeof(BinControl)
                           atIndex:0];
        [encoder2 setFragmentBuffer:scene_commands_buffer_.object
                             offset:0
                            atIndex:1];
        [encoder2 setFragmentBuffer:bin_slots_buffer_.object
                             offset:0
                            atIndex:2];
        [encoder2 setFragmentTexture:texture_ atIndex:0];

        // we use one triangle
        [encoder2 drawPrimitives:MTLPrimitiveTypeTriangle
                     vertexStart:0
                     vertexCount:3];
        [encoder2 endEncoding];

        [command_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
          auto alloc = (BinAlloc*)bin_alloc_buffer_.object.contents;
          if (alloc->out_of_space) {
              spargel_log_warn("bin slots overflow (tiles: %zu, cmds: %zu, "
                               "max: %zu, need: %u)",
                               tile_count, command_count, max_slot,
                               alloc->offset);
          }
        }];

        return command_buffer;
    }
    id<MTLCommandBuffer> UIRendererMetal::renderToTexture(
        UIScene const& scene, id<MTLTexture> texture) {
        if (use_compute) {
            return renderToTextureComputeV2(scene, texture);
            // return renderToTextureCompute(scene, texture);
        }
        // return renderToTextureRender(scene, texture);
        return renderToTextureRender2(scene, texture);
    }
    void UIRendererMetal::render(UIScene const& scene) {
        spargel_check(layer_);
        auto drawable = [layer_ nextDrawable];

        auto command_buffer = renderToTexture(scene, drawable.texture);

        [command_buffer presentDrawable:drawable];
        [command_buffer commit];

        [command_buffer waitUntilCompleted];

        // auto dt = command_buffer.GPUEndTime - command_buffer.GPUStartTime;
        // spargel_log_info("GPU time: %.3fms", dt * 1000);
    }
    UIRendererMetal::~UIRendererMetal() {
        [pass_desc_ release];
        [sdf_pipeline_ release];
        [sdf_frag_ release];
        [sdf_vert_ release];
        [library_ release];
    }
    namespace {
        // Get the current time in nanoseconds.
        //
        // NOTE: This is not portable.
        // Both `clock_gettime_nsec_np` and `CLOCK_UPTIME_RAW` are Darwin
        // extensions.
        u64 getTimestamp() { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW); }
    }  // namespace
    id<MTLBuffer> UIRendererMetal::BufferPool::request(usize length) {
        auto now = getTimestamp();
        if (now - last_purged > 1'000'000'000) {
            purge(now);
        }
        base::Optional<BufferItem> candidate = base::nullopt;
        for (auto const& item : buffers) {
            if (!item.in_use && item.object.length >= length &&
                item.olderThan(candidate)) {
                candidate = item;
            }
        }
        if (candidate.hasValue()) {
            candidate.value().last_used = now;
            candidate.value().in_use = true;
            return candidate.value().object;
        }
        auto buffer = context->createBuffer(length);
        buffers.emplace(buffer, now, true);
        return buffer;
    }
    void UIRendererMetal::BufferPool::purge(u64 now) {
        buffers.eraseIfFastWithDeleter(
            [this](BufferItem const& item) {
                return !item.in_use && item.last_used > last_purged;
            },
            [this](BufferItem const& item) {
                context->destroyBuffer(item.object);
            });
        last_purged = now;
    }
    void UIRendererMetal::BufferPool::putBack(id<MTLBuffer> buffer) {
        for (auto& item : buffers) {
            if (item.object == buffer) {
                item.in_use = false;
                break;
            }
        }
    }
    bool UIRendererMetal::BufferItem::olderThan(
        base::Optional<BufferItem> const& other) const {
        if (!other.hasValue()) {
            return true;
        }
        return last_used < other.value().last_used;
    }
    UIRendererMetal::GrowingBuffer::~GrowingBuffer() {
        // spargel_log_info("buffer destructor");
    }
    void UIRendererMetal::GrowingBuffer::request(usize length) {
        if (length == 0) {
            return;
        }
        if (!object) {
            object = context->createBuffer(length);
            return;
        }
        if (length <= object.length) {
            return;
        }
        context->destroyBuffer(object);
        object = context->createBuffer(length);
    }
    void UIRendererMetal::uploadBitmap(TextureHandle handle,
                                       text::Bitmap const& bitmap) {
        [texture_ replaceRegion:MTLRegionMake2D(handle.x, handle.y,
                                                handle.width, handle.height)
                    mipmapLevel:0
                      withBytes:bitmap.data.data()
                    bytesPerRow:bitmap.width];
    }
}  // namespace spargel::render
