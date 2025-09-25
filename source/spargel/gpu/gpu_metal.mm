#include "spargel/gpu/gpu_metal.h"
#include "spargel/base/unique_ptr.h"
#include "spargel/gpu/metal_shader_manager.h"
#include "spargel/logging/logging.h"

// metal
#import <Metal/Metal.h>

namespace spargel::gpu {

    namespace {

        // MTLCullMode translate_cull_mode(cull_mode mode) {
        //     using enum cull_mode;
        //     switch (mode) {
        //     case none:
        //         return MTLCullModeNone;
        //     case front:
        //         return MTLCullModeFront;
        //     case back:
        //         return MTLCullModeBack;
        //     }
        // }

        // MTLWinding translate_orientation(orientation o) {
        //     using enum orientation;
        //     switch (o) {
        //     case clockwise:
        //         return MTLWindingClockwise;
        //     case counter_clockwise:
        //         return MTLWindingCounterClockwise;
        //     }
        // }

        MTLPrimitiveTopologyClass translatePrimitiveType(PrimitiveKind kind) {
            using enum PrimitiveKind;
            switch (kind) {
            case point:
                return MTLPrimitiveTopologyClassPoint;
            case line:
                return MTLPrimitiveTopologyClassLine;
            case triangle:
                return MTLPrimitiveTopologyClassTriangle;
            }
        }

        MTLVertexStepFunction translateStepMode(VertexStepMode mode) {
            using enum VertexStepMode;
            switch (mode) {
            case vertex:
                return MTLVertexStepFunctionPerVertex;
            case instance:
                return MTLVertexStepFunctionPerInstance;
            }
        }

        MTLVertexFormat translateVertexFormat(VertexAttributeFormat format) {
            using enum VertexAttributeFormat;
            switch (format) {
            case float32x2:
                return MTLVertexFormatFloat2;
            case float32x4:
                return MTLVertexFormatFloat4;
            default:
                spargel_panic_here();
            }
        }

        MTLLoadAction translate_load_action(LoadAction action) {
            using enum LoadAction;
            switch (action) {
            case load:
                return MTLLoadActionLoad;
            case clear:
                return MTLLoadActionClear;
            case dont_care:
                return MTLLoadActionDontCare;
            }
        }

        MTLStoreAction translate_store_action(StoreAction action) {
            using enum StoreAction;
            switch (action) {
            case store:
                return MTLStoreActionStore;
            case dont_care:
                return MTLStoreActionDontCare;
            }
        }

        MTLPixelFormat translatePixelFormat(TextureFormat format) {
            using enum TextureFormat;
            switch (format) {
            case bgra8_unorm:
                return MTLPixelFormatBGRA8Unorm;
            default:
                spargel_panic_here();
            }
        }

        MTLResourceUsage translateBufferAccess(BufferAccess access) {
            MTLResourceUsage result = 0;
#define _TRANSLATE(x, y)               \
    if (access.has(BufferAccess::x)) { \
        result |= y;                   \
    }
            _TRANSLATE(read, MTLResourceUsageRead);
            _TRANSLATE(write, MTLResourceUsageWrite);
#undef _TRANSLATE
            return result;
        }

    }  // namespace

    base::unique_ptr<Device> makeMetalDevice() {
        return base::make_unique<DeviceMetal>();
    }

    DeviceMetal::DeviceMetal() {
        _device = MTLCreateSystemDefaultDevice();  // ns_returns_retained
        [_device retain];
    }

    DeviceMetal::~DeviceMetal() { [_device release]; }

