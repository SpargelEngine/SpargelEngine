#pragma once

#include "spargel/base/functional.h"
#include "spargel/base/span.h"
#include "spargel/base/types.h"
#include "spargel/base/unique_ptr.h"
#include "spargel/codec/codec.h"

// Forward declarations.
namespace spargel::gpu {
    class BindGroup;
    class BindGroupLayout;
    class Buffer;
    class CommandBuffer;
    class CommandQueue;
    class ComputePassEncoder;
    class ComputePipeline;
    class ComputePipeline2;
    class Device;
    class RenderPassEncoder;
    class RenderPipeline;
    class RenderPipeline2;
    class ShaderLibrary;
    class ShaderFunction;
    // class Surface;
    class Texture;
}  // namespace spargel::gpu

namespace spargel::gpu {

    // Enums
    // -----

    enum class TextureFormat {
        a8_unorm,
        bgra8_unorm,
        bgra8_srgb,
    };

    enum class LoadAction {
        dont_care,
        load,
        clear,
    };

    enum class StoreAction {
        dont_care,
        store,
    };

    enum class DeviceKind {
        directx,
        metal,
        vulkan,
        unknown,
    };

    enum class CullMode {
        none,
        // Front-facing triangles are discarded.
        front,
        // Back-facing triangles are discarded.
        back,
    };

    // The front-facing orientation.
    enum class Orientation {
        clockwise,
        counter_clockwise,
    };

    enum class PrimitiveKind {
        point,
        line,
        triangle,
    };

    // The rate of fetching vertex attributes.
    enum class VertexStepMode {
        // Attributes are fetched per vertex.
        vertex,
        // Attributes are fetched per instance.
        instance,
    };

    enum class VertexAttributeFormat {
        float32,
        float32x2,
        float32x4,
    };

    enum class BlendAction {
        add,
    };

    enum class BlendFactor {
        zero,
        one,
        src_color,
        dst_color,
        one_minus_src_color,
        one_minus_dst_color,
        src_alpha,
        dst_alpha,
        one_minus_src_alpha,
        one_minus_dst_alpha,
    };

    enum class DepthCompare {
        never,
        less,
        equal,
        less_equal,
        greater,
        not_equal,
        greater_equal,
        always,
    };

    // NOTE:
    // Resources (except render targets) are read-only on GPU before Shader
    // Model 5. UAV (unordered access view) is introduced in Shader Model 5 for
    // writing/reading from GPU threads (in an unordered way).
    enum class BindEntryKind {
        uniform_buffer,
        storage_buffer,
        sample_texture,
        storage_texture,
    };

    // Bitflags
    // --------

    struct BufferUsage {
        // map and read
        static const BufferUsage map_read;
        // map and write
        static const BufferUsage map_write;
        // the source of a copy command
        static const BufferUsage copy_src;
        // the destination of a copy command
        static const BufferUsage copy_dst;
        // index buffer
        static const BufferUsage index;
        // vertex buffer
        static const BufferUsage vertex;
        // uniform buffer
        static const BufferUsage uniform;
        // storage buffer
        static const BufferUsage storage;
        // draw indirect buffer
        static const BufferUsage indirect;

        constexpr bool has(BufferUsage usage) const {
            return (value & usage.value) != 0;
        }

        constexpr BufferUsage operator|(BufferUsage other) const {
            return BufferUsage{value | other.value};
        }

        u32 value;
    };
    inline constexpr BufferUsage BufferUsage::map_read{1 << 0};
    inline constexpr BufferUsage BufferUsage::map_write{1 << 1};
    inline constexpr BufferUsage BufferUsage::copy_src{1 << 2};
    inline constexpr BufferUsage BufferUsage::copy_dst{1 << 3};
    inline constexpr BufferUsage BufferUsage::index{1 << 4};
    inline constexpr BufferUsage BufferUsage::vertex{1 << 5};
    inline constexpr BufferUsage BufferUsage::uniform{1 << 5};
    inline constexpr BufferUsage BufferUsage::storage{1 << 6};
    inline constexpr BufferUsage BufferUsage::indirect{1 << 7};

