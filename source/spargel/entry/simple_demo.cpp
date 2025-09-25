#include "spargel/base/platform.h"
#include "spargel/entry/simple.h"
#include "spargel/logging/logging.h"

namespace spargel::entry {

    int simple_entry(simple_entry_data* data) {
        spargel_log_info("running executable: %s",
                         base::get_executable_path().data());
        return 0;
    }

}  // namespace spargel::entry
