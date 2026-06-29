cmake_minimum_required(VERSION 3.22)

project ("tmi" LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/module)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

include(TryAppendCXXFlags)

add_library(warnings_interface INTERFACE)
add_library(common_interface INTERFACE)
try_append_cxx_flags("-Wall" TARGET warnings_interface SKIP_LINK)
try_append_cxx_flags("-Wextra" TARGET warnings_interface SKIP_LINK)
try_append_cxx_flags("-pedantic" TARGET warnings_interface SKIP_LINK)
try_append_cxx_flags("-Weverything" TARGET warnings_interface SKIP_LINK)

try_append_cxx_flags("-Wc++98-compat" TARGET warnings_interface SKIP_LINK
  IF_CHECK_PASSED "-Wno-c++98-compat"
)
try_append_cxx_flags("-Wc++20-compat" TARGET warnings_interface SKIP_LINK
  IF_CHECK_PASSED "-Wno-c++20-compat"
)
try_append_cxx_flags("-Wc++98-compat-pedantic" TARGET warnings_interface SKIP_LINK
  IF_CHECK_PASSED "-Wno-c++98-compat-pedantic"
)
try_append_cxx_flags("-Wpadded" TARGET warnings_interface SKIP_LINK
  IF_CHECK_PASSED "-Wno-padded"
)
try_append_cxx_flags("-Wunsafe-buffer-usage" TARGET warnings_interface SKIP_LINK
  IF_CHECK_PASSED "-Wno-unsafe-buffer-usage"
)
try_append_cxx_flags("-Wlifetime-safety-intra-tu-suggestions" TARGET warnings_interface SKIP_LINK
  IF_CHECK_PASSED "-Wno-lifetime-safety-intra-tu-suggestions"
)

target_include_directories(common_interface INTERFACE ${CMAKE_SOURCE_DIR})

add_executable(tmi-test test/test.cpp
    test/hashed_unique.cpp
    test/hashed_non_unique.cpp
    test/ordered_unique.cpp
    test/ordered_non_unique.cpp
    test/stdset.cpp
    test/std_unordered_set.cpp
)
target_compile_definitions(tmi-test PRIVATE DEBUG)
target_link_libraries(tmi-test PRIVATE warnings_interface)
target_link_libraries(tmi-test PRIVATE common_interface)

enable_testing()
foreach(suite hashed_unique hashed_non_unique ordered_unique ordered_non_unique)
    add_test(NAME ${suite} COMMAND tmi-test ${suite})
endforeach()

option(BUILD_FUZZ "Build libFuzzer fuzz targets (requires Clang)" OFF)

if(BUILD_FUZZ)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "BUILD_FUZZ requires Clang (got ${CMAKE_CXX_COMPILER_ID})")
    endif()
    add_executable(fuzz_multi_index fuzz/fuzz_multi_index.cpp)
    target_link_libraries(fuzz_multi_index PRIVATE warnings_interface)
    target_link_libraries(fuzz_multi_index PRIVATE common_interface)
    target_compile_definitions(fuzz_multi_index PRIVATE DEBUG)
    target_compile_options(fuzz_multi_index PRIVATE
        -g -O1 -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
    target_link_options(fuzz_multi_index PRIVATE
        -fsanitize=fuzzer,address,undefined)
endif()


add_subdirectory(test/llvm-tests)