    ShaderLibrary* DeviceMetal::createShaderLibrary(base::span<u8> bytes) {
        // dispatch create a copy of the data
        //
        // notes:
        // This object is retained initially. It is your responsibility to
        // release the data object when you are done using it.
        //
        auto library_data = dispatch_data_create(
            bytes.data(), bytes.count(), dispatch_get_main_queue(),
            DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        NSError* error;
        auto library = [_device newLibraryWithData:library_data error:&error];
        if (library == nil) {
            return nullptr;
        }
        return new ShaderLibraryMetal(library_data, library);
    }

    RenderPipeline* DeviceMetal::createRenderPipeline(
        RenderPipelineDescriptor const& descriptor) {
        auto desc = [[MTLRenderPipelineDescriptor alloc] init];

        auto vertex_library = static_cast<ShaderLibraryMetal*>(
            descriptor.vertex_shader->library());
        auto vertex_entry = descriptor.vertex_shader->entry();

        auto vertex_function = [vertex_library->library()
            newFunctionWithName:[NSString stringWithUTF8String:vertex_entry]];

        auto vertex_descriptor = [MTLVertexDescriptor vertexDescriptor];

        for (usize i = 0; i < descriptor.vertex_buffers.count(); i++) {
            vertex_descriptor.layouts[i].stepFunction =
                translateStepMode(descriptor.vertex_buffers[i].step_mode);
            vertex_descriptor.layouts[i].stride =
                descriptor.vertex_buffers[i].stride;
        }

        for (usize i = 0; i < descriptor.vertex_attributes.count(); i++) {
            vertex_descriptor.attributes[i].format =
                translateVertexFormat(descriptor.vertex_attributes[i].format);
            vertex_descriptor.attributes[i].offset =
                descriptor.vertex_attributes[i].offset;
            // TODO: fix
            vertex_descriptor.attributes[i].bufferIndex =
                descriptor.vertex_attributes[i].buffer;
        }

        desc.vertexFunction = vertex_function;

        for (usize i = 0; i < descriptor.color_attachments.count(); i++) {
            desc.colorAttachments[i].pixelFormat =
                translatePixelFormat(descriptor.color_attachments[i].format);
            desc.colorAttachments[i].blendingEnabled =
                descriptor.color_attachments[i].enable_blend;
            desc.colorAttachments[i].alphaBlendOperation = MTLBlendOperationAdd;
            desc.colorAttachments[i].rgbBlendOperation = MTLBlendOperationAdd;
            desc.colorAttachments[i].sourceRGBBlendFactor =
                MTLBlendFactorSourceAlpha;
            desc.colorAttachments[i].sourceAlphaBlendFactor = MTLBlendFactorOne;
            desc.colorAttachments[i].destinationRGBBlendFactor =
                MTLBlendFactorOneMinusSourceAlpha;
            desc.colorAttachments[i].destinationAlphaBlendFactor =
                MTLBlendFactorOne;
        }

        desc.vertexDescriptor = vertex_descriptor;
        desc.inputPrimitiveTopology =
            translatePrimitiveType(descriptor.primitive);

        auto frag_library = static_cast<ShaderLibraryMetal*>(
            descriptor.fragment_shader->library());
        auto frag_entry = descriptor.fragment_shader->entry();

        auto frag_function = [frag_library->library()
            newFunctionWithName:[NSString stringWithUTF8String:frag_entry]];

        for (MTLAttribute* attr in frag_function.stageInputAttributes) {
            NSLog(@"%@", attr);
        }

        desc.fragmentFunction = frag_function;

        NSError* error;

        auto state = [_device newRenderPipelineStateWithDescriptor:desc
                                                             error:&error];

        [desc release];

        return new RenderPipelineMetal(vertex_function, state);
    }

    Buffer* DeviceMetal::createBuffer([[maybe_unused]] BufferUsage usage,
                                      base::span<u8> bytes) {
        auto buf = [_device newBufferWithBytes:bytes.data()
                                        length:bytes.count()
                                       options:MTLResourceStorageModeShared];
        return new BufferMetal(buf);
    }

    Buffer* DeviceMetal::createBuffer([[maybe_unused]] BufferUsage usage,
                                      u32 size) {
        auto buf = [_device newBufferWithLength:size
                                        options:MTLResourceStorageModeShared];
        return new BufferMetal(buf);
    }

    // Surface* DeviceMetal::createSurface(ui::Window* w) {
    //     CAMetalLayer* l = static_cast<ui::WindowAppKit*>(w)->metalLayer();
    //     l.device = _device;
    //     return new SurfaceMetal(l);
    // }

    void DeviceMetal::destroyShaderLibrary(ShaderLibrary* l) {
        auto library = static_cast<ShaderLibraryMetal*>(l);
        delete library;
    }

    void DeviceMetal::destroyRenderPipeline(RenderPipeline* p) {
        auto pipeline = static_cast<RenderPipelineMetal*>(p);
        delete pipeline;
    }

    void DeviceMetal::destroyBuffer(Buffer* b) {
        auto buf = static_cast<BufferMetal*>(b);
        delete buf;
    }

    CommandQueue* DeviceMetal::createCommandQueue() {
        return new CommandQueueMetal([_device newCommandQueue]);
    }

    void DeviceMetal::destroyCommandQueue(CommandQueue* q) {
        delete static_cast<CommandQueueMetal*>(q);
    }

    Texture* DeviceMetal::createTexture([[maybe_unused]] u32 width,
                                        [[maybe_unused]] u32 height) {
        spargel_panic_here();
        // auto desc = [MTLTextureDescriptor
        // texture2DDescriptorWithPixelFormat:MTLPixelFormatA8Unorm
        //                                                                width:width
        //                                                               height:height
        //                                                            mipmapped:NO];
        // desc.storageMode = MTLStorageModeShared;
        // auto texture = [_device newTextureWithDescriptor:desc];
        // return new TextureMetal(texture);
    }

    Texture* DeviceMetal::createMonochromeTexture(u32 width, u32 height) {
        auto desc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatA8Unorm
                                         width:width
                                        height:height
                                     mipmapped:NO];
        desc.storageMode = MTLStorageModeShared;
        auto texture = [_device newTextureWithDescriptor:desc];
        return new TextureMetal(texture);
    }

