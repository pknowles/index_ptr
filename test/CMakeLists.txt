cmake_minimum_required(VERSION 3.15)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.3.11
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nanobench)

add_executable(${PROJECT_NAME}_tests
    test_main.cpp
    test_benchmark.cpp
)

target_include_directories(${PROJECT_NAME}_tests PRIVATE .)

target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    index_ptr
    gtest_main
    nanobench
)

# TODO: presets? https://stackoverflow.com/questions/45955272/modern-way-to-set-compiler-flags-in-cross-platform-cmake-project
if(MSVC)
    target_compile_options(${PROJECT_NAME}_tests PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME}_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME}_tests PRIVATE -fbounds-check)
    endif()
endif()


# Enable testing with CTest
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)
