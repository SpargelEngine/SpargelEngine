#include <spargel/config.h>
#include <spargel/gpu/gpu.h>

#if SPARGEL_GPU_ENABLE_METAL
namespace spargel::gpu {
    base::unique_ptr<Device> make_device_metal();
}
#endif

#if SPARGEL_GPU_ENABLE_VULKAN
namespace spargel::gpu {
    base::unique_ptr<Device> make_device_vulkan();
}
#endif

namespace spargel::gpu {

    base::unique_ptr<Device> makeDevice(DeviceKind kind) {
        switch (kind) {
#if SPARGEL_GPU_ENABLE_METAL
        case DeviceKind::metal:
            return make_device_metal();
#endif
#if SPARGEL_GPU_ENABLE_VULKAN
        case DeviceKind::vulkan:
            return make_device_vulkan();
#endif
        default:
            return nullptr;
        }
    }

}  // namespace spargel::gpu
