set (unordered_set_cnstr_sources
    # deduct.verify.cpp Deduction
    init_size_hash_equal_allocator.pass.cpp
    size_hash.pass.cpp
    move.pass.cpp
    size_allocator.pass.cpp
    iter_iter.pass.cpp
    init_size_hash.pass.cpp
    copy_alloc.pass.cpp
    size_hash_equal.pass.cpp
    init_size_allocator.pass.cpp
    iter_iter_size_allocator.pass.cpp
    default_noexcept.pass.cpp
    size_hash_equal_allocator.pass.cpp
    copy.pass.cpp
    init_size_hash_allocator.pass.cpp
    default.pass.cpp
    move_noexcept.pass.cpp
    iter_iter_size_hash_equal_allocator.pass.cpp
    iter_iter_size_hash.pass.cpp
    assign_move.pass.cpp
    size_hash_allocator.pass.cpp
    iter_iter_size_hash_allocator.pass.cpp
    #from_range.pass.cpp Requires c++23
    move_alloc.pass.cpp
    init_size.pass.cpp
    iter_iter_size_hash_equal.pass.cpp
    dtor_noexcept.pass.cpp
    allocator.pass.cpp
    init.pass.cpp
    size.pass.cpp
    assign_copy.pass.cpp
    assign_init.pass.cpp
    #deduct.pass.cpp Deduction
)

set (unordered_set_cnstr_compile_sources
    move.addressof.compile.pass.cpp
    move_alloc.addressof.compile.pass.cpp
    assign_copy.addressof.compile.pass.cpp
    move_assign_noexcept.compile.pass.cpp
)

set (unordered_set_cnstr_compile_fail_sources
    hash_copy_constructible.compile.fail.cpp
    size.compile.fail.cpp
    compare_copy_constructible.compile.fail.cpp
)

foreach(source_file IN LISTS unordered_set_cnstr_compile_fail_sources)
    try_compile(result_var SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}" NO_CACHE
        OUTPUT_VARIABLE compile_output
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/test/llvm-tests/stdlib-overrides"
    )
    if (result_var)
        message(SEND_ERROR "${source_file} should not compile. Output: ${compile_output}")
    endif()
endforeach()

add_library(unordered_set_cnstr_compile OBJECT ${unordered_set_cnstr_compile_sources})
target_link_libraries(unordered_set_cnstr_compile PRIVATE llvm_test_interface)

foreach(source_file IN LISTS unordered_set_cnstr_sources)
    cmake_path(REMOVE_EXTENSION source_file LAST_ONLY OUTPUT_VARIABLE target_name)
    string(PREPEND target_name "unord.set.cnstr.")
    add_executable("${target_name}" "${source_file}")
    target_link_libraries(${target_name} PRIVATE llvm_test_interface)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

