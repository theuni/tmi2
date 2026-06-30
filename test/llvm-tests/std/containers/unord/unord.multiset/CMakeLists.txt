set (unordered_multiset_sources
    count.pass.cpp
    emplace.pass.cpp
    find_const.pass.cpp
    reserve.pass.cpp
    emplace_hint.pass.cpp
    count.transparent.pass.cpp
    # insert_range.pass.cpp Requires c++23
    merge.pass.cpp
    max_bucket_count.pass.cpp
    get_allocator.pass.cpp
    insert_hint_rvalue.pass.cpp
    incomplete.pass.cpp
    insert_hint_const_lvalue.pass.cpp
    bucket.pass.cpp
    erase_range.pass.cpp
    equal_range_const.pass.cpp
    find.transparent.pass.cpp
    erase_if.pass.cpp
    erase_key.pass.cpp
    insert_node_type_hint.pass.cpp
    eq.different_hash.pass.cpp
    eq.pass.cpp
    insert_init.pass.cpp
    insert_iter_iter.pass.cpp
    insert_rvalue.pass.cpp
    insert_node_type.pass.cpp
    max_size.pass.cpp
    types.pass.cpp
    hash_function.pass.cpp
    swap_member.pass.cpp
    bucket_size.pass.cpp
    local_iterators.pass.cpp
    rehash.pass.cpp
    insert_const_lvalue.pass.cpp
    iterators.pass.cpp
    clear.pass.cpp
    extract_iterator.pass.cpp
    bucket_count.pass.cpp
    max_load_factor.pass.cpp
    load_factor.pass.cpp
    key_eq.pass.cpp
    size.pass.cpp
    contains.transparent.pass.cpp
    empty.pass.cpp
    equal_range.transparent.pass.cpp
    insert_emplace_allocator_requirements.pass.cpp
    find_non_const.pass.cpp
    erase_const_iter.pass.cpp
    equal_range_non_const.pass.cpp
    extract_key.pass.cpp
)

set (unordered_multiset_compile_sources
    empty.verify.cpp
    iterator_concept_conformance.compile.pass.cpp
    range_concept_conformance.compile.pass.cpp
)


set (unordered_multiset_compile_fail_sources
    allocator_mismatch.verify.cpp
    iterators.compile.fail.cpp
    local_iterators.compile.fail.cpp
)

set_source_files_properties(insert_range.pass.cpp
    PROPERTIES
    COMPILE_OPTIONS "-Wno-missing-field-initializers"
)

foreach(source_file IN LISTS unordered_multiset_compile_fail_sources)
    try_compile(result_var SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}" NO_CACHE
        OUTPUT_VARIABLE compile_output
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/test/llvm-tests/support"
    )
    if (result_var)
        message(SEND_ERROR "${source_file} should not compile. Output: ${compile_output}")
    endif()
endforeach()


add_library(unordered_multiset_compile OBJECT ${unordered_multiset_compile_sources})
target_link_libraries(unordered_multiset_compile PRIVATE llvm_test_interface)

foreach(source_file IN LISTS unordered_multiset_sources)
    cmake_path(REMOVE_EXTENSION source_file LAST_ONLY OUTPUT_VARIABLE target_name)
    string(PREPEND target_name "unord.multiset.")
    add_executable("${target_name}" "${source_file}")
    target_link_libraries(${target_name} PRIVATE llvm_test_interface)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

add_subdirectory(unord.multiset.cnstr)
add_subdirectory(unord.multiset.swap)