    void DeviceMetal::destroyTexture([[maybe_unused]] Texture* texture) {
        // todo
    }

    ShaderLibraryMetal::ShaderLibraryMetal(dispatch_data_t data,
                                           id<MTLLibrary> library)
        : _data{data}, _library{library} {}

    ShaderLibraryMetal::~ShaderLibraryMetal() {
        [_library release];
        [_data release];
    }

    RenderPipelineMetal::RenderPipelineMetal(
        id<MTLFunction> vertex, id<MTLRenderPipelineState> pipeline)
        : _vertex{vertex}, _pipeline{pipeline} {}

    RenderPipelineMetal::~RenderPipelineMetal() {
        [_pipeline release];
        [_vertex release];
    }

    BufferMetal::BufferMetal(id<MTLBuffer> b) : _buffer{b} {}

    BufferMetal::~BufferMetal() { [_buffer release]; }

    CommandBuffer* CommandQueueMetal::createCommandBuffer() {
        return new CommandBufferMetal([_queue commandBuffer]);
    }

    void CommandQueueMetal::destroyCommandBuffer(CommandBuffer* cmdbuf) {
        delete static_cast<CommandBufferMetal*>(cmdbuf);
    }

    RenderPassEncoder* CommandBufferMetal::beginRenderPass(
        RenderPassDescriptor const& descriptor) {
        auto desc = [MTLRenderPassDescriptor renderPassDescriptor];
        for (usize i = 0; i < descriptor.color_attachments.count(); i++) {
            desc.colorAttachments[i].texture =
                static_cast<TextureMetal*>(
                    descriptor.color_attachments[i].texture)
                    ->texture();
            desc.colorAttachments[i].loadAction =
                translate_load_action(descriptor.color_attachments[i].load);
            desc.colorAttachments[i].storeAction =
                translate_store_action(descriptor.color_attachments[i].store);
            auto& c = descriptor.color_attachments[i].clear_color;
            desc.colorAttachments[i].clearColor =
                MTLClearColorMake(c.r, c.g, c.b, c.a);
        }
        return new RenderPassEncoderMetal(
            [_cmdbuf renderCommandEncoderWithDescriptor:desc]);
    }

