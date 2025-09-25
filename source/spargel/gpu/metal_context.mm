#include "spargel/gpu/metal_context.h"
#include "spargel/logging/logging.h"

namespace spargel::gpu {
    base::UniquePtr<GPUContext> makeMetalContext() {
        return base::makeUnique<MetalContext>();
    }
    void MetalContext::init() {
        device_ = MTLCreateSystemDefaultDevice();
        queue_ = [device_ newCommandQueue];
    }
    void MetalContext::cleanup() {
        [queue_ release];
        [device_ release];
    }
    id<MTLBuffer> MetalContext::createBuffer(usize length) {
        buffer_count_++;
        spargel_log_info("create; current count: %zu", buffer_count_);
        return [device_ newBufferWithLength:length
                                    options:MTLResourceStorageModeShared];
    }
    void MetalContext::destroyBuffer(id<MTLBuffer> buffer) {
        buffer_count_--;
        spargel_log_info("destroy; current count: %zu", buffer_count_);
        [buffer release];
    }
}  // namespace spargel::gpu