    struct ShaderStage {
        static const ShaderStage vertex;
        static const ShaderStage fragment;
        static const ShaderStage compute;
        constexpr bool has(ShaderStage usage) const {
            return (value & usage.value) != 0;
        }
        friend constexpr bool operator==(ShaderStage lhs, ShaderStage rhs) {
            return lhs.value == rhs.value;
        }
        u32 value;
    };
    inline constexpr ShaderStage ShaderStage::vertex{1 << 0};
    inline constexpr ShaderStage ShaderStage::fragment{1 << 1};
    inline constexpr ShaderStage ShaderStage::compute{1 << 2};

    struct BufferAccess {
        static const BufferAccess read;
        static const BufferAccess write;
        static const BufferAccess read_write;
        constexpr bool has(BufferAccess access) const {
            return (value & access.value) != 0;
        }
        friend constexpr bool operator==(BufferAccess lhs, BufferAccess rhs) {
            return lhs.value == rhs.value;
        }
        u32 value;
    };
    inline constexpr BufferAccess BufferAccess::read{1 << 0};
    inline constexpr BufferAccess BufferAccess::write{1 << 1};
    inline constexpr BufferAccess BufferAccess::read_write{(1 << 0) | (1 << 1)};

    // Structs

    struct Viewport {
        float x;
        float y;
        float width;
        float height;
        float z_near;
        float z_far;
    };

    struct VertexBufferLocation {
        struct {
            int buffer_index;
        } apple;
        struct {
            int vertex_buffer_index;
        } vulkan;
    };

    struct ClearColor {
        float r;
        float g;
        float b;
        float a;
    };

    struct DispatchSize {
        u32 x;
        u32 y;
        u32 z;
    };

    // struct ShaderFunction {
    //     ShaderLibrary* library;
    //     char const* entry;
    // };

    // One entry of a bind group.
    struct BindEntry {
        // The unique identifier of this entry.
        //
        // Syntax in shaders:
        // - glsl: `location (set = 0, binding = 1) ...`
        // - msl: `struct ArgBuf { float* buf [[id(1)]]; };`
        //
        u32 binding;

        // The kind of this entry.
        BindEntryKind kind;
    };

    // Descriptors

    struct ShaderLibraryDescriptor {
        base::span<base::Byte> bytes;
    };

    // Description of one vertex attribute.
    struct VertexAttributeDescriptor {
        // @brief index of the vertex buffer where the data of the attribute is
        // fetched
        u32 buffer;

        // @brief the location of the vertex attribute
        //
        // This is called input element in directx.
        //
        // Synatx in shaders:
        // - glsl: `layout (location = 0) in vec3 position;`
        // - msl: `float3 position [[attribute(0)]];`
        // - hlsl: this is specified via semantic name and semantic index.
        //
        // Note:
        // - Both dawn and wgpu use dummy SemanticName, and use SemanticIndex
        // for location.
        // - Dawn chooses "TEXCOORD", and wgpu/naga uses "LOC".
        // - SPIRV-Cross uses "TEXCOORD" by default, and provides the options
        // for remapping.
        // - dxc relies on user declaration `[[vk::location(0)]]`.
        //
        u32 location;

        // @brief the format of the vertex attribute
        VertexAttributeFormat format;

        // @brief the offset of the vertex attribute to the start of the vertex
        // data
        usize offset;
    };

    // @brief description of one vertex buffer
    struct VertexBufferDescriptor {
        // @brief the number of bytes between consecutive elements in the buffer
        usize stride;

        // @brief the rate of fetching vertex attributes
        VertexStepMode step_mode;

        // TODO: step rate is not supported by vulkan.
    };

    struct ColorAttachmentBindDescriptor {
        Texture* texture;
        LoadAction load = LoadAction::clear;
        StoreAction store = StoreAction::store;
        ClearColor clear_color;
    };

    struct ColorAttachmentDescriptor {
        TextureFormat format;
        bool enable_blend = false;
    };

    // @brief The description of a compute pipeline.
    //
    // Most information should be automatically generated using reflection.
    //
    struct ComputePipelineDescriptor {
        ShaderFunction* shader;
    };

    struct RenderPassDescriptor {
        base::span<ColorAttachmentBindDescriptor> color_attachments;
    };

