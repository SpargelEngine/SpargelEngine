#pragma once

#include <spargel/base/allocator.h>
#include <spargel/base/intrinsic.h>
#include <spargel/base/meta.h>
#include <spargel/base/object.h>
#include <spargel/base/span.h>
#include <spargel/base/string_view.h>
#include <spargel/base/types.h>
#include <spargel/base/unique_ptr.h>
#include <spargel/base/vector.h>

namespace spargel::ui {
    class Window;
}

// forward declarations
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
    class Surface;
    class Texture;
}  // namespace spargel::gpu

namespace spargel::gpu {

    // dummy wrapper for gpu objects
    template <typename T>
    class ObjectPtr {
    public:
        ObjectPtr() = default;
        ObjectPtr(nullptr_t) {}

        template <typename U>
            requires(base::is_convertible<U*, T*>)
        explicit ObjectPtr(U* ptr) : _ptr{ptr} {}

        template <typename U>
            requires(base::is_convertible<U*, T*>)
        ObjectPtr(ObjectPtr<U> const ptr) : _ptr{ptr.get()} {}

        T* operator->() { return _ptr; }
        T const* operator->() const { return _ptr; }

        T* get() const { return _ptr; }

        template <typename U>
        ObjectPtr<U> cast() {
            return ObjectPtr<U>(static_cast<U*>(_ptr));
        }
        template <typename U>
        ObjectPtr<U> const cast() const {
            return ObjectPtr<U>(static_cast<U*>(_ptr));
        }

    private:
        T* _ptr = nullptr;
    };

    template <typename T, typename... Args>
    ObjectPtr<T> make_object(Args&&... args) {
        return ObjectPtr<T>(
            base::default_allocator()->allocObject<T>(base::forward<Args>(args)...));
    }

    template <typename T>
    void destroy_object(ObjectPtr<T> ptr) {
        base::default_allocator()->freeObject(ptr.get());
    }

    // Enums

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

    /// @brief triangles with particular facing will not be drawn
    enum class CullMode {
        /// @brief no triangles are discarded
        none,
        /// @brief front-facing triangles are discarded
        front,
        /// @brief back-facing triangles are discarded
        back,
    };

    /// @brief the front-facing orientation
    enum class Orientation {
        /// @brief clockwise triangles are front-facing
        clockwise,
        /// @brief counter-clockwise triangles are front-facing
        counter_clockwise,
    };

    /// @brief geometric primitive type for rendering
    enum class PrimitiveKind {
        point,
        line,
        triangle,
    };

    /// @brief the rate of fetching vertex attributes
    enum class VertexStepMode {
        /// @brief attributes are fetched per vertex
        vertex,
        /// @brief attributes are fetched per instance
        instance,
    };

    enum class VertexAttributeFormat {
        uint8,
        uint8x2,
        uint8x4,
        sint8,
        sint8x2,
        sint8x4,
        unorm8,
        unorm8x2,
        unorm8x4,
        snorm8,
        snorm8x2,
        snorm8x4,
        uint16,
        uint16x2,
        uint16x4,
        sint16,
        sint16x2,
        sint16x4,
        unorm16,
        unorm16x2,
        unorm16x4,
        snorm16,
        snorm16x2,
        snorm16x4,
        float16,
        float16x2,
        float16x4,
        float32,
        float32x2,
        float32x4,
    };

    enum class BlendAction {
        min,
        max,
        add,
        subtract,
        reverse_subtract,
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

    enum class BindEntryKind {
        uniform_buffer,
        storage_buffer,
        sample_texture,
        storage_texture,
    };

    // Bitflags

    struct BufferUsage {
        struct BufferUsageImpl {
            u32 value;
            constexpr operator BufferUsage() const { return BufferUsage(value); }
            constexpr BufferUsageImpl operator|(BufferUsageImpl other) const {
                return BufferUsageImpl(value | other.value);
            }
        };

