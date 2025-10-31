#pragma once

namespace spargel::gpu {

class Context {
public:
  static Context& get();

  virtual ~Context() = default;

  virtual void init() = 0;
};

}  // namespace spargel::gpu
