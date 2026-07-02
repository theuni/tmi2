set (set_sources
    clear.pass.cpp
    contains.pass.cpp
    contains_transparent.pass.cpp
    count.pass.cpp
    count_transparent.pass.cpp
    emplace_hint.pass.cpp
    emplace.pass.cpp
    empty.pass.cpp
    equal_range.pass.cpp
    equal_range_transparent.pass.cpp
    erase_iter_iter.pass.cpp
    erase_iter.pass.cpp
    erase_key.pass.cpp
    extract_iterator.pass.cpp
    extract_key.pass.cpp
    find.pass.cpp
    get_allocator.pass.cpp
    incomplete_type.pass.cpp
    insert_and_emplace_allocator_requirements.pass.cpp
    insert_cv.pass.cpp
    insert_initializer_list.pass.cpp
    insert_iter_cv.pass.cpp
    insert_iter_iter.pass.cpp
    insert_iter_rv.pass.cpp
    insert_node_type_hint.pass.cpp
    insert_node_type.pass.cpp
    # insert_range.pass.cpp Requires c++23
    insert_rv.pass.cpp
    iterator.pass.cpp
    lower_bound.pass.cpp
    max_size.pass.cpp
    merge.pass.cpp
    size.pass.cpp
    types.pass.cpp
    upper_bound.pass.cpp
)

set (set_compile_sources
    gcc_workaround.compile.pass.cpp
    iterator_concept_conformance.compile.pass.cpp
    range_concept_conformance.compile.pass.cpp
    empty.verify.cpp
)

set (set_compile_fail_sources
    allocator_mismatch.verify.cpp
)

set_source_files_properties(insert_range.pass.cpp
    PROPERTIES
    COMPILE_OPTIONS "-Wno-missing-field-initializers"
)

foreach(source_file IN LISTS set_compile_fail_sources)
    try_compile(result_var SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}" NO_CACHE
        OUTPUT_VARIABLE compile_output
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/test/llvm-tests/stdlib-overrides"
    )
    if (result_var)
        message(SEND_ERROR "${source_file} should not compile. Output: ${compile_output}")
    endif()
endforeach()

add_library(set_compile OBJECT ${set_compile_sources})
target_link_libraries(set_compile PRIVATE llvm_test_interface)

foreach(source_file IN LISTS set_sources)
    cmake_path(REMOVE_EXTENSION source_file LAST_ONLY OUTPUT_VARIABLE target_name)
    string(PREPEND target_name "set.")
    add_executable("${target_name}" "${source_file}")
    target_link_libraries(${target_name} PRIVATE llvm_test_interface)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

add_subdirectory(set.cons)
add_subdirectory(set.erasure)
add_subdirectory(set.nonmember)
add_subdirectory(set.observers)
add_subdirectory(set.special)