    // @brief description of a render pipeline
    struct RenderPipelineDescriptor {
        // @brief the geometric primitive for this render pipeline
        PrimitiveKind primitive = PrimitiveKind::triangle;

        // @brief the orientation of front-facing triangles
        Orientation front_face = Orientation::counter_clockwise;

        // @brief which triangles to discard
        CullMode cull = CullMode::none;

        // @brief the vertex function
        ShaderFunction* vertex_shader;

        // @brief the vertex buffers used in this pipeline
        base::span<VertexBufferDescriptor> vertex_buffers;

        // @brief the vertex attributes used in this pipeline
        base::span<VertexAttributeDescriptor> vertex_attributes;

        // @brief the fragment function
        ShaderFunction* fragment_shader;

        base::span<ColorAttachmentDescriptor> color_attachments;
    };

    struct BindGroupEntry {
        u32 id;
        BindEntryKind kind;
        char const* name = nullptr;
    };

    struct PipelineArgumentGroup {
        u32 location;
        base::span<BindGroupEntry> arguments;
        char const* name = nullptr;
    };
    struct PipelineStage {
        ShaderFunction* shader;
        base::Span<PipelineArgumentGroup> groups;
    };

    struct PipelinePart {
        ShaderStage stage;
        ShaderFunction* func;
    };

    struct ComputePipeline2Descriptor {
        PipelineStage compute_stage;
        char const* name = nullptr;
    };

    struct RenderPipeline2Descriptor {
        // @brief the geometric primitive for this render pipeline
        PrimitiveKind primitive = PrimitiveKind::triangle;

        // @brief the orientation of front-facing triangles
        Orientation front_face = Orientation::counter_clockwise;

        // @brief which triangles to discard
        CullMode cull = CullMode::none;

        // @brief the vertex function
        ShaderFunction* vertex_shader;

        // @brief the vertex buffers used in this pipeline
        base::span<VertexBufferDescriptor> vertex_buffers;

        // @brief the vertex attributes used in this pipeline
        base::span<VertexAttributeDescriptor> vertex_attributes;

        // @brief the fragment function
        ShaderFunction* fragment_shader;

        base::span<ColorAttachmentDescriptor> color_attachments;

        base::span<PipelineArgumentGroup> groups;
    };

    struct BindEntryKindCodec {
        using TargetType = BindEntryKind;

        template <codec::DecodeBackend DB>
        base::Either<TargetType, codec::ErrorType<DB>> decode(
            DB& backend, const codec::DataType<DB>& data) const {
            auto maybe_str = backend.getString(data);
            if (maybe_str.isRight()) {
                auto& err = maybe_str.right();
                err.append(base::StringView{"BindEntryKind expects a string."});
                return base::Right<codec::ErrorType<DB>>(base::move(err));
            }
            auto& str = maybe_str.left();
            if (str == base::StringView{"uniform_buffer"}) {
                return base::Left{BindEntryKind::uniform_buffer};
            }
            return base::Right<codec::ErrorType<DB>>(
                base::StringView{"Unknown bind entry kind."});
        }

        template <codec::EncodeBackend EB>
        base::Either<codec::DataType<EB>, codec::ErrorType<EB>> encode(
            EB&, TargetType) const {
            return base::Right<codec::ErrorType<EB>>(
                base::StringView{"<TODO>"});
        }
    };

    //
    struct BindTableEntry {
        u32 id;
        BindEntryKind kind;
        // debug
        char const* name;

        static constexpr auto CODEC = codec::makeRecordCodec<BindTableEntry>(
            base::Constructor<BindTableEntry>{},
            codec::makeNormalField("id", codec::U32Codec{},
                                   &BindTableEntry::id),
            codec::makeNormalField("kind", BindEntryKindCodec{},
                                   &BindTableEntry::kind));
    };

    // Interfaces
    // ----------

    // A logical GPU device.
    class Device {
    public:
        static base::UniquePtr<Device> create(DeviceKind kind);

        virtual ~Device() = default;

        // Command submission
        // ------------------

        // Create a command queue.
        virtual CommandQueue* createCommandQueue() = 0;
        // Destroy the command buffer.
        virtual void destroyCommandQueue(CommandQueue* queue) = 0;

