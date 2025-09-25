#include "spargel/task/task.h"

namespace spargel::task {
    Task Task::PromiseType::get_return_object() {
        return Task{HandleType::from_promise(*this)};
    }
}  // namespace spargel::task
