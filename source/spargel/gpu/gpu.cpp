#include "spargel/gpu/gpu.h"

#include "spargel/config.h"
#include "spargel/logging/logging.h"

#if SPARGEL_ENABLE_METAL
namespace spargel::gpu {
    base::unique_ptr<Device> makeMetalDevice();
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
#if SPARGEL_ENABLE_METAL
        case DeviceKind::metal:
            return makeMetalDevice();
#endif
#if SPARGEL_GPU_ENABLE_VULKAN
        case DeviceKind::vulkan:
            return make_device_vulkan();
#endif
        default:
            spargel_log_fatal("unknown gpu backend");
            spargel_panic_here();
            return nullptr;
        }
    }

    base::unique_ptr<Device> Device::create() {
#if SPARGEL_ENABLE_METAL
        return makeMetalDevice();
#else
        return nullptr;
#endif
    }

}  // namespace spargel::gpu
