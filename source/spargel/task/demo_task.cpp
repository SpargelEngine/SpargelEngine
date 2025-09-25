#include "spargel/logging/logging.h"
#include "spargel/task/task_manager.h"

namespace spargel::task {
    namespace {
        void demoMain() {
            // auto tm = TaskManager::create();
            // tm->postTask([] { spargel_log_info("hello task!"); });
        }
    }  // namespace
}  // namespace spargel::task

int main() {
    spargel::task::demoMain();
    return 0;
}