    void CommandBufferMetal::endRenderPass(RenderPassEncoder* e) {
        auto encoder = static_cast<RenderPassEncoderMetal*>(e);
        [encoder->encoder() endEncoding];
        delete encoder;
    }

    // void CommandBufferMetal::present(Surface* s) {
    // [_cmdbuf presentDrawable:static_cast<SurfaceMetal*>(s)->drawable()];
    // }

    void CommandBufferMetal::submit() { [_cmdbuf commit]; }

    ComputePassEncoder* CommandBufferMetal::beginComputePass() {
        return new ComputePassEncoderMetal([_cmdbuf computeCommandEncoder]);
    }

    void CommandBufferMetal::endComputePass(ComputePassEncoder* e) {
        auto encoder = static_cast<ComputePassEncoderMetal*>(e);
        [encoder->encoder() endEncoding];
        delete encoder;
    }

    void RenderPassEncoderMetal::setRenderPipeline(RenderPipeline* p) {
        [_encoder setRenderPipelineState:static_cast<RenderPipelineMetal*>(p)
                                             ->pipeline()];
    }
    void RenderPassEncoderMetal::setVertexBuffer(
        Buffer* b, VertexBufferLocation const& loc) {
        [_encoder setVertexBuffer:static_cast<BufferMetal*>(b)->buffer()
                           offset:0
                          atIndex:(NSUInteger)loc.apple.buffer_index];
    }
    void RenderPassEncoderMetal::setTexture(Texture* texture) {
        // todo: index
        [_encoder
            setFragmentTexture:static_cast<TextureMetal*>(texture)->texture()
                       atIndex:0];
    }
    void RenderPassEncoderMetal::setViewport(Viewport v) {
        [_encoder setViewport:{v.x, v.y, v.width, v.height, v.z_near, v.z_far}];
    }
    void RenderPassEncoderMetal::draw(int vertex_start, int vertex_count) {
        [_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                     vertexStart:(NSUInteger)vertex_start
                     vertexCount:(NSUInteger)vertex_count];
    }
    void RenderPassEncoderMetal::draw(int vertex_start, int vertex_count,
                                      int instance_start, int instance_count) {
        [_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                     vertexStart:(NSUInteger)vertex_start
                     vertexCount:(NSUInteger)vertex_count
                   instanceCount:(NSUInteger)instance_count
                    baseInstance:(NSUInteger)instance_start];
    }

    void TextureMetal::updateRegion(u32 x, u32 y, u32 width, u32 height,
                                    u32 bytes_per_row,
                                    base::span<base::Byte> bytes) {
        [_texture replaceRegion:MTLRegionMake2D(x, y, width, height)
                    mipmapLevel:0
                      withBytes:bytes.data()
                    bytesPerRow:bytes_per_row];
    }

    void ComputePassEncoderMetal::setBindGroup(u32 index, BindGroup* g) {
        auto group = static_cast<BindGroupMetal*>(g);
        [_encoder setBuffer:group->buffer()
                     offset:0
                    atIndex:group->program()->getBufferId(index)];
    }

    void ComputePassEncoderMetal::setComputePipeline2(
        ComputePipeline2* pipeline) {
        [_encoder setComputePipelineState:static_cast<ComputePipeline2Metal*>(
                                              pipeline)
                                              ->pipeline()];
    }

    void ComputePassEncoderMetal::useBuffer(Buffer* buffer,
                                            BufferAccess access) {
        [_encoder useResource:static_cast<BufferMetal*>(buffer)->buffer()
                        usage:translateBufferAccess(access)];
    }

