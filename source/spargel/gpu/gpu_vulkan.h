#pragma once

#include "spargel/base/hash_map.h"
#include "spargel/base/platform.h"
#include "spargel/base/vector.h"
#include "spargel/config.h"
#include "spargel/gpu/gpu.h"

// vulkan

#define VK_NO_PROTOTYPES
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#if SPARGEL_IS_ANDROID
#include <vulkan/vulkan_android.h>
#endif

#if SPARGEL_IS_LINUX
#include <xcb/xcb.h>
/* after xcb */
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xcb.h>
#endif

#if SPARGEL_IS_MACOS
#include <vulkan/vulkan_metal.h>
#endif

#if SPARGEL_IS_WINDOWS
#include <windows.h>
/* after windows.h */
#include <vulkan/vulkan_win32.h>
#endif

// forward declarations
namespace spargel::gpu {
    class BindGroupVulkan;
    class BindGroupLayoutVulkan;
    class BufferVulkan;
    class CommandBufferVulkan;
    class CommandQueueVulkan;
    class ComputePassEncoderVulkan;
    class ComputePipelineVulkan;
    class DeviceVulkan;
    class ComputePipeline2Vulkan;
    class RenderPassEncoderVulkan;
    class RenderPipelineVulkan;
    class ShaderLibraryVulkan;
    class SurfaceVulkan;
    class TextureVulkan;
}  // namespace spargel::gpu

namespace spargel::gpu {

    struct VulkanProcTable {
        void loadGeneralProcs(base::dynamic_library_handle* library);
        void loadInstanceProcs(VkInstance instance);
        void loadDeviceProcs(VkDevice device);

#define VULKAN_PROC_DECL(name) PFN_##name name;
#define VULKAN_LIBRARY_PROC(name) VULKAN_PROC_DECL(name)
#define VULKAN_GENERAL_PROC(name) VULKAN_PROC_DECL(name)
#define VULKAN_INSTANCE_PROC(name) VULKAN_PROC_DECL(name)
#define VULKAN_DEVICE_PROC(name) VULKAN_PROC_DECL(name)
#include "spargel/gpu/vulkan_procs.inc"
#undef VULKAN_DEVICE_PROC
#undef VULKAN_INSTANCE_PROC
#undef VULKAN_GENERAL_PROC
#undef VULKAN_LIBRARY_PROC
#undef VULKAN_PROC_DECL
    };

    class ShaderLibraryVulkan final : public ShaderLibrary {
    public:
        explicit ShaderLibraryVulkan(VkShaderModule shader)
            : _library{shader} {}
        ~ShaderLibraryVulkan() {}

        auto library() const { return _library; }

    private:
        VkShaderModule _library;
    };

    class RenderPipelineVulkan final : public RenderPipeline {
    public:
        explicit RenderPipelineVulkan();
        ~RenderPipelineVulkan();

        auto pipeline() { return _pipeline; }

    private:
        VkPipeline _pipeline;
    };

    class ComputePipelineVulkan final : public ComputePipeline {
    public:
        ComputePipelineVulkan(VkPipeline pipeline, VkPipelineLayout layout)
            : _pipeline{pipeline}, _layout{layout} {}

        auto pipeline() const { return _pipeline; }
        auto layout() const { return _layout; }

    private:
        VkPipeline _pipeline;
        VkPipelineLayout _layout;
    };

    class BufferVulkan final : public Buffer {
    public:
        BufferVulkan(DeviceVulkan* device, VkBuffer buffer, usize offset,
                     usize size);
        ~BufferVulkan() {}

        auto buffer() const { return _buffer; }

        void* mapAddr();

    private:
        [[maybe_unused]] DeviceVulkan* _device;
        VulkanProcTable const* _procs;
        [[maybe_unused]] VkBuffer _buffer;
        [[maybe_unused]] usize _offset;
        [[maybe_unused]] usize _size;
        void* _addr;
    };

    class TextureVulkan final : public Texture {
    public:
        explicit TextureVulkan(VkImage t) : _texture{t} {}
        ~TextureVulkan() {}

        auto texture() const { return _texture; }

        void updateRegion(u32 x, u32 y, u32 width, u32 height,
                          u32 bytes_per_row,
                          base::span<base::Byte> bytes) override;

    private:
        VkImage _texture;
    };

