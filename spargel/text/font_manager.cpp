#include "spargel/text/font_manager.h"

#include "spargel/base/logging.h"

namespace spargel::text {

FontManager::FontManager(char const* backend) {
  LOG_INFO("init font manager (backend: %s)", backend);
}

}  // namespace spargel::text