    RenderPipeline2* DeviceMetal::createRenderPipeline2(
        RenderPipeline2Descriptor const& descriptor) {
        auto desc = [[MTLRenderPipelineDescriptor alloc] init];

        auto vertex_library = static_cast<ShaderLibraryMetal*>(
            descriptor.vertex_shader->library());
        auto vertex_entry = descriptor.vertex_shader->entry();

        auto vertex_function = [vertex_library->library()
            newFunctionWithName:[NSString stringWithUTF8String:vertex_entry]];

        auto vertex_descriptor = [MTLVertexDescriptor vertexDescriptor];

        for (usize i = 0; i < descriptor.vertex_buffers.count(); i++) {
            vertex_descriptor.layouts[i].stepFunction =
                translateStepMode(descriptor.vertex_buffers[i].step_mode);
            vertex_descriptor.layouts[i].stride =
                descriptor.vertex_buffers[i].stride;
        }

        for (usize i = 0; i < descriptor.vertex_attributes.count(); i++) {
            vertex_descriptor.attributes[i].format =
                translateVertexFormat(descriptor.vertex_attributes[i].format);
            vertex_descriptor.attributes[i].offset =
                descriptor.vertex_attributes[i].offset;
            // TODO: fix
            vertex_descriptor.attributes[i].bufferIndex =
                descriptor.vertex_attributes[i].buffer;
        }

        desc.vertexFunction = vertex_function;

        for (usize i = 0; i < descriptor.color_attachments.count(); i++) {
            desc.colorAttachments[i].pixelFormat =
                translatePixelFormat(descriptor.color_attachments[i].format);
            desc.colorAttachments[i].blendingEnabled =
                descriptor.color_attachments[i].enable_blend;
            desc.colorAttachments[i].alphaBlendOperation = MTLBlendOperationAdd;
            desc.colorAttachments[i].rgbBlendOperation = MTLBlendOperationAdd;
            desc.colorAttachments[i].sourceRGBBlendFactor =
                MTLBlendFactorSourceAlpha;
            desc.colorAttachments[i].sourceAlphaBlendFactor = MTLBlendFactorOne;
            desc.colorAttachments[i].destinationRGBBlendFactor =
                MTLBlendFactorOneMinusSourceAlpha;
            desc.colorAttachments[i].destinationAlphaBlendFactor =
                MTLBlendFactorOne;
        }

        desc.vertexDescriptor = vertex_descriptor;
        desc.inputPrimitiveTopology =
            translatePrimitiveType(descriptor.primitive);

        auto frag_library = static_cast<ShaderLibraryMetal*>(
            descriptor.fragment_shader->library());
        auto frag_entry = descriptor.fragment_shader->entry();

        auto frag_function = [frag_library->library()
            newFunctionWithName:[NSString stringWithUTF8String:frag_entry]];

        for (MTLAttribute* attr in frag_function.stageInputAttributes) {
            NSLog(@"%@", attr);
        }

        desc.fragmentFunction = frag_function;

        NSError* error;

        auto state = [_device newRenderPipelineStateWithDescriptor:desc
                                                             error:&error];

        [desc release];

        return new RenderPipeline2Metal(vertex_function, frag_function, state,
                                        descriptor.groups);
    }

    void ComputePassEncoderMetal::setComputePipeline(
        ComputePipeline* pipeline) {
        [_encoder
            setComputePipelineState:static_cast<ComputePipelineMetal*>(pipeline)
                                        ->pipeline()];
    }

    id<MTLLibrary> DeviceMetal::loadLibrary(base::StringView path) {
        auto* lib = libraries_.get(path);
        if (lib != nullptr) {
            return *lib;
        }
        return nullptr;
    }

    ShaderFunction* DeviceMetal::createShaderFunction(
        base::StringView shader_id) {
        auto& manager = MetalShaderManager::instance();
        auto result = manager.queryShader(shader_id);
        if (result.is_error()) {
            logging::info("shader `{}` is not registered", shader_id);
            return nullptr;
        }
        auto [path, meta] = result.value();
        auto* lib = loadLibrary(path);
        (void)(meta);
        (void)(lib);
        return nullptr;
    }
    void DeviceMetal::destroyShaderFunction(ShaderFunction* shader) {
        delete shader;
    }
}  // namespace spargel::gpu
