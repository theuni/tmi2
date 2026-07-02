set (multiset_cons_sources
    alloc.pass.cpp
    assign_initializer_list.pass.cpp
    compare_alloc.pass.cpp
    compare.pass.cpp
    copy_alloc.pass.cpp
    copy_assign.pass.cpp
    copy.pass.cpp
    #deduct.pass.cpp Deduction
    default_noexcept.pass.cpp
    default.pass.cpp
    dtor_noexcept.pass.cpp
    #from_range.pass.cpp Requires c++23
    initializer_list_compare_alloc.pass.cpp
    initializer_list_compare.pass.cpp
    initializer_list.pass.cpp
    iter_iter_alloc.pass.cpp
    iter_iter_comp.pass.cpp
    iter_iter.pass.cpp
    move_alloc.pass.cpp
    move_assign.pass.cpp
    move_noexcept.pass.cpp
    move.pass.cpp
)

set (multiset_cons_compile_sources
    copy_assign.addressof.compile.pass.cpp
    # deduct.verify.cpp Deduction
    move_assign_noexcept.compile.pass.cpp
)

set (multiset_cons_compile_fail_sources
    compare_copy_constructible.compile.fail.cpp
)

foreach(source_file IN LISTS multiset_cons_compile_fail_sources)
    try_compile(result_var SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}" NO_CACHE
        OUTPUT_VARIABLE compile_output
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/test/llvm-tests/stdlib-overrides"
    )
    if (result_var)
        message(WARNING "${source_file} should not compile. Output: ${compile_output}")
    endif()
endforeach()

add_library(multiset_cons_compile OBJECT ${multiset_cons_compile_sources})
target_link_libraries(multiset_cons_compile PRIVATE llvm_test_interface)

foreach(source_file IN LISTS multiset_cons_sources)
    cmake_path(REMOVE_EXTENSION source_file LAST_ONLY OUTPUT_VARIABLE target_name)
    string(PREPEND target_name "multiset.cons.")
    add_executable("${target_name}" "${source_file}")
    target_link_libraries(${target_name} PRIVATE llvm_test_interface)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