        static constexpr auto map_read = BufferUsageImpl(1 << 0);
        static constexpr auto map_write = BufferUsageImpl(1 << 1);
        static constexpr auto copy_src = BufferUsageImpl(1 << 2);
        static constexpr auto copy_dst = BufferUsageImpl(1 << 3);
        static constexpr auto index = BufferUsageImpl(1 << 4);
        static constexpr auto vertex = BufferUsageImpl(1 << 5);
        static constexpr auto uniform = BufferUsageImpl(1 << 5);
        static constexpr auto storage = BufferUsageImpl(1 << 6);
        static constexpr auto indirect = BufferUsageImpl(1 << 7);

        constexpr bool has(BufferUsage usage) const { return (value & usage.value) != 0; }

        constexpr BufferUsage operator|(BufferUsage other) const {
            return BufferUsage(value | other.value);
        }

        u32 value;
    };

    struct ShaderStage {
        struct ShaderStageImpl {
            constexpr operator ShaderStage() const { return ShaderStage(value); }
            u32 value;
        };
        static constexpr auto vertex = ShaderStageImpl(0b1);
        static constexpr auto fragment = ShaderStageImpl(0b10);
        static constexpr auto compute = ShaderStageImpl(0b100);
        constexpr bool has(ShaderStage usage) const { return (value & usage.value) != 0; }
        friend constexpr bool operator==(ShaderStage lhs, ShaderStage rhs) {
            return lhs.value == rhs.value;
        }
        u32 value;
    };

    struct BufferAccess {
        struct BufferAccessImpl {
            constexpr operator BufferAccess() const { return BufferAccess(value); }
            u32 value;
        };
        static constexpr auto read = BufferAccessImpl(0b1);
        static constexpr auto write = BufferAccessImpl(0b10);
        static constexpr auto read_write = BufferAccessImpl(0b11);
        constexpr bool has(BufferAccess access) const { return (value & access.value) != 0; }
        friend constexpr bool operator==(BufferAccess lhs, BufferAccess rhs) {
            return lhs.value == rhs.value;
        }
        u32 value;
    };

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

    struct ShaderFunction {
        ObjectPtr<ShaderLibrary> library;
        char const* entry;
    };

    /// @brief one entry of a bind group
    struct BindEntry {
        /// @brief the unique identifier of this entry
        ///
        /// Syntax in shaders:
        /// - glsl: `location (set = 0, binding = 1) ...`
        /// - msl: `struct ArgBuf { float* buf [[id(1)]]; };`
        ///
        u32 binding;

        /// @brief the kind of this entry
        BindEntryKind kind;
    };

    // Descriptors

    struct ShaderLibraryDescriptor {
        base::span<u8> bytes;
    };

    /// @brief description of one vertex attribute
    struct VertexAttributeDescriptor {
        /// @brief index of the vertex buffer where the data of the attribute is fetched
        u32 buffer;

        /// @brief the location of the vertex attribute
        ///
        /// This is called input element in directx.
        ///
        /// Synatx in shaders:
        /// - glsl: `layout (location = 0) in vec3 position;`
        /// - msl: `float3 position [[attribute(0)]];`
        /// - hlsl: this is specified via semantic name and semantic index.
        ///
        /// Note:
        /// - Both dawn and wgpu use dummy SemanticName, and use SemanticIndex for location.
        /// - Dawn chooses "TEXCOORD", and wgpu/naga uses "LOC".
        /// - SPIRV-Cross uses "TEXCOORD" by default, and provides the options for remapping.
        /// - dxc relies on user declaration `[[vk::location(0)]]`.
        ///
        u32 location;

        /// @brief the format of the vertex attribute
        VertexAttributeFormat format;

        /// @brief the offset of the vertex attribute to the start of the vertex data
        ssize offset;
    };

    /// @brief description of one vertex buffer
    struct VertexBufferDescriptor {
        /// @brief the number of bytes between consecutive elements in the buffer
        usize stride;

        /// @brief the rate of fetching vertex attributes
        VertexStepMode step_mode;

        // TODO: step rate is not supported by vulkan.
    };

    struct ColorAttachmentBindDescriptor {
        ObjectPtr<Texture> texture;
        LoadAction load = LoadAction::clear;
        StoreAction store = StoreAction::store;
        ClearColor clear_color;
    };