        // Allocate a command buffer.
        //
        // Vulkan (and Metal 3) requires that every command buffer can
        // be submitted only to the queue it was created with.
        // This restrction is removed in Metal 4.
        //
        // TODO:
        // The strategy here is to ask the user for the usage of the
        // command buffer, e.g. rendering or async compute.
        //
        // TODO: Batch api.
        virtual CommandBuffer* createCommandBuffer() = 0;
        virtual void destroyCommandBuffer(CommandBuffer* buffer) = 0;

        // Shader objects
        // --------------

        // TODO: Review the design of shader objects.
        //
        // Metal shaders are linked together (but still they can be separated on
        // the source level). Vulkan spec allows multiple entries in a spirv
        // file. Note that SPIRV-link is not stable. Mesa supports using
        // multiple entry points in one spirv file, see
        // <<@mesa//src/compiler/spirv/nir_spirv.h>>.
        // TODO: Investigate other implementations.

        // Create a shader library (consisting of several shader functions) from
        // the given shader data.
        //
        // NOTE: The data is backend dependent (i.e. spirv, or metallib, or
        // dxir).
        virtual ShaderLibrary* createShaderLibrary(
            base::Span<base::Byte> bytes) = 0;
        // Destroy the shader library.
        virtual void destroyShaderLibrary(ShaderLibrary* library) = 0;

        // The new design.
        //
        // Shaders are created using their IDs, instead of passing bytes. Each
        // backend querys `ShaderManager` for specific data.
        virtual ShaderFunction* createShaderFunction(
            base::StringView shader_id) = 0;
        virtual void destroyShaderFunction(ShaderFunction* shader) = 0;

        // Pipeline objects
        // ----------------

        // Create a render pipeline
        //
        // TODO: Batch api.
        virtual RenderPipeline* createRenderPipeline(
            RenderPipelineDescriptor const& descriptor) = 0;
        // Create a render pipeline. Use BindGroup.
        //
        // TODO: Batch api.
        virtual RenderPipeline2* createRenderPipeline2(
            RenderPipeline2Descriptor const& desc) = 0;
        // Destroy the render pipeline.
        virtual void destroyRenderPipeline(RenderPipeline* pipeline) = 0;

        // Create a compute pipeline
        //
        // TODO: Batch api.
        virtual ComputePipeline2* createComputePipeline2(
            ComputePipeline2Descriptor const& desc) = 0;

        // Argument binding
        // ----------------

        // Create a bind group layout object.
        [[deprecated]]
        virtual BindGroupLayout* createBindGroupLayout(
            ShaderStage stage, base::Span<BindEntry> entries) = 0;

        // Create a bind group.
        [[deprecated]]
        virtual BindGroup* createBindGroup(BindGroupLayout* layout) = 0;
        // Create a bind group for the `id`-th argument group of the pipeline.
        virtual BindGroup* createBindGroup(ComputePipeline2* pipeline,
                                           u32 id) = 0;
        virtual BindGroup* createBindGroup(RenderPipeline2* pipeline,
                                           u32 id) = 0;

        // Resources
        // ---------

        // Create a buffer with given data.
        virtual Buffer* createBuffer(BufferUsage usage,
                                     base::Span<base::Byte> bytes) = 0;
        // Create a buffer with size.
        virtual Buffer* createBuffer(BufferUsage usage, u32 size) = 0;
        // Destroy the buffer.
        virtual void destroyBuffer(Buffer* buffer) = 0;

        // Create a texture with given dimensions.
        //
        // TODO: format, 2d
        virtual Texture* createTexture(u32 width, u32 height) = 0;
        virtual Texture* createMonochromeTexture(u32 width, u32 height) = 0;
        // Destroy the texture.
        virtual void destroyTexture(Texture* texture) = 0;

        // Create a surface backing the window.
        // virtual Surface* createSurface(ui::Window* window) = 0;
    };

    // A group of shader arguments that are updated together.
    //
    // A `BindGroup` is a top-level argument of a pipeline. Therefore one
    // creates a `BindGroup` by specifying a pipeline and the name/id of the
    // group.
    //
    // NOTE:
    // - BindGroup of WebGPU
    // - DescriptorSet of Vulkan
    // - ArgumentBuffer of Metal 3
    // - ArgumentTable of Metal 4
    class BindGroup {
    public:
        virtual ~BindGroup() = default;
        // set bind entry by id
        virtual void setBuffer(u32 id, Buffer* buffer) = 0;
        // set bind entry by name
        virtual void setBuffer(base::StringView name, Buffer* buffer) = 0;
    };

