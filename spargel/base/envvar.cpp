#include "spargel/base/envvar.h"

#include <stdlib.h>

namespace spargel::base {

std::string get_env_var(char const* name) { return ::getenv(name); }

}  // namespace spargel::base