    struct ColorAttachmentDescriptor {
        TextureFormat format;
        bool enable_blend = false;
    };

    /// @brief The description of a compute pipeline.
    ///
    /// Most information should be automatically generated using reflection.
    ///
    struct ComputePipelineDescriptor {
        ShaderFunction shader;
    };

    struct RenderPassDescriptor {
        base::span<ColorAttachmentBindDescriptor> color_attachments;
    };

    /// @brief description of a render pipeline
    struct RenderPipelineDescriptor {
        /// @brief the geometric primitive for this render pipeline
        PrimitiveKind primitive = PrimitiveKind::triangle;

        /// @brief the orientation of front-facing triangles
        Orientation front_face = Orientation::counter_clockwise;

        /// @brief which triangles to discard
        CullMode cull = CullMode::none;

        /// @brief the vertex function
        ShaderFunction vertex_shader;

        /// @brief the vertex buffers used in this pipeline
        base::span<VertexBufferDescriptor> vertex_buffers;

        /// @brief the vertex attributes used in this pipeline
        base::span<VertexAttributeDescriptor> vertex_attributes;

        /// @brief the fragment function
        ShaderFunction fragment_shader;

        base::span<ColorAttachmentDescriptor> color_attachments;
    };

    struct PipelineArgument {
        u32 id;
        BindEntryKind kind;
    };

    struct GroupLocation {
        struct {
            u32 buffer_id;
        } metal;
        struct {
            u32 set_id;
        } vulkan;
    };

    struct PipelineArgumentGroup {
        /// @brief stage where this group of arguments is used
        ShaderStage stage;
        GroupLocation location;
        base::span<PipelineArgument> arguments;
    };

    struct PipelinePart {
        ShaderStage stage;
        ShaderFunction func;
    };

    struct ComputePipeline2Descriptor {
        ShaderFunction compute;
        base::span<PipelineArgumentGroup> groups;
    };

    struct RenderPipeline2Descriptor {
        /// @brief the geometric primitive for this render pipeline
        PrimitiveKind primitive = PrimitiveKind::triangle;

        /// @brief the orientation of front-facing triangles
        Orientation front_face = Orientation::counter_clockwise;

        /// @brief which triangles to discard
        CullMode cull = CullMode::none;

        /// @brief the vertex function
        ShaderFunction vertex_shader;

        /// @brief the vertex buffers used in this pipeline
        base::span<VertexBufferDescriptor> vertex_buffers;

        /// @brief the vertex attributes used in this pipeline
        base::span<VertexAttributeDescriptor> vertex_attributes;

        /// @brief the fragment function
        ShaderFunction fragment_shader;

        base::span<ColorAttachmentDescriptor> color_attachments;

        base::span<PipelineArgumentGroup> groups;
    };

    // Interfaces

    class BindGroup {
    public:
        virtual void setBuffer(u32 id, ObjectPtr<Buffer> buffer) = 0;
    };

    class BindGroupLayout {};

    class Buffer {
    public:
        virtual void* mapAddr() = 0;
    };

    class CommandBuffer {
    public:
        virtual ObjectPtr<RenderPassEncoder> beginRenderPass(
            RenderPassDescriptor const& descriptor) = 0;
        virtual void endRenderPass(ObjectPtr<RenderPassEncoder> encoder) = 0;
        virtual ObjectPtr<ComputePassEncoder> beginComputePass() = 0;
        virtual void endComputePass(ObjectPtr<ComputePassEncoder> encoder) = 0;
        virtual void present(ObjectPtr<Surface> surface) = 0;
        virtual void submit() = 0;
        virtual void wait() = 0;
    };

    class CommandQueue {
    public:
        virtual ObjectPtr<CommandBuffer> createCommandBuffer() = 0;
        virtual void destroyCommandBuffer(ObjectPtr<CommandBuffer>) = 0;
    };

