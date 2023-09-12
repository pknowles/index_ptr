cmake_minimum_required(VERSION 3.15)

project(index_ptr)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fconcepts-diagnostics-depth=32)
endif()

set(HEADERS
    include/nodecode/index_ptr.hpp
)

add_library(index_ptr INTERFACE ${HEADERS})
target_include_directories(index_ptr INTERFACE include)

if(BUILD_TESTING)
  option(BUILD_INDEX_PTR_TESTING "Enable index_ptr testing" ON)
  if(BUILD_INDEX_PTR_TESTING)
    enable_testing()
    add_subdirectory(test)
  endif()
endif()