    // Layout of a BindGroup.
    //
    // It describes how many objects of each type the bind group can hold.
    //
    // TODO: Can we cache the layout?
    class BindGroupLayout {
    public:
        virtual ~BindGroupLayout() = default;
    };

    class ComputePipeline {
    public:
        virtual ~ComputePipeline() = default;
    };

    class RenderPipeline {
    public:
        virtual ~RenderPipeline() = default;
    };

    class ComputePipeline2 {
    public:
        virtual ~ComputePipeline2() = default;
    };

    class RenderPipeline2 {
    public:
        virtual ~RenderPipeline2() = default;
    };

    // A buffer.
    class Buffer {
    public:
        virtual ~Buffer() = default;
        // Get the mapped address.
        virtual void* mapAddr() = 0;
    };

    class Texture {
    public:
        virtual ~Texture() = default;
        virtual void updateRegion(u32 x, u32 y, u32 width, u32 height,
                                  u32 bytes_per_row,
                                  base::span<base::Byte> bytes) = 0;
    };

    class CommandQueue {
    public:
        virtual ~CommandQueue() = default;
        virtual CommandBuffer* createCommandBuffer() = 0;
        virtual void destroyCommandBuffer(CommandBuffer*) = 0;
        virtual void submit(CommandBuffer*) = 0;
    };

    class CommandBuffer {
    public:
        virtual ~CommandBuffer() = default;
        virtual RenderPassEncoder* beginRenderPass(
            RenderPassDescriptor const& descriptor) = 0;
        virtual void endRenderPass(RenderPassEncoder* encoder) = 0;
        virtual ComputePassEncoder* beginComputePass() = 0;
        virtual void endComputePass(ComputePassEncoder* encoder) = 0;
        // virtual void present(Surface* surface) = 0;
        virtual void submit() = 0;
        virtual void wait() = 0;
    };

    class ComputePassEncoder {
    public:
        virtual ~ComputePassEncoder() = default;
        virtual void setComputePipeline(ComputePipeline* pipeline) = 0;
        virtual void setComputePipeline2(ComputePipeline2* pipeline) = 0;
        virtual void setBindGroup(u32 index, BindGroup* group) = 0;
        virtual void setBuffer(Buffer* buffer,
                               VertexBufferLocation const& loc) = 0;
        // grid_size is the number of thread groups of the grid
        // group_size is the number of threads of a thread group
        virtual void dispatch(DispatchSize grid_size,
                              DispatchSize group_size) = 0;

        virtual void useBuffer(Buffer* buffer, BufferAccess access) = 0;
    };

    class RenderPassEncoder {
    public:
        virtual ~RenderPassEncoder() = default;
        virtual void setRenderPipeline(RenderPipeline* pipeline) = 0;
        virtual void setVertexBuffer(Buffer* buffer,
                                     VertexBufferLocation const& loc) = 0;
        virtual void setFragmentBuffer(Buffer* buffer,
                                       VertexBufferLocation const& loc) = 0;

        virtual void setTexture(Texture* texture) = 0;
        virtual void setViewport(Viewport viewport) = 0;
        virtual void draw(int vertex_start, int vertex_count) = 0;
        virtual void draw(int vertex_start, int vertex_count,
                          int instance_start, int instance_count) = 0;
    };

    class ShaderLibrary {
    public:
        virtual ~ShaderLibrary() = default;
    };

    class ShaderFunction {
    public:
        virtual ~ShaderFunction() = default;
        virtual ShaderLibrary* library() = 0;
        virtual char const* entry() = 0;
    };

    // class Surface {
    // public:
    //     virtual ~Surface() = default;
    //     virtual Texture* nextTexture() = 0;
    //     virtual float width() = 0;
    //     virtual float height() = 0;
    // };

    [[deprecated]]
    base::unique_ptr<Device> makeDevice(DeviceKind kind);