    class ComputePassEncoder {
    public:
        virtual void setComputePipeline(ObjectPtr<ComputePipeline> pipeline) = 0;
        virtual void setComputePipeline2(ObjectPtr<ComputePipeline2> pipeline) = 0;
        virtual void setBindGroup(u32 index, ObjectPtr<BindGroup> group) = 0;
        virtual void setBuffer(ObjectPtr<Buffer> buffer, VertexBufferLocation const& loc) = 0;
        // grid_size is the number of thread groups of the grid
        // group_size is the number of threads of a thread group
        virtual void dispatch(DispatchSize grid_size, DispatchSize group_size) = 0;

        virtual void useBuffer(ObjectPtr<Buffer> buffer, BufferAccess access) = 0;
    };

    class ComputePipeline {
    public:
        // virtual u32 maxGroupSize() = 0;
    };

    class Device {
    public:
        virtual ~Device() = default;

        DeviceKind kind() const { return _kind; }

        virtual ObjectPtr<ShaderLibrary> createShaderLibrary(base::span<u8> bytes) = 0;
        virtual ObjectPtr<RenderPipeline> createRenderPipeline(
            RenderPipelineDescriptor const& descriptor) = 0;
        virtual ObjectPtr<Surface> createSurface(ui::Window* w) = 0;
        virtual void destroyShaderLibrary(ObjectPtr<ShaderLibrary> library) = 0;
        virtual void destroyRenderPipeline(ObjectPtr<RenderPipeline> pipeline) = 0;

        virtual ObjectPtr<Buffer> createBuffer(BufferUsage usage, base::span<u8> bytes) = 0;
        virtual ObjectPtr<Buffer> createBuffer(BufferUsage usage, u32 size) = 0;
        virtual void destroyBuffer(ObjectPtr<Buffer> buffer) = 0;

        // todo: format, 2d
        virtual ObjectPtr<Texture> createTexture(u32 width, u32 height) = 0;
        virtual void destroyTexture(ObjectPtr<Texture> texture) = 0;

        virtual ObjectPtr<CommandQueue> createCommandQueue() = 0;
        virtual void destroyCommandQueue(ObjectPtr<CommandQueue> queue) = 0;

        virtual ObjectPtr<ComputePipeline> createComputePipeline(
            ShaderFunction func, base::span<ObjectPtr<BindGroupLayout>> layouts) = 0;
        // virtual ObjectPtr<ComputePipeline> createComputePipeline2(
        //     ObjectPtr<ComputePipeline2> program) = 0;

        /// @brief create a bind group layout object
        /// @param stage the stage where the layout is used
        /// @param entries the entries of the layout
        virtual ObjectPtr<BindGroupLayout> createBindGroupLayout(ShaderStage stage,
                                                                 base::span<BindEntry> entries) = 0;

        virtual ObjectPtr<BindGroup> createBindGroup(ObjectPtr<BindGroupLayout> layout) = 0;

        virtual ObjectPtr<ComputePipeline2> createComputePipeline2(
            ComputePipeline2Descriptor const& desc) = 0;
        virtual ObjectPtr<RenderPipeline2> createRenderPipeline2(
            RenderPipeline2Descriptor const& desc) = 0;
        /// Create a bind group for the `id`-th argument group of the pipeline.
        virtual ObjectPtr<BindGroup> createBindGroup2(ObjectPtr<ComputePipeline2> pipeline,
                                                      u32 id) = 0;
        virtual ObjectPtr<BindGroup> createBindGroup2(ObjectPtr<RenderPipeline2> pipeline,
                                                      u32 id) = 0;

    protected:
        explicit Device(DeviceKind k) : _kind{k} {}

    private:
        DeviceKind _kind;
    };

    /// PipelineLayout describes the signature of a pipeline.
    ///
    /// A pipeline can be viewed abstractly as a function. Then pipeline layout is the description
    /// of its arguments.
    ///

    class ComputePipeline2 {};
    class RenderPipeline2 {};

    class RenderPassEncoder {
    public:
        virtual void setRenderPipeline(ObjectPtr<RenderPipeline> pipeline) = 0;
        virtual void setVertexBuffer(ObjectPtr<Buffer> buffer, VertexBufferLocation const& loc) = 0;
        virtual void setFragmentBuffer(ObjectPtr<Buffer> buffer,
                                       VertexBufferLocation const& loc) = 0;

