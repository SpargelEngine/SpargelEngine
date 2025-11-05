#include "spargel/init.h"

#include "spargel/gpu/context.h"
#include "spargel/ui/context.h"

namespace spargel {

void init_runtime() {
  gpu::Context::get().init();
  ui::Context::get().init();
}

}  // namespace spargel
