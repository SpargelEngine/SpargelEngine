#pragma once

namespace spargel::gpu {

class Device;
class Buffer;
class Texture;
class ComputePipeline;
class ComputeEncoder;
class CommandQueue;
class CommandBuffer;

class Device {
public:
  static Device* create();

  virtual ~Device() = default;

  virtual CommandQueue* createCommandQueue() = 0;
  virtual CommandBuffer* createCommandBuffer() = 0;

  virtual Buffer* createBuffer() = 0;
  virtual Texture* createTexture() = 0;

  virtual ComputePipeline* createComputePipeline() = 0;
  virtual ComputeEncoder* createComputeEncoder() = 0;

  // destroy objects -----------------------------------------------------------

  void destroy(CommandQueue* queue);
  void destroy(CommandBuffer* cmdbuf);
  void destroy(Buffer* buffer);
  void destroy(Texture* texture);
  void destroy(ComputePipeline* pipeline);
  void destroy(ComputeEncoder* encoder);
};

class CommandQueue {
public:
  virtual ~CommandQueue() = default;

  // TODO(tianjiao):
  // - Metal4 supports committing multiple command buffers.
  virtual void submit(CommandBuffer* cmdbuf) = 0;
};

// A command buffer must be submitted to the queue that it is created with.
// TODO(tianjiao):
// - Metal4 supports submitting to different queues.
// - Metal3 doesn't support reusing command buffers.
class CommandBuffer {
public:
  virtual ~CommandBuffer() = default;
};

class Buffer {
public:
  virtual ~Buffer() = default;
};

class Texture {
public:
  virtual ~Texture() = default;
};

class ComputePipeline {
public:
  virtual ~ComputePipeline() = default;
};

class ComputeEncoder {
public:
  virtual ~ComputeEncoder() = default;
};

}  // namespace spargel::gpu
