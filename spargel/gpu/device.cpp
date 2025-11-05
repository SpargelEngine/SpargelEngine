#include "spargel/gpu/device.h"

namespace spargel::gpu {

void Device::destroy(CommandQueue* queue) { delete queue; }
void Device::destroy(CommandBuffer* cmdbuf) { delete cmdbuf; }
void Device::destroy(Buffer* buffer) { delete buffer; }
void Device::destroy(Texture* texture) { delete texture; }
void Device::destroy(ComputePipeline* pipeline) { delete pipeline; }
void Device::destroy(ComputeEncoder* encoder) { delete encoder; }

}  // namespace spargel::gpu
