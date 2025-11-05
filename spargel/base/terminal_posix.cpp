#include "spargel/base/terminal.h"

#include <unistd.h>

#include "spargel/base/envvar.h"

namespace spargel::base {

bool supports_color(FILE* file) {
  if (!isatty(fileno(file))) {
    return false;
  }
  auto terminfo = get_env_var("TERMINFO");
  // TODO: parse TERMINFO
  return true;
}

}  // namespace spargel::base
