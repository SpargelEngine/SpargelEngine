#pragma once

#include "spargel/base/allocator.h"
#include "spargel/base/vector.h"

namespace spargel::base {

    class Test {
    public:
        virtual ~Test() = default;
        virtual void run() = 0;
    };

    class TestManager {
    public:
        static TestManager* getInstance();

        template <typename T>
        T* registerTest(char const* name) {
            T* ptr = default_allocator()->allocObject<T>();
            _tests.emplace(name, ptr);
            return ptr;
        }

        void runAll();

    private:
        struct TestEntry {
            char const* name;
            Test* test;
        };

        Vector<TestEntry> _tests;
    };

}  // namespace spargel::base

#define _TEST_CLASS_NAME(test_name) _SpargelTestClass_##test_name

#define TEST(test_name)                                                      \
    class _TEST_CLASS_NAME(test_name) final : public ::spargel::base::Test { \
    public:                                                                  \
        void run() override;                                                 \
                                                                             \
    private:                                                                 \
        static _TEST_CLASS_NAME(test_name) * _instance;                      \
    };                                                                       \
    _TEST_CLASS_NAME(test_name) * _TEST_CLASS_NAME(test_name)::_instance =   \
        ::spargel::base::TestManager::getInstance()                          \
            ->registerTest<_TEST_CLASS_NAME(test_name)>(#test_name);         \
    void _TEST_CLASS_NAME(test_name)::run()