        virtual void setTexture(ObjectPtr<Texture> texture) = 0;
        virtual void setViewport(Viewport viewport) = 0;
        virtual void draw(int vertex_start, int vertex_count) = 0;
        virtual void draw(int vertex_start, int vertex_count, int instance_start,
                          int instance_count) = 0;
    };

    class RenderPipeline {};

    class ShaderLibrary {};

    class Surface {
    public:
        virtual ObjectPtr<Texture> nextTexture() = 0;
        virtual float width() = 0;
        virtual float height() = 0;
    };

    class Texture {
    public:
        virtual void updateRegion(u32 x, u32 y, u32 width, u32 height, u32 bytes_per_row,
                                  base::span<base::Byte> bytes) = 0;
    };

    base::unique_ptr<Device> makeDevice(DeviceKind kind);

    // Helpers

    class BindGroupLayoutBuilder {
    public:
        void setStage(ShaderStage stage) { _stage = stage; }
        void addEntry(u32 binding, BindEntryKind kind) { _entries.push(binding, kind); }

        ObjectPtr<BindGroupLayout> build(Device* _device) {
            return _device->createBindGroupLayout(_stage, _entries.toSpan());
        }

    private:
        ShaderStage _stage;
        base::vector<BindEntry> _entries;
    };

    class RenderPipelineBuilder {
    public:
        void setPrimitive(PrimitiveKind primitive) { _desc.primitive = primitive; }
        void setVertexShader(ObjectPtr<ShaderLibrary> library, char const* entry) {
            _desc.vertex_shader.library = library;
            _desc.vertex_shader.entry = entry;
        }
        void setFragmentShader(ObjectPtr<ShaderLibrary> library, char const* entry) {
            _desc.fragment_shader.library = library;
            _desc.fragment_shader.entry = entry;
        }
        void addVertexBuffer(usize stride) { _vertex_buffers.push(stride, VertexStepMode::vertex); }
        void addVertexBuffer(usize stride, VertexStepMode step_mode) {
            _vertex_buffers.push(stride, step_mode);
        }
        void addVertexAttribute(VertexAttributeFormat format, u32 location, u32 buffer,
                                u32 offset) {
            _vertex_attributes.push(buffer, location, format, offset);
        }
        void addColorAttachment(TextureFormat format, bool enable_blend) {
            _color_attachments.push(format, enable_blend);
        }

        ObjectPtr<RenderPipeline> build(Device* device) {
            _desc.vertex_buffers = _vertex_buffers.toSpan();
            _desc.vertex_attributes = _vertex_attributes.toSpan();
            _desc.color_attachments = _color_attachments.toSpan();
            return device->createRenderPipeline(_desc);
        }

    private:
        RenderPipelineDescriptor _desc;
        base::vector<VertexBufferDescriptor> _vertex_buffers;
        base::vector<VertexAttributeDescriptor> _vertex_attributes;
        base::vector<ColorAttachmentDescriptor> _color_attachments;
    };

    // This is the allocator for device memory.
    //
    // The maximal memory allocation count (maxMemoryAllocationCount) returned by Vulkan drivers is
    // still 4096 on half of the devices. So we should have few allocations of large device memory
    // blocks, and then suballocate from these blocks. It's then our task to handle memory
    // fragmentation.
    //
    // Two algorithms:
    // - Bump allocator.
    // - TLSF (two-level seggregated fit) allocator.
    //

