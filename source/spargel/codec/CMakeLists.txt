spargel_add_library(
    NAME codec
    PRIVATE
        codec.cpp
        json_codec.cpp
    DEPS
        base
        json
)

# TEST

spargel_add_executable(
    NAME test_codec
    PRIVATE
        test_codec.cpp
        test_json_codec.cpp
    DEPS
        codec
        test_main
)
add_test(
    NAME test_codec
    COMMAND test_codec
)
