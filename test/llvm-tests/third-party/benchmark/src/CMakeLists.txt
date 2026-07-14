set (benchmark_sources
    benchmark_api_internal.cc
    benchmark.cc
    benchmark_main.cc
    benchmark_name.cc
    benchmark_register.cc
    benchmark_runner.cc
    check.cc
    colorprint.cc
    commandlineflags.cc
    complexity.cc
    console_reporter.cc
    counter.cc
    csv_reporter.cc
    json_reporter.cc
    perf_counters.cc
    reporter.cc
    statistics.cc
    string_util.cc
    sysinfo.cc
    timers.cc
)

add_library(benchmark_lib STATIC ${benchmark_sources})
target_include_directories(benchmark_lib SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/test/llvm-tests/third-party/benchmark/include)
target_compile_definitions(benchmark_lib PRIVATE BENCHMARK_VERSION="v0.0.0")
target_link_libraries(llvm_bench_interface INTERFACE benchmark_lib)
