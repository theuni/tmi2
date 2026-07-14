add_executable(multiset.bench multiset.bench.cpp)
add_executable(set.bench set.bench.cpp)
add_executable(unordered_multiset.bench unordered_multiset.bench.cpp)
add_executable(unordered_set.bench unordered_set.bench.cpp)

target_link_libraries(multiset.bench PRIVATE llvm_bench_interface)
target_link_libraries(set.bench PRIVATE llvm_bench_interface)
target_link_libraries(unordered_multiset.bench PRIVATE llvm_bench_interface)
target_link_libraries(unordered_set.bench PRIVATE llvm_bench_interface)