    // The TLSF Allocator
    //
    // One instance manages a sequence of memory blocks, which are binned into a two levels.
    //
    // The number of (first level) bins is `bin_count`. Each bin contains `subbin_count` many
    // (second level) sub-bins. Every sub-bin contains a sequence of memory blocks, organized as a
    // free-list.
    //
    // Suppose the i-th (first level) bin is for memory blocks with size in [a, b).
    // Let delta be ceil((b - a) / subbin_count).
    // Then the j-th sub-bin is for blocks with size in [a + j * delta, a + (j + 1) * delta).
    //
    // We maintain two lookup structures:
    // - The first one is a bitset indicating whether each bin contains free memory blocks.
    // - The second one is a bitset for every bin, indicating which sub-bins contains free blocks.
    //
    // It remains to have a good choice of (first level) bins. A common approach is to associate
    // [2^i, 2^(i+1)) with the i-th bin. Zero-sized allocations are special handled. However, this
    // is not ideal for small sized bins.
    //
    // Therefore, we use the following ranges:
    // - 0-th bin: [2^0, 2^linear_bits)
    // - 1-th bin: [2^linear_bits, 2^(linear_bits + 1))
    // - 2-th bin: ...
    //
    // Then delta for sub-bins in the 0-th bin is (2^linear_bits - 1) / subbin_count, which is
    // about 3.9.
    //
    // Note: We would like 2^linear_bits to be divisible by subbin_count. So we take subbin_count to
    // be a power of 2. Moreover, we require that subbin_count <= 2^(linear_bits - 2) so that delta
    // >= 4.
    //
    //
    // size   | [2^0, 2^8) | [2^8, 2^9) | [2^9, 2^10) | ...
    // bin_id |     0      |      1     |       2     | ...
    //
    class TLSFAllocator {
        static constexpr u8 linear_bits = 8;
        static constexpr u8 subbin_bits = 5;
        static constexpr u8 subbin_count = 1 << subbin_bits;

    public:
        TLSFAllocator() = default;

    private:
        struct BinResult {
            u8 bin_id;
            u8 subbin_id;
        };

        // BinResult binDown(usize size) {
        //     spargel_assert(size > 0);

        //     u8 bin_id;
        //     if (size < (1 << linear_bits)) {
        //         bin_id = 0;
        //     } else {
        //         bin_id = base::GetMostSignificantBit(size) - (linear_bits - 1);
        //     }
        //     u8 subbin_id = size >> (bin_id - subbin_bits);
        //     return {bin_id, subbin_id};
        // }

        // BinResult binUp(usize size) {}

        // u64 _first_bitset = 0;
        // u64 _second_bitsets[subbin_count] = {0};
    };

}  // namespace spargel::gpu

// ## Normalized Device Coordinates
//
// ### Vulkan
//
// Clip coordinates for a vertex result from shader execution, which yields a vertex coordinate
// Position.
//
// Perspective division on clip coordinates yields normalized device coordinates, followed by a
// viewport transformation to convert these coordinates into framebuffer coordinates.
//
// Rasterizing a primitive begins by determining which squares of an integer grid in framebuffer
// coordinates are occupied by the primitive, and assigning one or more depth values to each such
// square.
//
// ### Metal
//
// The render pipeline linearly maps vertex positions from normalized device coordinates to viewport
// coordinates by applying a viewport during the rasterization stage. It applies the transform first
// and then rasterizes the primitive while clipping any fragments outside the scissor rectangle or
// the render target’s extents.
//

// Notes:
//
// msl = metal shading language specification
//
// [msl, v3.2, p84]
// an address space attribute specifies the region of memory from where buffer memory objects
// are allocated. these attributes describe disjoint address spaces:
// - device
// - constant
// - thread
// - threadgroup
// - threadgroup_imageblock
// - ray_data
// - object_data
// all arguments to a graphics or kernel function that are a pointer or reference to a type
// needs to be declared with an address space attribute.
//
// the address space for a variable at program scope needs to be `constant`.
//
// [msl, v3.2, p98]
// arguments to graphics and kernel functions can be any of the following:
// - device buffer: a pointer or reference to any data type in the device address space
// - constant buffer: a pointer or reference to any data type in the constant address space
// - ...
//
// [msl, v3.2, p99]
// for each argument, an attribute can be optionally specified to identify the location of a
// buffer, texture, or sampler to use for this argument type.
// - device and constant buffers: `[[buffer(index)]]`
// - textures (including texture buffers): `[[texture(index)]]`
// - samplers: `[[sampler(index)]]`
//
