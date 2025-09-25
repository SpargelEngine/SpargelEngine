#include "spargel/gpu/gpu_vulkan.h"

#include "spargel/base/assert.h"
#include "spargel/base/object.h"
#include "spargel/base/platform.h"
#include "spargel/base/types.h"
#include "spargel/base/vector.h"
#include "spargel/config.h"
#include "spargel/logging/logging.h"
#include "spargel/ui/window.h"

// libc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_VK_RESULT(expr)                                            \
    do {                                                                 \
        VkResult result = expr;                                          \
        if (result != VK_SUCCESS) {                                      \
            spargel_log_fatal(#expr " failed with result = %d", result); \
            spargel_panic_here();                                        \
        }                                                                \
    } while (0)

namespace spargel::gpu {

    namespace {

        VKAPI_ATTR VkBool32 VKAPI_CALL onDebugUtilsMessage(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            VkDebugUtilsMessengerCallbackDataEXT const* data, void* user_data) {
            fprintf(stderr, "validator: %s\n", data->pMessage);
            if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                spargel_panic_here();
            }
            return VK_FALSE;
        }

        float const vulkan_queue_priorities[4] = {
            1.0,
            1.0,
            1.0,
            1.0,
        };

        constexpr char const* vulkan_library_name =
#if SPARGEL_IS_ANDROID
            "libvulkan.so"
#elif SPARGEL_IS_LINUX
            "libvulkan.so.1"
#elif SPARGEL_IS_MACOS
            "libvulkan.dylib"
#elif SPARGEL_IS_WINDOWS
            "vulkan-1.dll"
#else
#error unimplemented
#endif
            ;

        VkBufferUsageFlags translateBufferUsage(BufferUsage usage) {
            VkBufferUsageFlags flags = 0;
#define _TRANSLATE(x, y)             \
    if (usage.has(BufferUsage::x)) { \
        flags |= y;                  \
    }
            _TRANSLATE(copy_src, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            _TRANSLATE(copy_dst, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            _TRANSLATE(index, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            _TRANSLATE(vertex, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            _TRANSLATE(uniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            _TRANSLATE(storage, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            _TRANSLATE(indirect, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
#undef _TRANSLATE
            return flags;
        }

        VkShaderStageFlags translateShaderStage(ShaderStage stage) {
            VkShaderStageFlags flags = 0;
#define _TRANSLATE(x, y)             \
    if (stage.has(ShaderStage::x)) { \
        flags |= y;                  \
    }
            _TRANSLATE(vertex, VK_SHADER_STAGE_VERTEX_BIT);
            _TRANSLATE(fragment, VK_SHADER_STAGE_FRAGMENT_BIT);
            _TRANSLATE(compute, VK_SHADER_STAGE_COMPUTE_BIT);
#undef _TRANSLATE
            return flags;
        }

        VkDescriptorType translateBindEntryKind(BindEntryKind kind) {
            switch (kind) {
            case BindEntryKind::uniform_buffer:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case BindEntryKind::storage_buffer:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case BindEntryKind::sample_texture:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case BindEntryKind::storage_texture:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            default:
                spargel_panic_here();
            }
        }

    }  // namespace

    base::unique_ptr<Device> make_device_vulkan() {
        return base::make_unique<DeviceVulkan>();
    }

    void VulkanProcTable::loadGeneralProcs(
        base::dynamic_library_handle* library) {
        // Use `this->` to avoid accidental collision with <vulkan/vulkan.h>.
        this->vkGetInstanceProcAddr =
            reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                base::get_proc_address(library, "vkGetInstanceProcAddr"));
#define VULKAN_GENERAL_PROC(name)              \
    this->name = reinterpret_cast<PFN_##name>( \
        this->vkGetInstanceProcAddr(nullptr, #name));
#include "spargel/gpu/vulkan_procs.inc"
#undef VULKAN_GENERAL_PROC
    }

    void VulkanProcTable::loadInstanceProcs(VkInstance instance) {
#define VULKAN_INSTANCE_PROC(name)             \
    this->name = reinterpret_cast<PFN_##name>( \
        this->vkGetInstanceProcAddr(instance, #name));
#include "spargel/gpu/vulkan_procs.inc"
#undef VULKAN_INSTANCE_PROC
    }

    void VulkanProcTable::loadDeviceProcs(VkDevice device) {
#define VULKAN_DEVICE_PROC(name)               \
    this->name = reinterpret_cast<PFN_##name>( \
        this->vkGetDeviceProcAddr(device, #name));
#include "spargel/gpu/vulkan_procs.inc"
#undef VULKAN_DEVICE_PROC
    }

    class InstanceBuilder {
    public:
        InstanceBuilder(DeviceVulkan* ctx)
            : _ctx{ctx}, _procs{ctx->getProcTable()} {
            // VK_KHR_portability_enumeration
            //
            // This extension allows applications to control whether devices
            // that expose the VK_KHR_portability_subset extension are included
            // in the results of physical device enumeration. Since devices
            // which support the VK_KHR_portability_subset extension are not
            // fully conformant Vulkan implementations, the Vulkan loader does
            // not report those devices unless the application explicitly asks
            // for them. This prevents applications which may not be aware of
            // non-conformant devices from accidentally using them, as any
            // device which supports the VK_KHR_portability_subset extension
            // mandates that the extension must be enabled if that device is
            // used.
            //

            // FIXME
#if SPARGEL_IS_MACOS
            addExtension("VK_KHR_portability_enumeration", false,
                         &_has_portability_enumeration);
#endif
        }

        InstanceBuilder& addLayer(char const* name) {
            _want_layers.emplace(name, true, nullptr);
            return *this;
        }
        InstanceBuilder& addLayer(char const* name, bool required,
                                  bool* enabled) {
            _want_layers.emplace(name, required, enabled);
            return *this;
        }
        InstanceBuilder& addExtension(char const* name) {
            _want_exts.emplace(name, false, nullptr);
            return *this;
        }
        InstanceBuilder& addExtension(char const* name, bool required,
                                      bool* enabled) {
            _want_exts.emplace(name, required, enabled);
            return *this;
        }

        void build() {
            enumerateLayers();
            enumerateExtensions();
            selectLayers();
            selectExtensions();
            createInstance();

            _ctx->_instance = _instance;
            _ctx->_instance_extensions.portability_enumeration =
                _has_portability_enumeration;
        }

    private:
        struct LayerRequest {
            char const* name;
            bool required;
            bool* enabled;
        };
        struct ExtensionRequest {
            char const* name;
            bool required;
            bool* enabled;
        };

        void enumerateLayers() {
            u32 count;
            CHECK_VK_RESULT(
                _procs->vkEnumerateInstanceLayerProperties(&count, nullptr));
            _layers.reserve(count);
            CHECK_VK_RESULT(_procs->vkEnumerateInstanceLayerProperties(
                &count, _layers.data()));
            _layers.set_count(count);
        }

        void enumerateExtensions() {
            // When pLayerName parameter is NULL, only extensions provided by
            // the Vulkan implementation or by implicitly enabled layers are
            // returned. When pLayerName is the name of a layer, the instance
            // extensions provided by that layer are returned.

            u32 count;
            CHECK_VK_RESULT(_procs->vkEnumerateInstanceExtensionProperties(
                /* pLayerName = */ nullptr, &count, nullptr));
            _exts.reserve(count);
            CHECK_VK_RESULT(_procs->vkEnumerateInstanceExtensionProperties(
                /* pLayerName = */ nullptr, &count, _exts.data()));
            _exts.set_count(count);
        }

        void selectLayers() {
            for (usize i = 0; i < _want_layers.count(); i++) {
                auto const& req = _want_layers[i];
                bool found = false;
                for (usize j = 0; j < _layers.count(); j++) {
                    if (strcmp(_layers[j].layerName, req.name) == 0) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    _use_layers.emplace(req.name);
                    spargel_log_info("using layer %s", req.name);
                } else if (req.required) {
                    spargel_log_fatal(
                        "layer %s is required but cannot be found", req.name);
                    spargel_panic_here();
                }
                if (req.enabled != nullptr) {
                    *(req.enabled) = found;
                }
            }
        }

        void selectExtensions() {
            for (usize i = 0; i < _want_exts.count(); i++) {
                auto const& req = _want_exts[i];
                bool found = false;
                for (usize j = 0; j < _exts.count(); j++) {
                    if (strcmp(_exts[j].extensionName, req.name) == 0) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    _use_exts.emplace(req.name);
                    spargel_log_info("using extension %s", req.name);
                } else if (req.required) {
                    spargel_log_fatal(
                        "extension %s is required but cannot be found",
                        req.name);
                    spargel_panic_here();
                }
                if (req.enabled != nullptr) {
                    *(req.enabled) = found;
                }
            }
        }

        void createInstance() {
            VkApplicationInfo app_info;
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pNext = nullptr;
            app_info.pApplicationName = nullptr;
            app_info.applicationVersion = 0;
            app_info.pEngineName = "Spargel Engine";
            app_info.engineVersion = 0;
            // We need at least Vulkan 1.1, as subgroups are available only from
            // Vulkan 1.1.
            app_info.apiVersion = VK_API_VERSION_1_2;

            VkInstanceCreateInfo info;
            info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.pApplicationInfo = &app_info;
            info.enabledLayerCount = (u32)_use_layers.count();
            info.ppEnabledLayerNames = _use_layers.data();
            info.enabledExtensionCount = (u32)_use_exts.count();
            info.ppEnabledExtensionNames = _use_exts.data();

            // VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR specifies that
            // the instance will enumerate available Vulkan
            // Portability-compliant physical devices and groups in addition to
            // the Vulkan physical devices and groups that are enumerated by
            // default.
            //
            if (_has_portability_enumeration) {
                info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            }

            CHECK_VK_RESULT(
                _procs->vkCreateInstance(&info, nullptr, &_instance));
        }

        DeviceVulkan* _ctx;
        VulkanProcTable const* _procs;
        base::vector<VkLayerProperties> _layers;
        base::vector<VkExtensionProperties> _exts;
        base::vector<LayerRequest> _want_layers;
        base::vector<ExtensionRequest> _want_exts;
        base::vector<char const*> _use_layers;
        base::vector<char const*> _use_exts;
        bool _has_portability_enumeration = false;
        VkInstance _instance;
    };

    class PhysicalDeviceBuilder {
    public:
        PhysicalDeviceBuilder(DeviceVulkan* ctx)
            : _ctx{ctx},
              _procs{ctx->getProcTable()},
              _instance{ctx->getVkInstance()} {}

        void build() {
            enumeratePhysicalDevices();
            queryPhysicalDeviceProperties();
            queryPhysicalDeviceFeatures();
            choosePhysicalDevice();
            spargel_log_info("chosing physical device %zu : %s", _index,
                             _physical_device_properties[_index].deviceName);
            fillInfo();
        }

    private:
        void enumeratePhysicalDevices() {
            u32 count;
            CHECK_VK_RESULT(
                _procs->vkEnumeratePhysicalDevices(_instance, &count, nullptr));
            _physical_devices.reserve(count);
            CHECK_VK_RESULT(_procs->vkEnumeratePhysicalDevices(
                _instance, &count, _physical_devices.data()));
            _physical_devices.set_count(count);
            _count = count;
        }

        void queryPhysicalDeviceProperties() {
            _physical_device_properties.reserve(_count);
            _physical_device_properties.set_count(_count);
            for (usize i = 0; i < _count; i++) {
                _procs->vkGetPhysicalDeviceProperties(
                    _physical_devices[i], &_physical_device_properties[i]);
            }
        }

        void queryPhysicalDeviceFeatures() {
            _physical_device_features.reserve(_count);
            _physical_device_features.set_count(_count);
            for (usize i = 0; i < _count; i++) {
                _procs->vkGetPhysicalDeviceFeatures(
                    _physical_devices[i], &_physical_device_features[i]);
            }
        }

        // How to choose a good physical device?
        // 0. TODO: If the user has a preference, use it.
        // 1. Otherwise, filter out all devices with required features.
        // 2. Sort the devices.
        // 2.1. Discrete > Integrated
        // 2.2. TODO: Sort by memory.
        //
        // TODO: When to check presentation support?
        //
        void choosePhysicalDevice() {
            if (tryWithType(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) return;
            if (tryWithType(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) return;
            spargel_log_fatal("cannot find suitable gpu");
            spargel_panic_here();
        }

        bool tryWithType(VkPhysicalDeviceType type) {
            for (usize i = 0; i < _physical_device_properties.count(); i++) {
                auto const& prop = _physical_device_properties[i];
                if (prop.deviceType == type) {
                    _index = i;
                    return true;
                }
            }
            return false;
        }

        void fillInfo() {
            _ctx->_physical_device = _physical_devices[_index];
            _ctx->_physical_device_info.max_memory_allocation_count =
                _physical_device_properties[_index]
                    .limits.maxMemoryAllocationCount;
        }

        DeviceVulkan* _ctx;
        VulkanProcTable const* _procs;
        VkInstance _instance;
        // number of physical devices
        usize _count;
        base::vector<VkPhysicalDevice> _physical_devices;
        base::vector<VkPhysicalDeviceProperties> _physical_device_properties;
        base::vector<VkPhysicalDeviceFeatures> _physical_device_features;
        usize _index = 0;
    };

    class DeviceBuilder {
    public:
        DeviceBuilder(DeviceVulkan* ctx)
            : _ctx{ctx},
              _procs{ctx->getProcTable()},
              _instance{ctx->getVkInstance()},
              _physical_device{ctx->getVkPhysicalDevice()} {}

        void build() {
            enumerateExtensions();
            selectExtensions();
            queryQueueFamilyProperties();
            selectQueues();
            createDevice();
            _ctx->_device = _device;
            _ctx->_queue_family_index = _queue_family_index;
        }

        DeviceBuilder& addExtension(char const* name) {
            _want_exts.emplace(name, false, nullptr);
            return *this;
        }
        DeviceBuilder& addExtension(char const* name, bool required,
                                    bool* enabled) {
            _want_exts.emplace(name, required, enabled);
            return *this;
        }

    private:
        struct ExtensionRequest {
            char const* name;
            bool required;
            bool* enabled;
        };

        void enumerateExtensions() {
            u32 count;
            CHECK_VK_RESULT(_procs->vkEnumerateDeviceExtensionProperties(
                _physical_device, nullptr, &count, nullptr));
            _exts.reserve(count);
            CHECK_VK_RESULT(_procs->vkEnumerateDeviceExtensionProperties(
                _physical_device, nullptr, &count, _exts.data()));
            _exts.set_count(count);
        }

        void selectExtensions() {
            for (usize i = 0; i < _want_exts.count(); i++) {
                auto const& req = _want_exts[i];
                bool found = false;
                for (usize j = 0; j < _exts.count(); j++) {
                    if (strcmp(_exts[j].extensionName, req.name) == 0) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    _use_exts.emplace(req.name);
                    spargel_log_info("using extension %s", req.name);
                } else if (req.required) {
                    spargel_log_fatal(
                        "extension %s is required but cannot be found",
                        req.name);
                    spargel_panic_here();
                }
                if (req.enabled != nullptr) {
                    *(req.enabled) = found;
                }
            }
        }

        void queryQueueFamilyProperties() {
            u32 count;
            _procs->vkGetPhysicalDeviceQueueFamilyProperties(_physical_device,
                                                             &count, nullptr);
            _queue_families.reserve(count);
            _procs->vkGetPhysicalDeviceQueueFamilyProperties(
                _physical_device, &count, _queue_families.data());
            _queue_families.set_count(count);
        }

        // How to select queue families?
        //
        // Spec:
        //
        // The general expectation is that a physical device groups all queues
        // of matching capabilities into a single family. However, while
        // implementations should do this, it is possible that a physical device
        // may return two separate queue families with the same capabilities.
        //
        //
        // If an implementation exposes any queue family that supports graphics
        // operations, at least one queue family of at least one physical device
        // exposed by the implementation must support both graphics and compute
        // operations.
        //
        //
        // All commands that are allowed on a queue that supports transfer
        // operations are also allowed on a queue that supports either graphics
        // or compute operations. Thus, if the capabilities of a queue family
        // include VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT, then reporting
        // the VK_QUEUE_TRANSFER_BIT capability separately for that queue family
        // is optional.
        //
        //
        // Not all physical devices will include WSI support. Within a physical
        // device, not all queue families will support presentation.
        //
        //
        // Discussion:
        //
        // We need one graphics queue. So we can choose the family that supports
        // both graphics and compute.
        //
        // For simplicity, we require that the queue also supports presentation.
        //
        // TODO: async compute and multi-queue
        //
        void selectQueues() {
            spargel_log_info("queue families: %zu", _queue_families.count());

            static constexpr VkQueueFlags queue_flags =
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

            bool found = false;
            for (usize i = 0; i < _queue_families.count(); i++) {
                auto const& prop = _queue_families[i];
                if (prop.queueFlags & queue_flags) {
                    found = true;
                    _queue_family_index = (u32)i;
                    break;
                }
            }

            if (!found) {
                spargel_log_fatal("cannot find a suitable queue family");
                spargel_panic_here();
            }

            _queue_info_count = 1;

            _queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            _queue_infos[0].pNext = nullptr;
            _queue_infos[0].flags = 0;
            _queue_infos[0].queueFamilyIndex = _queue_family_index;
            _queue_infos[0].queueCount = 1;
            _queue_infos[0].pQueuePriorities = vulkan_queue_priorities;

            spargel_log_info("chosing queue family %u", _queue_family_index);
        }

        void createDevice() {
            VkDeviceCreateInfo info;
            info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.queueCreateInfoCount = _queue_info_count;
            info.pQueueCreateInfos = _queue_infos;
            info.enabledLayerCount = 0;
            info.ppEnabledLayerNames = 0;
            info.enabledExtensionCount = (u32)_use_exts.count();
            info.ppEnabledExtensionNames = _use_exts.data();
            info.pEnabledFeatures = 0;

            CHECK_VK_RESULT(_procs->vkCreateDevice(_physical_device, &info,
                                                   nullptr, &_device));
        }

        DeviceVulkan* _ctx;
        VulkanProcTable const* _procs;
        VkInstance _instance;
        VkPhysicalDevice _physical_device;

        base::vector<VkExtensionProperties> _exts;
        base::vector<ExtensionRequest> _want_exts;
        base::vector<char const*> _use_exts;

        base::vector<VkQueueFamilyProperties> _queue_families;

        VkDeviceQueueCreateInfo _queue_infos[4];
        u32 _queue_info_count = 0;

        u32 _queue_family_index = 0;

        VkDevice _device;
    };

    DeviceVulkan::DeviceVulkan() : Device(DeviceKind::vulkan), _dalloc(this) {
        _library = base::open_dynamic_library(vulkan_library_name);
        if (_library == nullptr) {
            spargel_log_fatal("Cannot load the Vulkan loader.");
            spargel_panic_here();
        }

        _procs.loadGeneralProcs(_library);
        createInstance();
        _procs.loadInstanceProcs(_instance);

        if (_instance_extensions.debug_utils) {
            createDebugMessenger();
        }

        selectPhysicalDevice();
        createDevice();
        _procs.loadDeviceProcs(_device);

        getQueue();
        queryMemoryInfo();
        createDescriptorPool();
        createMemoryPool();
    }

    DeviceVulkan::~DeviceVulkan() { base::close_dynamic_library(_library); }

    void DeviceVulkan::createInstance() {
        InstanceBuilder(this)
            .addLayer("VK_LAYER_KHRONOS_validation")
            .addExtension("VK_KHR_surface", true, nullptr)
            .addExtension("VK_EXT_debug_utils", false,
                          &_instance_extensions.debug_utils)
#if SPARGEL_IS_ANDROID
            .addExtension("VK_KHR_android_surface", true, nullptr)
#endif
#if SPARGEL_IS_LINUX
            .addExtension("VK_KHR_wayland_surface", true, nullptr)
            .addExtension("VK_KHR_xcb_surface", true, nullptr)
#endif
#if SPARGEL_IS_MACOS
            .addExtension("VK_EXT_metal_surface", true, nullptr)
#endif
#if SPARGEL_IS_WINDOWS
            .addExtension("VK_KHR_win32_surface", true, nullptr)
#endif
            .build();
    }

    void DeviceVulkan::createDebugMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT info;
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        info.pNext = nullptr;
        info.flags = 0;
        info.messageSeverity =
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = onDebugUtilsMessage;
        info.pUserData = nullptr;

        CHECK_VK_RESULT(_procs.vkCreateDebugUtilsMessengerEXT(
            _instance, &info, nullptr, &_debug_messenger));
    }

    void DeviceVulkan::selectPhysicalDevice() {
        PhysicalDeviceBuilder(this).build();
    }

    void DeviceVulkan::createDevice() {
        auto builder = DeviceBuilder(this);
        builder.addExtension("VK_KHR_swapchain", true, nullptr);
        if (_instance_extensions.portability_enumeration) {
            builder.addExtension("VK_KHR_portability_subset", true, nullptr);
        }
        builder.build();
    }

    void DeviceVulkan::getQueue() {
        _procs.vkGetDeviceQueue(_device, _queue_family_index, 0, &_queue);
    }

    void DeviceVulkan::queryMemoryInfo() {
        _procs.vkGetPhysicalDeviceMemoryProperties(_physical_device,
                                                   &_memory_props);
        for (u32 i = 0; i < _memory_props.memoryHeapCount; i++) {
            [[maybe_unused]] auto const& heap = _memory_props.memoryHeaps[i];
            // spargel_log_info("memory heap [%u] : flags = %u, size = %lu", i,
            // heap.flags, heap.size);
        }
        for (u32 i = 0; i < _memory_props.memoryTypeCount; i++) {
            auto const& type = _memory_props.memoryTypes[i];
            spargel_log_info("memory type [%u] : flags = %u, heap = %u", i,
                             type.propertyFlags, type.heapIndex);
        }

        bool found = false;
        for (u32 i = 0; i < _memory_props.memoryTypeCount; i++) {
            auto const& type = _memory_props.memoryTypes[i];
            // auto const& heap = _memory_props.memoryHeaps[type.heapIndex];
            if ((type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
                (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                found = true;
                _memory_type = i;
                break;
            }
        }

        spargel_assert(found);
        spargel_log_info("chosen memory type = %u", _memory_type);

        // The memory heap flags is either 0 or VK_MEMORY_HEAP_DEVICE_LOCAL_BIT.
        //
        // General situation:
        //
        // Case 1: unified
        //   One device local heap. All memory types are device local and host
        //   visible.
        //
        // Case 2: separated
        //   One device local heap, the memory types of which are not host
        //   visible. Another heap for host visible memory.
        //
        //   It is possible (and more common) that there is another device local
        //   heap that exposes host visible memory. The size is 256MB (AMD).
        //   Larger values require hardware configuration (resizable bar), which
        //   is not popular.
        //
        // Case 3: weird (AMD integrated, old) (new hardware belongs to case 2)
        //   One heap for host visible memory.
        //   Another device local heap (256MB) with both host visible and non
        //   host visible.
        //
        // Examples:
        //
        // NVIDIA:
        //   (4090 Super)
        //   Heap 0: device local
        //     Type 0: device local
        //     Type 1: device local, host visible, host coherent
        //   Heap 1:
        //     Type 0:
        //     Type 1: host visible, host coherent
        //     Type 2: host visible, host coherent, host cached
        //
        //   (3050 Laptop)
        //   Heap 0: device local
        //     Type 0: device local
        //   Heap 1:
        //     Type 0:
        //     Type 1: host visible, host coherent
        //     Type 2: host visible, host coherent, host cached
        //   Heap 2:
        //     Type 0: device local, host visible, host coherent
        //
        // AMD:
        //   (RX 6900 XT)
        //   Heap 0: device local
        //     Type 0: device local
        //     Type 1: device local, amd specific
        //     Type 2: device local
        //     Type 3: device local, amd specific
        //   Heap 1:
        //     Type 0: host visible, host coherent
        //     Type 1: host visible, host coherent, host cached
        //     Type 2: host visible, host coherent, amd specific
        //     Type 3: host visible, host coherent, host cached, amd specific
        //     Type 4: host visible, host coherent
        //     Type 5: host visible, host coherent, host cached
        //     Type 6: host visible, host coherent, amd specific
        //     Type 7: host visible, host coherent, host cached, amd specific
        //   Heap 2: device local
        //     Type 0: device local, host visible, host coherent
        //     Type 1: device local, host visible, host coherent, amd specific
        //     Type 2: device local, host visible, host coherent
        //     Type 3: device local, host visible, host coherent, amd specific
    }

    ShaderLibrary* DeviceVulkan::createShaderLibrary(base::span<u8> bytes) {
        VkShaderModuleCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.codeSize = bytes.count();
        info.pCode = (u32 const*)bytes.data();

        VkShaderModule shader;
        CHECK_VK_RESULT(
            _procs.vkCreateShaderModule(_device, &info, nullptr, &shader));
        return make_object<ShaderLibraryVulkan>(shader);
    }

    RenderPipeline* DeviceVulkan::createRenderPipeline(
        RenderPipelineDescriptor const& descriptor) {
        return nullptr;
    }

    void DeviceVulkan::createMemoryPool() {
        VkMemoryAllocateInfo info;
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.pNext = nullptr;
        info.allocationSize = pool_size;
        info.memoryTypeIndex = _memory_type;

        CHECK_VK_RESULT(
            _procs.vkAllocateMemory(_device, &info, nullptr, &_memory_pool));
        CHECK_VK_RESULT(_procs.vkMapMemory(_device, _memory_pool, 0, pool_size,
                                           0, &_pool_addr));
    }

    usize DeviceVulkan::allocateMemory(usize size, usize align) {
        spargel_log_info(
            "current offset = %zu, request size = %zu, request align = %zu",
            _pool_offset, size, align);
        usize addr = (_pool_offset + align - 1) & (-align);
        spargel_log_info("adjusted offset = %zu", addr);
        spargel_assert(addr + size < pool_size);
        _pool_offset = addr + size;
        return addr;
    }

    // VulkanMemoryAllocator:
    // - BlockVector indexed by memoryType
    // - A BlockVector is a sequence of MemoryBlock. It represents memory blocks
    // allocated for a
    //   specific memory type.
    // - A MemoryBlock is simply a VkDeviceMemory, i.e. an allocation. So we
    // should have at most 4096 blocks.

    Buffer* DeviceVulkan::createBuffer(BufferUsage usage,
                                       base::span<u8> bytes) {
        auto b = createBuffer(usage, bytes.count());
        auto addr = b->mapAddr();
        memcpy(addr, bytes.data(), bytes.count());
        return b;
    }

    Buffer* DeviceVulkan::createBuffer(BufferUsage usage, u32 size) {
        // Step 1. Create the buffer object.

        VkBufferCreateInfo buffer_info;
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.pNext = nullptr;
        // TODO: sparse
        buffer_info.flags = 0;
        buffer_info.size = size;
        buffer_info.usage = translateBufferUsage(usage);
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_info.queueFamilyIndexCount = 0;
        buffer_info.pQueueFamilyIndices = nullptr;

        VkBuffer buffer;
        CHECK_VK_RESULT(
            _procs.vkCreateBuffer(_device, &buffer_info, nullptr, &buffer));

        // Step 2. Memory requirements

        VkMemoryRequirements reqs;
        _procs.vkGetBufferMemoryRequirements(_device, buffer, &reqs);

        spargel_log_info("allowed memory types for buffer: %u",
                         reqs.memoryTypeBits);

        spargel_assert(reqs.memoryTypeBits & _memory_type);

        auto offset = allocateMemory(reqs.size, reqs.alignment);

        _procs.vkBindBufferMemory(_device, buffer, _memory_pool, offset);

        return make_object<BufferVulkan>(this, buffer, offset, size);
    }

    Surface* DeviceVulkan::createSurface(ui::Window* w) { return nullptr; }

    Texture* DeviceVulkan::createTexture(u32 width, u32 height) {
        return nullptr;
    }

    void DeviceVulkan::destroyTexture(Texture* texture) {}

    void DeviceVulkan::destroyShaderLibrary(ShaderLibrary* library) {}

    void DeviceVulkan::destroyRenderPipeline(RenderPipeline* pipeline) {}

    void DeviceVulkan::destroyBuffer(Buffer* b) {}

    CommandQueue* DeviceVulkan::createCommandQueue() {
        return make_object<CommandQueueVulkan>(_queue, this);
    }

    void DeviceVulkan::destroyCommandQueue(CommandQueue* q) {}

    ComputePipeline* DeviceVulkan::createComputePipeline(
        ShaderFunction func, base::span<BindGroupLayout*> layouts) {
        base::vector<VkDescriptorSetLayout> set_layouts;
        set_layouts.reserve(layouts.count());
        set_layouts.set_count(layouts.count());
        for (usize i = 0; i < layouts.count(); i++) {
            set_layouts[i] = layouts[i].cast<BindGroupLayoutVulkan>()->layout();
        }

        VkPipelineLayoutCreateInfo layout_info;
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.pNext = nullptr;
        layout_info.flags = 0;
        layout_info.setLayoutCount = (u32)set_layouts.count();
        layout_info.pSetLayouts = set_layouts.data();
        layout_info.pushConstantRangeCount = 0;
        layout_info.pPushConstantRanges = nullptr;

        VkPipelineLayout layout;
        CHECK_VK_RESULT(_procs.vkCreatePipelineLayout(_device, &layout_info,
                                                      nullptr, &layout));

        VkComputePipelineCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage.pNext = nullptr;
        info.stage.flags = 0;
        info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module = func.library.cast<ShaderLibraryVulkan>()->library();
        info.stage.pName = func.entry;
        info.stage.pSpecializationInfo = nullptr;
        info.layout = layout;
        info.basePipelineHandle = VK_NULL_HANDLE;
        info.basePipelineIndex = 0;

        VkPipeline pipeline;
        CHECK_VK_RESULT(_procs.vkCreateComputePipelines(
            _device, /* cache = */ VK_NULL_HANDLE, 1, &info, nullptr,
            &pipeline));
        return make_object<ComputePipelineVulkan>(pipeline, layout);
    }

    BindGroupLayout* DeviceVulkan::createBindGroupLayout(
        ShaderStage stage, base::span<BindEntry> entries) {
        auto stage_flag = translateShaderStage(stage);
        base::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(entries.count());
        bindings.set_count(entries.count());
        for (usize i = 0; i < entries.count(); i++) {
            auto const& entry = entries[i];
            auto& binding = bindings[i];
            binding.binding = entry.binding;
            binding.descriptorType = translateBindEntryKind(entry.kind);
            binding.descriptorCount = 1;
            binding.stageFlags = stage_flag;
            binding.pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.bindingCount = bindings.count();
        info.pBindings = bindings.data();

        VkDescriptorSetLayout layout;
        CHECK_VK_RESULT(_procs.vkCreateDescriptorSetLayout(_device, &info,
                                                           nullptr, &layout));

        return make_object<BindGroupLayoutVulkan>(layout);
    }

    void DeviceVulkan::createDescriptorPool() {
        VkDescriptorPoolSize sizes[4];
        sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sizes[0].descriptorCount = 128;
        sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        sizes[1].descriptorCount = 128;
        sizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sizes[2].descriptorCount = 128;
        sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        sizes[3].descriptorCount = 128;

        VkDescriptorPoolCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        // TODO
        info.maxSets = 128;
        info.poolSizeCount = 4;
        info.pPoolSizes = sizes;

        CHECK_VK_RESULT(
            _procs.vkCreateDescriptorPool(_device, &info, nullptr, &_dpool));
    }

    CommandQueueVulkan::CommandQueueVulkan(VkQueue queue, DeviceVulkan* device)
        : _device{device}, _procs{device->getProcTable()}, _queue{queue} {
        createCommandPool();
    }

    CommandBuffer* CommandQueueVulkan::createCommandBuffer() {
        VkCommandBufferAllocateInfo info;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext = nullptr;
        info.commandPool = _pool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = 1;

        VkCommandBuffer buffer;
        CHECK_VK_RESULT(_procs->vkAllocateCommandBuffers(_device->device(),
                                                         &info, &buffer));
        return make_object<CommandBufferVulkan>(_device, this, buffer);
    }

    void CommandQueueVulkan::destroyCommandBuffer(CommandBuffer*) {}

    void CommandQueueVulkan::createCommandPool() {
        VkCommandPoolCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.queueFamilyIndex = _device->getQueueFamilyIndex();

        CHECK_VK_RESULT(_procs->vkCreateCommandPool(_device->device(), &info,
                                                    nullptr, &_pool));
    }

    CommandBufferVulkan::CommandBufferVulkan(DeviceVulkan* device,
                                             CommandQueueVulkan* queue,
                                             VkCommandBuffer cmdbuf)
        : _device{device},
          _queue{queue},
          _procs{device->getProcTable()},
          _cmdbuf{cmdbuf} {
        createFence();
        beginCommandBuffer();
    }

    RenderPassEncoder* CommandBufferVulkan::beginRenderPass(
        RenderPassDescriptor const& descriptor) {
        return nullptr;
    }

    void CommandBufferVulkan::endRenderPass(RenderPassEncoder* encoder) {}

    ComputePassEncoder* CommandBufferVulkan::beginComputePass() {
        return make_object<ComputePassEncoderVulkan>(_device, _cmdbuf);
    }

    void CommandBufferVulkan::endComputePass(ComputePassEncoder* encoder) {}

    void CommandBufferVulkan::present(Surface* surface) {}

    void CommandBufferVulkan::submit() {
        endCommandBuffer();

        VkSubmitInfo info;
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pNext = 0;
        info.waitSemaphoreCount = 0;
        info.pWaitSemaphores = 0;
        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        info.pWaitDstStageMask = &stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &_cmdbuf;
        info.signalSemaphoreCount = 0;
        info.pSignalSemaphores = nullptr;
        CHECK_VK_RESULT(
            _procs->vkQueueSubmit(_queue->commandQueue(), 1, &info, _fence));
    }

    void CommandBufferVulkan::wait() {
        CHECK_VK_RESULT(_procs->vkWaitForFences(_device->device(), 1, &_fence,
                                                VK_TRUE, UINT64_MAX));
        // _procs->vkDeviceWaitIdle(_device->device());
    }

    void CommandBufferVulkan::beginCommandBuffer() {
        VkCommandBufferBeginInfo info;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.pNext = nullptr;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        info.pInheritanceInfo = nullptr;
        CHECK_VK_RESULT(_procs->vkBeginCommandBuffer(_cmdbuf, &info));
    }

    void CommandBufferVulkan::endCommandBuffer() {
        CHECK_VK_RESULT(_procs->vkEndCommandBuffer(_cmdbuf));
    }

    void CommandBufferVulkan::createFence() {
        VkFenceCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = 0;
        info.flags = 0;
        CHECK_VK_RESULT(
            _procs->vkCreateFence(_device->device(), &info, 0, &_fence));
    }

    ComputePassEncoderVulkan::ComputePassEncoderVulkan(DeviceVulkan* device,
                                                       VkCommandBuffer cmdbuf)
        : /* _device{device}, */
          _procs{device->getProcTable()},
          _cmdbuf{cmdbuf} {}

    void ComputePassEncoderVulkan::setComputePipeline(
        ComputePipeline* pipeline) {
        _procs->vkCmdBindPipeline(
            _cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeline.cast<ComputePipelineVulkan>()->pipeline());
    }

    ComputePipeline2Vulkan::ComputePipeline2Vulkan(
        DeviceVulkan* device, ShaderFunction compute,
        base::span<PipelineArgumentGroup> groups)
        : _device{device}, _procs{device->getProcTable()} {
        createDescriptorSetLayouts(groups);
        createPipelineLayout();
        createPipeline(compute);
    }

    void ComputePipeline2Vulkan::createDescriptorSetLayouts(
        base::span<PipelineArgumentGroup> groups) {
        VkDescriptorSetLayoutCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;

        base::vector<VkDescriptorSetLayoutBinding> bindings;

        // validate shader stage
        for (usize i = 0; i < groups.count(); i++) {
            auto const& group = groups[i];
            spargel_assert(group.stage == ShaderStage::compute);
        }

        u32 max_loc = 0;
        for (usize i = 0; i < groups.count(); i++) {
            auto const& group = groups[i];
            auto id = group.location.vulkan.set_id;
            _groups.emplace(i, id);
            if (id > max_loc) {
                max_loc = id;
            }
        }

        spargel_log_info("max_loc = %u", max_loc);

        // starting from zero
        _dset_layouts.reserve(max_loc + 1);
        _dset_layouts.set_count(max_loc + 1);

        for (usize i = 0; i < groups.count(); i++) {
            auto const& group = groups[i];

            bindings.clear();
            for (usize j = 0; j < group.arguments.count(); j++) {
                auto const& arg = group.arguments[j];
                bindings.emplace(
                    /* binding = */ arg.id,
                    /* descriptorType = */ translateBindEntryKind(arg.kind),
                    /* descriptorCount = */ 1,
                    /* stageFlags = */ VK_SHADER_STAGE_COMPUTE_BIT,
                    /* pImmutableSamplers = */ nullptr);
                _args.emplace(i, arg.id, arg.kind);
            }

            info.bindingCount = (u32)bindings.count();
            info.pBindings = bindings.data();

            VkDescriptorSetLayout layout;
            CHECK_VK_RESULT(_procs->vkCreateDescriptorSetLayout(
                _device->device(), &info, nullptr, &layout));
            _dset_layouts[group.location.vulkan.set_id] = layout;
        }
    }

    void ComputePipeline2Vulkan::createPipelineLayout() {
        VkPipelineLayoutCreateInfo layout_info;
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.pNext = nullptr;
        layout_info.flags = 0;
        layout_info.setLayoutCount = (u32)_dset_layouts.count();
        layout_info.pSetLayouts = _dset_layouts.data();
        layout_info.pushConstantRangeCount = 0;
        layout_info.pPushConstantRanges = nullptr;

        CHECK_VK_RESULT(_procs->vkCreatePipelineLayout(
            _device->device(), &layout_info, nullptr, &_layout));
        spargel_log_info("pipeline layout created");
    }

    void ComputePipeline2Vulkan::createPipeline(ShaderFunction compute) {
        VkComputePipelineCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage.pNext = nullptr;
        info.stage.flags = 0;
        info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module =
            compute.library.cast<ShaderLibraryVulkan>()->library();
        info.stage.pName = compute.entry;
        info.stage.pSpecializationInfo = nullptr;
        info.layout = _layout;
        info.basePipelineHandle = VK_NULL_HANDLE;
        info.basePipelineIndex = 0;

        CHECK_VK_RESULT(_procs->vkCreateComputePipelines(
            _device->device(), /* pipelineCache = */ VK_NULL_HANDLE,
            /* createInfoCount = */ 1, &info, nullptr, &_pipeline));
    }

    BindGroup* DeviceVulkan::createBindGroup2(ComputePipeline2* p, u32 id) {
        return p.cast<ComputePipeline2Vulkan>()->createBindGroup2(id);
    }

    BindGroupVulkan* ComputePipeline2Vulkan::createBindGroup2(u32 id) {
        auto set_id = getSetId(id);
        auto set_layout = _dset_layouts[set_id];

        VkDescriptorSetAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = _device->getDescriptorPool();
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &set_layout;

        VkDescriptorSet dset;
        CHECK_VK_RESULT(_procs->vkAllocateDescriptorSets(_device->device(),
                                                         &alloc_info, &dset));

        return make_object<BindGroupVulkan>(dset, _device, this, id);
    }

    VkDescriptorType ComputePipeline2Vulkan::getDescriptorType(u32 id,
                                                               u32 binding) {
        for (usize i = 0; i < _args.count(); i++) {
            if (_args[i].id == id && _args[i].binding == binding) {
                return translateBindEntryKind(_args[i].kind);
            }
        }
        spargel_panic_here();
    }

    // Spec:
    //
    // Each of the pDescriptorSets must be compatible with the pipeline layout
    // specified by layout. The layout used to program the bindings must also be
    // compatible with the pipeline used in subsequent bound pipeline commands
    // with that pipeline type, as defined in the Pipeline Layout Compatibility
    // section.
    //
    void ComputePassEncoderVulkan::setBindGroup(u32 index, BindGroup* g) {
        auto group = g.cast<BindGroupVulkan>();
        VkDescriptorSet sets[1] = {group->getDescriptorSet()};
        // TODO: where to get pipeline layout
        _procs->vkCmdBindDescriptorSets(_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                        group->pipeline()->layout(), index, 1,
                                        sets, 0, nullptr);
    }

    void ComputePassEncoderVulkan::setBuffer(Buffer* buffer,
                                             VertexBufferLocation const& loc) {}

    void ComputePassEncoderVulkan::setComputePipeline2(
        ComputePipeline2* pipeline) {
        _procs->vkCmdBindPipeline(
            _cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeline.cast<ComputePipeline2Vulkan>()->pipeline());
    }

    void ComputePassEncoderVulkan::dispatch(DispatchSize grid_size,
                                            DispatchSize group_size) {
        _procs->vkCmdDispatch(_cmdbuf, grid_size.x, grid_size.y, grid_size.z);
    }

    BufferVulkan::BufferVulkan(DeviceVulkan* device, VkBuffer buffer,
                               usize offset, usize size)
        : _device{device},
          _procs{device->getProcTable()},
          _buffer{buffer},
          _offset{offset},
          _size{size} {
        _addr = static_cast<void*>(
            static_cast<base::Byte*>(device->getPoolAddr()) + offset);
    }

    void* BufferVulkan::mapAddr() { return _addr; }

    BindGroupVulkan::BindGroupVulkan(VkDescriptorSet set, DeviceVulkan* device,
                                     ComputePipeline2Vulkan* pipeline, u32 id)
        : _set{set},
          _device{device},
          _procs{device->getProcTable()},
          _pipeline{pipeline},
          _id{id} {}

    void BindGroupVulkan::setBuffer(u32 id, Buffer* buffer) {
        VkDescriptorBufferInfo info;
        info.buffer = buffer.cast<BufferVulkan>()->buffer();
        info.offset = 0;
        info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = _set;
        write.dstBinding = id;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = _pipeline->getDescriptorType(_id, id);
        write.pImageInfo = nullptr;
        write.pBufferInfo = &info;
        write.pTexelBufferView = nullptr;
        _procs->vkUpdateDescriptorSets(_device->device(), 1, &write, 0,
                                       nullptr);
    }

    DescriptorAllocator::DescriptorAllocator(DeviceVulkan* device)
        : _device{device}, _procs{device->getProcTable()} {}

    VkDescriptorSet DescriptorAllocator::allocate(
        DescriptorSetShape const& shape) {
        [[maybe_unused]] auto& suballoc =
            _suballocs.getOrConstruct(shape, _device, shape);
        return VK_NULL_HANDLE;
    }

    ShapedDescriptorAllocator::ShapedDescriptorAllocator(
        DeviceVulkan* device, DescriptorSetShape const& shape)
        : _device{device}, _procs{device->getProcTable()} {
        createLayout();
    }

    void ShapedDescriptorAllocator::createLayout() {}

    //         d->queue.backend = BACKEND_VULKAN;

    //         VkQueue queue;
    //         procs->vkGetDeviceQueue(dev, queue_family_index, 0, &queue);

    //         d->queue.queue = queue;

    //         VkCommandPool cmd_pool;
    //         {
    //             VkCommandPoolCreateInfo info;
    //             info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //             info.pNext = 0;
    //             info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //             info.queueFamilyIndex = queue_family_index;
    //             CHECK_VK_RESULT(procs->vkCreateCommandPool(dev, &info, 0,
    //             &cmd_pool));
    //         }
    //         d->cmd_pool = cmd_pool;

    //         *device = (device_id)d;
    //         return RESULT_SUCCESS;
    //     }

    //     void vulkan_destroy_device(device_id device) {
    //         cast_object(vulkan_device, d, device);
    //         struct vulkan_proc_table* procs = &d->procs;

    //         procs->vkDestroyDevice(d->device, 0);
    //         if (d->debug_messenger)
    //             procs->vkDestroyDebugUtilsMessengerEXT(d->instance,
    //             d->debug_messenger, 0);
    //         procs->vkDestroyInstance(d->instance, 0);

    //         base::close_dynamic_library(d->library);

    //         dealloc_object(vulkan_device, d);
    //     }

    //     int vulkan_create_command_queue(device_id device, command_queue_id*
    //     queue) {
    //         cast_object(vulkan_device, d, device);
    //         *queue = (command_queue_id)&d->queue;
    //         return RESULT_SUCCESS;
    //     }

    //     void vulkan_destroy_command_queue(device_id device, command_queue_id
    //     queue) {}

    //     int vulkan_create_shader_function(device_id device,
    //                                       struct
    //                                       vulkan_shader_function_descriptor
    //                                       const* descriptor,
    //                                       shader_function_id* func) {
    //         cast_object(vulkan_device, d, device);
    //         alloc_object(vulkan_shader_function, f);

    //         VkShaderModuleCreateInfo info;
    //         info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    //         info.pNext = 0;
    //         info.flags = 0;
    //         info.codeSize = descriptor->size;
    //         info.pCode = (u32*)descriptor->code;

    //         VkShaderModule mod;
    //         CHECK_VK_RESULT(d->procs.vkCreateShaderModule(d->device, &info,
    //         0, &mod)); f->shader = mod; f->device = d;

    //         *func = (shader_function_id)f;
    //         return RESULT_SUCCESS;
    //     }

    //     void vulkan_destroy_shader_function(device_id device,
    //     shader_function_id func) {
    //         cast_object(vulkan_shader_function, f, func);
    //         struct vulkan_device* d = f->device;
    //         d->procs.vkDestroyShaderModule(d->device, f->shader, 0);
    //         dealloc_object(vulkan_shader_function, f);
    //     }

    //     int vulkan_create_render_pipeline(device_id device,
    //                                       struct RenderPipelineDescriptor
    //                                       const* descriptor,
    //                                       render_pipeline_id* pipeline) {
    //         alloc_object(vulkan_render_pipeline, p);

    //         VkGraphicsPipelineCreateInfo pipeline_info;
    //         pipeline_info.sType =
    //         VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    //         pipeline_info.pNext = 0;
    //         pipeline_info.flags = 0;
    //         pipeline_info.stageCount = 2;

    //         *pipeline = (render_pipeline_id)p;
    //         return RESULT_SUCCESS;
    //     }

    //     void vulkan_destroy_render_pipeline(device_id device,
    //     render_pipeline_id pipeline) {
    //         cast_object(vulkan_render_pipeline, p, pipeline);
    //         dealloc_object(vulkan_render_pipeline, p);
    //     }

    //     int vulkan_create_command_buffer(device_id device,
    //                                      struct command_buffer_descriptor
    //                                      const* descriptor,
    //                                      command_buffer_id* command_buffer) {
    //         cast_object(vulkan_device, d, device);
    //         cast_object(vulkan_command_queue, q, descriptor->queue);
    //         alloc_object(vulkan_command_buffer, cmdbuf);

    //         VkCommandBufferAllocateInfo info;
    //         info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //         info.pNext = 0;
    //         info.commandPool = d->cmd_pool;
    //         info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //         info.commandBufferCount = 1;
    //         CHECK_VK_RESULT(
    //             d->procs.vkAllocateCommandBuffers(d->device, &info,
    //             &cmdbuf->command_buffer));

    //         cmdbuf->queue = q->queue;

    //         *command_buffer = (command_buffer_id)cmdbuf;
    //         return RESULT_SUCCESS;
    //     }

    //     void vulkan_destroy_command_buffer(device_id device,
    //     command_buffer_id command_buffer) {
    //         cast_object(vulkan_command_buffer, c, command_buffer);
    //         dealloc_object(vulkan_command_buffer, c);
    //     }

    //     void vulkan_reset_command_buffer(device_id device, command_buffer_id
    //     command_buffer)
    //     {
    //         cast_object(vulkan_device, d, device);
    //         cast_object(vulkan_command_buffer, c, command_buffer);
    //         CHECK_VK_RESULT(d->procs.vkResetCommandBuffer(c->command_buffer,
    //         0));
    //     }

    //     int vulkan_create_surface(device_id device, struct surface_descriptor
    //     const* descriptor,
    //                               surface_id* surface) {
    //         cast_object(vulkan_device, d, device);
    //         alloc_object(vulkan_surface, s);

    //         ui::window_handle wh = descriptor->window->handle();
    //         VkSurfaceKHR surf;
    // #if SPARGEL_IS_ANDROID
    //         VkAndroidSurfaceCreateInfoKHR info;
    //         info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    //         info.pNext = 0;
    //         info.flags = 0;
    //         info.window = (ANativeWindow*)wh.android.window;
    //         CHECK_VK_RESULT(d->procs.vkCreateAndroidSurfaceKHR(d->instance,
    //         &info, 0, &surf));
    // #endif
    // #if SPARGEL_IS_LINUX /* todo: wayland */
    //         VkXcbSurfaceCreateInfoKHR info;
    //         info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    //         info.pNext = 0;
    //         info.flags = 0;
    //         info.connection = (xcb_connection_t*)wh.xcb.connection;
    //         info.window = (xcb_window_t)wh.xcb.window;
    //         CHECK_VK_RESULT(d->procs.vkCreateXcbSurfaceKHR(d->instance,
    //         &info, 0, &surf));
    // #endif
    // #if SPARGEL_IS_MACOS
    //         VkMetalSurfaceCreateInfoEXT info;
    //         info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    //         info.pNext = 0;
    //         info.flags = 0;
    //         info.pLayer = wh.apple.layer;
    //         CHECK_VK_RESULT(d->procs.vkCreateMetalSurfaceEXT(d->instance,
    //         &info, 0, &surf));
    // #endif
    // #if SPARGEL_IS_WINDOWS
    //         VkWin32SurfaceCreateInfoKHR info;
    //         info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    //         info.pNext = 0;
    //         info.flags = 0;
    //         info.hinstance = (HINSTANCE)wh.win32.hinstance;
    //         info.hwnd = (HWND)wh.win32.hwnd;
    //         CHECK_VK_RESULT(d->procs.vkCreateWin32SurfaceKHR(d->instance,
    //         &info, 0, &surf));
    // #endif

    //         s->surface = surf;

    //         *surface = (surface_id)s;

    //         return RESULT_SUCCESS;
    //     }

    //     void vulkan_destroy_surface(device_id device, surface_id surface) {
    //         cast_object(vulkan_device, d, device);
    //         cast_object(vulkan_surface, s, surface);
    //         d->procs.vkDestroySurfaceKHR(d->instance, s->surface, 0);
    //         dealloc_object(vulkan_surface, s);
    //     }

    //     int vulkan_create_swapchain(device_id device, struct
    //     swapchain_descriptor const* descriptor,
    //                                 swapchain_id* swapchain) {
    //         cast_object(vulkan_device, d, device);
    //         cast_object(vulkan_surface, sf, descriptor->surface);
    //         alloc_object(vulkan_swapchain, sw);

    //         struct vulkan_proc_table* procs = &d->procs;

    //         VkSurfaceCapabilitiesKHR surf_caps;
    //         CHECK_VK_RESULT(procs->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d->physical_device,
    //                                                                          sf->surface,
    //                                                                          &surf_caps));

    //         u32 min_images = surf_caps.minImageCount + 1;
    //         if (surf_caps.maxImageCount > 0 && min_images >
    //         surf_caps.maxImageCount) {
    //             min_images = surf_caps.maxImageCount;
    //         }

    //         base::vector<VkSurfaceFormatKHR> formats;
    //         {
    //             u32 count;
    //             CHECK_VK_RESULT(procs->vkGetPhysicalDeviceSurfaceFormatsKHR(d->physical_device,
    //                                                                         sf->surface,
    //                                                                         &count, 0));
    //             formats.reserve(count);
    //             CHECK_VK_RESULT(procs->vkGetPhysicalDeviceSurfaceFormatsKHR(
    //                 d->physical_device, sf->surface, &count,
    //                 formats.data()));
    //             formats.set_count(count);
    //         }

    //         VkSurfaceFormatKHR* chosen_format = &formats[0];
    //         for (usize i = 0; i < formats.count(); i++) {
    //             VkSurfaceFormatKHR* fmt = &formats[i];
    //             if (fmt->format == VK_FORMAT_B8G8R8A8_SRGB &&
    //                 fmt->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
    //                 chosen_format = fmt;
    //                 break;
    //             }
    //         }

    //         VkSwapchainCreateInfoKHR info;
    //         info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    //         info.pNext = 0;
    //         info.flags = 0;
    //         info.surface = sf->surface;
    //         info.minImageCount = min_images;
    //         info.imageFormat = chosen_format->format;
    //         info.imageColorSpace = chosen_format->colorSpace;
    //         info.imageExtent.width = descriptor->width;
    //         info.imageExtent.height = descriptor->height;
    //         info.imageArrayLayers = 1;
    //         info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    //         info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //         info.queueFamilyIndexCount = 0;
    //         info.pQueueFamilyIndices = 0;
    //         info.preTransform = surf_caps.currentTransform;
    //         info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    //         info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    //         info.clipped = VK_TRUE;
    //         info.oldSwapchain = 0;

    //         CHECK_VK_RESULT(procs->vkCreateSwapchainKHR(d->device, &info, 0,
    //         &sw->swapchain));

    //         {
    //             u32 count;
    //             CHECK_VK_RESULT(procs->vkGetSwapchainImagesKHR(d->device,
    //             sw->swapchain, &count, 0)); sw->images.reserve(count);
    //             sw->image_views.reserve(count);
    //             sw->framebuffers.reserve(count);
    //             CHECK_VK_RESULT(procs->vkGetSwapchainImagesKHR(d->device,
    //             sw->swapchain, &count,
    //                                                            sw->images.data()));
    //             sw->images.set_count(count);
    //             sw->image_views.set_count(count);
    //             sw->framebuffers.set_count(count);
    //         }

    //         auto image_count = sw->images.count();

    //         {
    //             VkImageViewCreateInfo info;
    //             info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    //             info.pNext = 0;
    //             info.flags = 0;
    //             info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    //             info.format = chosen_format->format;
    //             info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    //             info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    //             info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    //             info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    //             info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //             info.subresourceRange.baseMipLevel = 0;
    //             info.subresourceRange.levelCount = 1;
    //             info.subresourceRange.baseArrayLayer = 0;
    //             info.subresourceRange.layerCount = 1;
    //             for (usize i = 0; i < image_count; i++) {
    //                 info.image = sw->images[i];
    //                 CHECK_VK_RESULT(procs->vkCreateImageView(d->device,
    //                 &info, 0, &sw->image_views[i]));
    //             }
    //         }

    //         /* create a dummy render pass */

    //         VkRenderPass rp;
    //         {
    //             VkAttachmentDescription attachment;
    //             attachment.flags = 0;
    //             attachment.format = chosen_format->format;
    //             attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    //             attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //             attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //             attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //             attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //             attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //             attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    //             VkAttachmentReference attachment_ref;
    //             attachment_ref.attachment = 0;
    //             attachment_ref.layout = VK_IMAGE_LAYOUT_GENERAL;

    //             VkSubpassDescription subpass;
    //             subpass.flags = 0;
    //             subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //             subpass.inputAttachmentCount = 0;
    //             subpass.pInputAttachments = 0;
    //             subpass.colorAttachmentCount = 1;
    //             subpass.pColorAttachments = &attachment_ref;
    //             subpass.pResolveAttachments = 0;
    //             subpass.pDepthStencilAttachment = 0;
    //             subpass.preserveAttachmentCount = 0;
    //             subpass.pPreserveAttachments = 0;

    //             VkSubpassDependency dependency;
    //             dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    //             dependency.dstSubpass = 0;
    //             dependency.srcStageMask =
    //             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //             dependency.dstStageMask =
    //             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //             dependency.srcAccessMask = 0;
    //             dependency.dstAccessMask =
    //             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //             dependency.dependencyFlags = 0;

    //             VkRenderPassCreateInfo info;
    //             info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    //             info.pNext = 0;
    //             info.flags = 0;
    //             info.attachmentCount = 1;
    //             info.pAttachments = &attachment;
    //             info.subpassCount = 1;
    //             info.pSubpasses = &subpass;
    //             info.dependencyCount = 1;
    //             info.pDependencies = &dependency;
    //             CHECK_VK_RESULT(procs->vkCreateRenderPass(d->device, &info,
    //             0, &rp));
    //         }
    //         sw->render_pass = rp;

    //         {
    //             VkFramebufferCreateInfo info;
    //             info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    //             info.pNext = 0;
    //             info.flags = 0;
    //             info.renderPass = rp;
    //             info.attachmentCount = 1;
    //             info.width = descriptor->width;
    //             info.height = descriptor->height;
    //             info.layers = 1;
    //             for (usize i = 0; i < image_count; i++) {
    //                 info.pAttachments = &sw->image_views[i];
    //                 CHECK_VK_RESULT(
    //                     procs->vkCreateFramebuffer(d->device, &info, 0,
    //                     &sw->framebuffers[i]));
    //             }
    //         }

    //         sw->width = descriptor->width;
    //         sw->height = descriptor->height;

    //         {
    //             VkFenceCreateInfo info;
    //             info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //             info.pNext = 0;
    //             info.flags = 0;
    //             CHECK_VK_RESULT(procs->vkCreateFence(d->device, &info, 0,
    //             &sw->submit_done));
    //         }
    //         {
    //             VkFenceCreateInfo info;
    //             info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //             info.pNext = 0;
    //             info.flags = 0;
    //             CHECK_VK_RESULT(procs->vkCreateFence(d->device, &info, 0,
    //             &sw->image_available));
    //         }
    //         {
    //             VkSemaphoreCreateInfo info;
    //             info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    //             info.pNext = 0;
    //             info.flags = 0;
    //             CHECK_VK_RESULT(procs->vkCreateSemaphore(d->device, &info, 0,
    //             &sw->render_complete));
    //         }

    //         *swapchain = (swapchain_id)sw;
    //         return RESULT_SUCCESS;
    //     }

    //     void vulkan_destroy_swapchain(device_id device, swapchain_id
    //     swapchain) {
    //         cast_object(vulkan_device, d, device);
    //         cast_object(vulkan_swapchain, s, swapchain);

    //         struct vulkan_proc_table* procs = &d->procs;

    //         for (auto f : s->framebuffers) {
    //             procs->vkDestroyFramebuffer(d->device, f, 0);
    //         }
    //         for (auto iv : s->image_views) {
    //             procs->vkDestroyImageView(d->device, iv, 0);
    //         }
    //         procs->vkDestroyRenderPass(d->device, s->render_pass, 0);
    //         procs->vkDestroySwapchainKHR(d->device, s->swapchain, 0);
    //         dealloc_object(vulkan_swapchain, s);
    //     }

    //     int vulkan_acquire_image(device_id device, struct acquire_descriptor
    //     const* descriptor,
    //                              presentable_id* presentable) {
    //         cast_object(vulkan_device, d, device);
    //         cast_object(vulkan_swapchain, s, descriptor->swapchain);

    //         /**
    //          * Spec:
    //          *
    //          * After acquiring a presentable image and before modifying it,
    //          the application must use a
    //          * synchronization primitive to ensure that the presentation
    //          engine has finished reading
    //          * from the image.
    //          *
    //          * The presentation engine may not have finished reading from the
    //          image at the time it is
    //          * acquired, so the application must use semaphore and/or fence
    //          to ensure that the image
    //          * layout and contents are not modified until the presentation
    //          engine reads have completed.
    //          */
    //         int result = d->procs.vkAcquireNextImageKHR(d->device,
    //         s->swapchain, UINT64_MAX, 0,
    //                                                     s->image_available,
    //                                                     &s->presentable.index);
    //         if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    //             spargel_log_fatal("cannot acquire next swapchain image");
    //             spargel_panic_here();
    //         }

    //         CHECK_VK_RESULT(
    //             d->procs.vkWaitForFences(d->device, 1, &s->image_available,
    //             VK_TRUE, UINT64_MAX));
    //         CHECK_VK_RESULT(d->procs.vkResetFences(d->device, 1,
    //         &s->image_available));

    //         s->presentable.swapchain = s;

    //         *presentable = (presentable_id)&s->presentable;
    //         return RESULT_SUCCESS;
    //     }

    //     void vulkan_begin_render_pass(device_id device, struct
    //     render_pass_descriptor const* descriptor,
    //                                   render_pass_encoder_id* encoder) {
    //         cast_object(vulkan_device, d, device);
    //         cast_object(vulkan_command_buffer, cmdbuf,
    //         descriptor->command_buffer); cast_object(vulkan_swapchain, sw,
    //         descriptor->swapchain); alloc_object(vulkan_render_pass_encoder,
    //         e);

    //     {
    //         VkCommandBufferBeginInfo info;
    //         info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //         info.pNext = 0;
    //         info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //         info.pInheritanceInfo = 0;
    //         CHECK_VK_RESULT(d->procs.vkBeginCommandBuffer(cmdbuf->command_buffer,
    //         &info));
    //     }
    //     {
    //         VkClearValue clear;
    //         /* bug: double to float */
    //         clear.color.float32[0] = descriptor->clear_color.r;
    //         clear.color.float32[1] = descriptor->clear_color.g;
    //         clear.color.float32[2] = descriptor->clear_color.b;
    //         clear.color.float32[3] = descriptor->clear_color.a;

    //         VkRenderPassBeginInfo info;
    //         info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    //         info.pNext = 0;
    //         /* !!! */
    //         info.renderPass = sw->render_pass;
    //         /* !!! */
    //         info.framebuffer = sw->framebuffers[sw->presentable.index];
    //         info.renderArea.offset.x = 0;
    //         info.renderArea.offset.y = 0;
    //         info.renderArea.extent.width = sw->width;
    //         info.renderArea.extent.height = sw->height;
    //         info.clearValueCount = 1;
    //         info.pClearValues = &clear;

    //         d->procs.vkCmdBeginRenderPass(cmdbuf->command_buffer, &info,
    //                                       VK_SUBPASS_CONTENTS_INLINE);
    //     }

    //     e->command_buffer = cmdbuf->command_buffer;

    //     *encoder = (render_pass_encoder_id)e;
    // }

    // void vulkan_end_render_pass(device_id device, render_pass_encoder_id
    // encoder) {
    //     cast_object(vulkan_device, d, device);
    //     cast_object(vulkan_render_pass_encoder, e, encoder);
    //     d->procs.vkCmdEndRenderPass(e->command_buffer);
    //     CHECK_VK_RESULT(d->procs.vkEndCommandBuffer(e->command_buffer));
    //     dealloc_object(vulkan_render_pass_encoder, e);
    // }

    // void vulkan_present(device_id device, struct present_descriptor const*
    // descriptor) {
    //     cast_object(vulkan_device, d, device);
    //     cast_object(vulkan_command_buffer, c, descriptor->command_buffer);
    //     cast_object(vulkan_presentable, p, descriptor->presentable);
    //     {
    //         VkSubmitInfo info;
    //         info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //         info.pNext = 0;
    //         info.waitSemaphoreCount = 0;
    //         info.pWaitSemaphores = 0;
    //         VkPipelineStageFlags stage =
    //         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //         info.pWaitDstStageMask = &stage;
    //         info.commandBufferCount = 1;
    //         info.pCommandBuffers = &c->command_buffer;
    //         info.signalSemaphoreCount = 1;
    //         info.pSignalSemaphores = &p->swapchain->render_complete;
    //         CHECK_VK_RESULT(d->procs.vkQueueSubmit(c->queue, 1, &info,
    //         p->swapchain->submit_done));
    //     }
    //     /**
    //      * Spec:
    //      *
    //      * Calls to vkQueuePresentKHR may block, but must return in finite
    //      time. The processing of
    //      * the presentation happens in issue order with other queue
    //      operations, but semaphores must
    //      * be used to ensure that prior rendering and other commands in the
    //      specified queue complete
    //      * before the presentation begins. The presentation command itself
    //      does not delay processing
    //      * of subsequent commands on the queue. However, presentation
    //      requests sent to a particular
    //      * queue are always performed in order. Exact presentation timing is
    //      controlled by the
    //      * semantics of the presentation engine and native platform in use.
    //      */
    //     {
    //         VkPresentInfoKHR info = {};
    //         info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //         info.pNext = 0;
    //         info.waitSemaphoreCount = 1;
    //         info.pWaitSemaphores = &p->swapchain->render_complete;
    //         info.swapchainCount = 1;
    //         info.pSwapchains = &p->swapchain->swapchain;
    //         info.pImageIndices = &p->index;
    //         info.pResults = 0;
    //         int result = d->procs.vkQueuePresentKHR(c->queue, &info);
    //         if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    //             spargel_log_fatal("cannot acquire next swapchain image");
    //             spargel_panic_here();
    //         }
    //     }
    //     /* todo */
    //     CHECK_VK_RESULT(d->procs.vkWaitForFences(d->device, 1,
    //     &p->swapchain->submit_done, VK_TRUE,
    //                                              UINT64_MAX));
    //     CHECK_VK_RESULT(d->procs.vkResetFences(d->device, 1,
    //     &p->swapchain->submit_done));
    // }

    // void vulkan_presentable_texture(device_id device, presentable_id
    // presentable,
    //                                 texture_id* texture) {
    //     *texture = 0;
    // }

}  // namespace spargel::gpu
