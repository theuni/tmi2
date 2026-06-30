set (multiset_nonmember_sources
    compare.three_way.pass.cpp 
    op_compare.pass.cpp
)

set (multiset_nonmember_compile_fail_sources
    compare.three_way.verify.cpp
)

foreach(source_file IN LISTS multiset_nonmember_compile_fail_sources)
    try_compile(result_var SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}" NO_CACHE
        OUTPUT_VARIABLE compile_output
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/test/llvm-tests/support"
    )
    if (result_var)
        message(SEND_ERROR "${source_file} should not compile. Output: ${compile_output}")
    endif()
endforeach()

foreach(source_file IN LISTS multiset_nonmember_sources)
    cmake_path(REMOVE_EXTENSION source_file LAST_ONLY OUTPUT_VARIABLE target_name)
    string(PREPEND target_name "multiset.nonmember.")
    add_executable("${target_name}" "${source_file}")
    target_link_libraries(${target_name} PRIVATE llvm_test_interface)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