    // class SurfaceVulkan final : public Surface {
    // public:
    //     explicit SurfaceVulkan(VkSurfaceKHR s) : _surface{s} {}
    //     ~SurfaceVulkan() {}

    //     auto surface() const { return _surface; }

    //     Texture* nextTexture() override;

    //     float width() override;
    //     float height() override;

    // private:
    //     VkSurfaceKHR _surface;
    //     VkSwapchainKHR _swapchain;
    // };

    class CommandQueueVulkan final : public CommandQueue {
    public:
        CommandQueueVulkan(VkQueue queue, DeviceVulkan* device);
        ~CommandQueueVulkan() {}

        auto commandQueue() { return _queue; }

        CommandBuffer* createCommandBuffer() override;
        void destroyCommandBuffer(CommandBuffer*) override;
        void submit(CommandBuffer*) override {}

    private:
        void createCommandPool();

        DeviceVulkan* _device;
        VulkanProcTable const* _procs;
        VkQueue _queue;
        VkCommandPool _pool;
    };

    class CommandBufferVulkan final : public CommandBuffer {
    public:
        CommandBufferVulkan(DeviceVulkan* device, CommandQueueVulkan* queue,
                            VkCommandBuffer cmdbuf);
        ~CommandBufferVulkan() {}

        auto commandBuffer() { return _cmdbuf; }

        RenderPassEncoder* beginRenderPass(
            RenderPassDescriptor const& descriptor) override;
        void endRenderPass(RenderPassEncoder* encoder) override;
        ComputePassEncoder* beginComputePass() override;
        void endComputePass(ComputePassEncoder* encoder) override;
        // void present(Surface* surface) override;
        void submit() override;
        void wait() override;

    private:
        void beginCommandBuffer();
        void endCommandBuffer();
        void createFence();

        DeviceVulkan* _device;
        CommandQueueVulkan* _queue;
        VulkanProcTable const* _procs;
        VkCommandBuffer _cmdbuf;
        VkFence _fence;
    };

    class RenderPassEncoderVulkan final : public RenderPassEncoder {
    public:
        explicit RenderPassEncoderVulkan() {}

        void setRenderPipeline(RenderPipeline* pipeline) override;
        void setVertexBuffer(Buffer* buffer,
                             VertexBufferLocation const& loc) override;
        void setFragmentBuffer(Buffer*, VertexBufferLocation const&) override {}
        void setTexture(Texture* texture) override;
        void setViewport(Viewport viewport) override;
        void draw(int vertex_start, int vertex_count) override;
        void draw(int vertex_start, int vertex_count, int instance_start,
                  int instance_count) override;

    private:
    };

    class ComputePassEncoderVulkan final : public ComputePassEncoder {
    public:
        ComputePassEncoderVulkan(DeviceVulkan* device, VkCommandBuffer cmdbuf);

        void setComputePipeline(ComputePipeline* pipeline) override;
        void setComputePipeline2(ComputePipeline2* pipeline) override;
        void setBindGroup(u32 index, BindGroup* group) override;
        void setBuffer(Buffer* buffer,
                       VertexBufferLocation const& loc) override;
        void dispatch(DispatchSize grid_size, DispatchSize group_size) override;
        void useBuffer(Buffer*, BufferAccess) override {}

    private:
        // DeviceVulkan* _device;
        VulkanProcTable const* _procs;
        VkCommandBuffer _cmdbuf;
    };

    class BindGroupLayoutVulkan final : public BindGroupLayout {
    public:
        explicit BindGroupLayoutVulkan(VkDescriptorSetLayout layout)
            : _layout{layout} {}

        auto layout() const { return _layout; }

    private:
        VkDescriptorSetLayout _layout;
    };

    class ComputePipeline2Vulkan;

    class BindGroupVulkan final : public BindGroup {
    public:
        BindGroupVulkan(VkDescriptorSet set, DeviceVulkan* device,
                        ComputePipeline2Vulkan* pipeline, u32 id);

        void setBuffer(u32 id, Buffer* buffer) override;
        void setBuffer(base::StringView, Buffer*) override {}

        auto getDescriptorSet() const { return _set; }

        auto pipeline() const { return _pipeline; }