    // class BindGroupLayoutBuilder {
    // public:
    //     void setStage(ShaderStage stage) { _stage = stage; }
    //     void addEntry(u32 binding, BindEntryKind kind) {
    //     _entries.emplace(binding, kind); }

    //    BindGroupLayout* build(Device* _device) {
    //        return _device->createBindGroupLayout(_stage, _entries.toSpan());
    //    }

    // private:
    //     ShaderStage _stage;
    //     base::vector<BindEntry> _entries;
    // };

    // class RenderPipelineBuilder {
    // public:
    //     void setPrimitive(PrimitiveKind primitive) { _desc.primitive =
    //     primitive; } void setVertexShader(ShaderLibrary* library, char const*
    //     entry) {
    //         _desc.vertex_shader.library = library;
    //         _desc.vertex_shader.entry = entry;
    //     }
    //     void setFragmentShader(ShaderLibrary* library, char const* entry) {
    //         _desc.fragment_shader.library = library;
    //         _desc.fragment_shader.entry = entry;
    //     }
    //     void addVertexBuffer(usize stride) {
    //         _vertex_buffers.emplace(stride, VertexStepMode::vertex);
    //     }
    //     void addVertexBuffer(usize stride, VertexStepMode step_mode) {
    //         _vertex_buffers.emplace(stride, step_mode);
    //     }
    //     void addVertexAttribute(VertexAttributeFormat format, u32 location,
    //     u32 buffer,
    //                             u32 offset) {
    //         _vertex_attributes.emplace(buffer, location, format, offset);
    //     }
    //     void addColorAttachment(TextureFormat format, bool enable_blend) {
    //         _color_attachments.emplace(format, enable_blend);
    //     }

    //    RenderPipeline* build(Device* device) {
    //        _desc.vertex_buffers = _vertex_buffers.toSpan();
    //        _desc.vertex_attributes = _vertex_attributes.toSpan();
    //        _desc.color_attachments = _color_attachments.toSpan();
    //        return device->createRenderPipeline(_desc);
    //    }

    // private:
    //     RenderPipelineDescriptor _desc;
    //     base::vector<VertexBufferDescriptor> _vertex_buffers;
    //     base::vector<VertexAttributeDescriptor> _vertex_attributes;
    //     base::vector<ColorAttachmentDescriptor> _color_attachments;
    // };

}  // namespace spargel::gpu

// ## Normalized Device Coordinates
//
// ### Vulkan
//
// Clip coordinates for a vertex result from shader execution, which yields a
// vertex coordinate position.
//
// Perspective division on clip coordinates yields normalized device
// coordinates, followed by a viewport transformation to convert these
// coordinates into framebuffer coordinates.
//
// Rasterizing a primitive begins by determining which squares of an integer
// grid in framebuffer coordinates are occupied by the primitive, and assigning
// one or more depth values to each such square.
//
// ### Metal
//
// The render pipeline linearly maps vertex positions from normalized device
// coordinates to viewport coordinates by applying a viewport during the
// rasterization stage. It applies the transform first and then rasterizes the
// primitive while clipping any fragments outside the scissor rectangle or the
// render target’s extents.
//

// Notes:
//
// msl = metal shading language specification
//
// [msl, v3.2, p84]
// an address space attribute specifies the region of memory from where buffer
// memory objects are allocated. these attributes describe disjoint address
// spaces:
// - device
// - constant
// - thread
// - threadgroup
// - threadgroup_imageblock
// - ray_data
// - object_data
// all arguments to a graphics or kernel function that are a pointer or
// reference to a type needs to be declared with an address space attribute.
//
// the address space for a variable at program scope needs to be `constant`.
//
// [msl, v3.2, p98]
// arguments to graphics and kernel functions can be any of the following:
// - device buffer: a pointer or reference to any data type in the device
// address space
// - constant buffer: a pointer or reference to any data type in the constant
// address space
// - ...
//
// [msl, v3.2, p99]
// for each argument, an attribute can be optionally specified to identify the
// location of a buffer, texture, or sampler to use for this argument type.
// - device and constant buffers: `[[buffer(index)]]`
// - textures (including texture buffers): `[[texture(index)]]`
// - samplers: `[[sampler(index)]]`
//

// NOTE: For the bitflag technique, see
// https://eel.is/c++draft/cmp.partialord#lib:partial_ordering
