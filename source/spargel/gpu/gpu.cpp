#include "spargel/gpu/gpu.h"

#include "spargel/gpu/build_config.h"

#if SPARGEL_GPU_ENABLE_METAL
namespace spargel::gpu {
    base::unique_ptr<Device> makeMetalDevice();
}
#endif

#if SPARGEL_GPU_ENABLE_VULKAN
namespace spargel::gpu {
    base::unique_ptr<Device> makeVulkanDevice();
}
#endif

namespace spargel::gpu {

    base::unique_ptr<Device> Device::create(DeviceKind kind) {
        switch (kind) {
        case DeviceKind::metal:
            return makeMetalDevice();
        case DeviceKind::vulkan:
            return makeVulkanDevice();
        default:
            return nullptr;
        }
    }

}  // namespace spargel::gpu