    private:
        VkDescriptorSet _set;
        DeviceVulkan* _device;
        VulkanProcTable const* _procs;
        ComputePipeline2Vulkan* _pipeline;
        u32 _id;
    };

    struct ArgumentInfoVulkan {
        // The index of the group in the program.
        u32 id;
        // The index of the argument in the group.
        u32 binding;
        BindEntryKind kind;
    };

    struct ArgumentGroupInfoVulkan {
        // The index of the group in the program.
        u32 id;
        // The index of the group in shader.
        u32 set_id;
    };

    class ComputePipeline2Vulkan final : public ComputePipeline2 {
    public:
        ComputePipeline2Vulkan(DeviceVulkan* device, ShaderLibrary* library,
                               char const* entry,
                               base::span<PipelineArgumentGroup> groups);

        auto layout() const { return _layout; }
        auto pipeline() const { return _pipeline; }

        BindGroupVulkan* createBindGroup2(u32 id);

        u32 getSetId(u32 id) {
            for (usize i = 0; i < _groups.count(); i++) {
                if (_groups[i].id == id) {
                    return _groups[i].set_id;
                }
            }
            spargel_panic_here();
        }

        // `id` is the abstract index.
        VkDescriptorType getDescriptorType(u32 id, u32 binding);

    private:
        void createDescriptorSetLayouts(
            base::span<PipelineArgumentGroup> groups);
        void createPipelineLayout();
        void createPipeline(ShaderLibrary* library, char const* entry);

        DeviceVulkan* _device;
        VulkanProcTable const* _procs;

        base::Vector<ArgumentGroupInfoVulkan> _groups;
        base::Vector<ArgumentInfoVulkan> _args;

        base::Vector<VkDescriptorSetLayoutBinding> _bindings;

        base::Vector<VkDescriptorSetLayout> _dset_layouts;

        VkPipelineLayout _layout;
        VkPipeline _pipeline;
    };

    // // One DeviceMemoryBlock corresponds to one VkDeviceMemory, i.e. one
    // allocation. class DeviceMemoryBlock { public:
    //     explicit DeviceMemoryBlock(VkDeviceMemory memory) : _memory{memory}
    //     {}

    // private:
    //     VkDeviceMemory _memory;
    // };
    // class DeviceMemoryAllocatorVulkan {
    // public:
    //     explicit DeviceMemoryAllocatorVulkan(DeviceVulkan* device) :
    //     _device{device} {}

    // private:
    //     DeviceVulkan* _device;
    //     base::vector<DeviceMemoryBlock> _blocks;
    // };

    struct DescriptorSetShape {
        friend constexpr bool operator==(DescriptorSetShape const& lhs,
                                         DescriptorSetShape const& rhs) {
            return (lhs.uniform_buffer_count == rhs.uniform_buffer_count) &&
                   (lhs.storage_buffer_count == rhs.storage_buffer_count) &&
                   (lhs.sampled_texture_count == rhs.sampled_texture_count) &&
                   (lhs.storage_texture_count == rhs.storage_texture_count);
        }

        u32 uniform_buffer_count;
        u32 storage_buffer_count;
        u32 sampled_texture_count;
        u32 storage_texture_count;

        friend void tag_invoke(base::tag<base::hash>, base::HashRun& run,
                               DescriptorSetShape const& shape) {
            run.combine(shape.uniform_buffer_count);
            run.combine(shape.storage_buffer_count);
            run.combine(shape.sampled_texture_count);
            run.combine(shape.storage_texture_count);
        }
    };

    class ShapedDescriptorAllocator {
    public:
        ShapedDescriptorAllocator(DeviceVulkan* device,
                                  DescriptorSetShape const& shape);

    private:
        void createLayout();

        [[maybe_unused]] DeviceVulkan* _device;
        VulkanProcTable const* _procs;
        [[maybe_unused]] VkDescriptorPool _pool;
        [[maybe_unused]] VkDescriptorSetLayout _layout;
    };

    class DescriptorAllocator {
    public:
        DescriptorAllocator(DeviceVulkan* device);

        VkDescriptorSet allocate(DescriptorSetShape const& shape);

    private:
        DeviceVulkan* _device;
        VulkanProcTable const* _procs;
        base::HashMap<DescriptorSetShape, ShapedDescriptorAllocator> _suballocs;
    };

    class DeviceVulkan final : public Device {
    public:
        DeviceVulkan();
        ~DeviceVulkan() override;

