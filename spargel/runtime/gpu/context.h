#pragma once

namespace spargel::runtime::gpu {
class Context {
public:
    static Context& get();

    virtual ~Context() = default;

    virtual void init() = 0;
};
}  // namespace spargel::runtime::gpu
