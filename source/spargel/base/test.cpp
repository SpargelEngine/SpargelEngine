#include <spargel/base/test.h>

// libc
#include <stdio.h>

namespace spargel::base {

    TestManager* TestManager::getInstance() {
        static TestManager manager;
        return &manager;
    }

    void TestManager::runAll() {
        for (usize i = 0; i < _tests.count(); i++) {
            auto entry = _tests[i];
            printf("[%zu/%zu] %s\n", i + 1, _tests.count(), entry.name);
            entry.test->run();
        }
    }

}  // namespace spargel::base