        // Device
        CommandQueue* createCommandQueue() override;
        void destroyCommandQueue(CommandQueue* q) override;
        CommandBuffer* createCommandBuffer() override { return nullptr; }
        void destroyCommandBuffer(CommandBuffer*) override {}
        ShaderLibrary* createShaderLibrary(base::span<u8> bytes) override;
        void destroyShaderLibrary(ShaderLibrary* library) override;
        ShaderFunction* createShaderFunction(base::StringView) override {
            return nullptr;
        }
        void destroyShaderFunction(ShaderFunction*) override {}
        RenderPipeline* createRenderPipeline(
            RenderPipelineDescriptor const& descriptor) override;
        RenderPipeline2* createRenderPipeline2(
            RenderPipeline2Descriptor const&) override {
            return nullptr;
        }
        void destroyRenderPipeline(RenderPipeline* pipeline) override;
        // ComputePipeline* createComputePipeline(
        //     ShaderFunction func, base::span<BindGroupLayout*> layouts)
        //     override;
        ComputePipeline2* createComputePipeline2(
            ComputePipeline2Descriptor const&) override {
            return nullptr;
            // return make_object<ComputePipeline2Vulkan>(this, desc.compute,
            //                                            desc.groups);
        }

        BindGroupLayout* createBindGroupLayout(
            ShaderStage stage, base::span<BindEntry> entries) override;

        BindGroup* createBindGroup(BindGroupLayout*) override {
            return nullptr;
        }
        BindGroup* createBindGroup(ComputePipeline2* p, u32 id) override;
        BindGroup* createBindGroup(RenderPipeline2*, u32) override {
            return nullptr;
        }

        Buffer* createBuffer(BufferUsage usage, base::span<u8> bytes) override;
        Buffer* createBuffer(BufferUsage usage, u32 size) override;
        void destroyBuffer(Buffer* b) override;
        // Surface* createSurface(ui::Window* w) override;
        Texture* createTexture(u32 width, u32 height) override;
        Texture* createMonochromeTexture(u32, u32) override { return nullptr; }
        void destroyTexture(Texture* texture) override;

        auto device() const { return _device; }
        u32 getQueueFamilyIndex() const { return _queue_family_index; }
        VulkanProcTable const* getProcTable() const { return &_procs; }

        VkInstance getVkInstance() const { return _instance; }
        VkPhysicalDevice getVkPhysicalDevice() const {
            return _physical_device;
        }
        VkDevice getVkDevice() const { return _device; }

        VkDescriptorPool getDescriptorPool() const { return _dpool; }

        VkDeviceMemory getMemoryPool() const { return _memory_pool; }
        void* getPoolAddr() const { return _pool_addr; }

    private:
        friend class InstanceBuilder;
        friend class PhysicalDeviceBuilder;
        friend class DeviceBuilder;

        struct InstanceExtensions {
            bool debug_utils;
            bool portability_enumeration;
        };

        // info for the selected physical device
        struct PhysicalDeviceInfo {
            u32 max_memory_allocation_count;
        };

        void createInstance();
        void createDebugMessenger();
        void selectPhysicalDevice();
        void createDevice();
        void getQueue();
        void queryMemoryInfo();
        void createDescriptorPool();
        void createMemoryPool();
        usize allocateMemory(usize size, usize align);

        base::dynamic_library_handle* _library;
        VulkanProcTable _procs;

        InstanceExtensions _instance_extensions;

        VkInstance _instance;
        VkDebugUtilsMessengerEXT _debug_messenger;

        VkPhysicalDevice _physical_device;
        PhysicalDeviceInfo _physical_device_info;

        u32 _queue_family_index = 0;

        VkDevice _device;
        VkQueue _queue;

        VkDescriptorPool _dpool;
        DescriptorAllocator _dalloc;

        VkPhysicalDeviceMemoryProperties _memory_props;

        // TODO:
        u32 _memory_type;  // unified memory

        static constexpr usize pool_size = 1024 * 1024;  // 1MB
        VkDeviceMemory _memory_pool;
        usize _pool_offset = 0;
        void* _pool_addr;

        // todo: hook allocations
        [[maybe_unused]] VkAllocationCallbacks _vkalloc;
    };

}  // namespace spargel::gpu
