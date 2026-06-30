set (associative_sources
    iterator_types.pass.cpp
    lookup_with_converting_comparator.pass.cpp
)

foreach(source_file IN LISTS associative_sources)
    cmake_path(REMOVE_EXTENSION source_file LAST_ONLY OUTPUT_VARIABLE target_name)
    string(PREPEND target_name "associative.")
    add_executable("${target_name}" "${source_file}")
    target_link_libraries(${target_name} PRIVATE llvm_test_interface)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

add_subdirectory(set)
add_subdirectory(multiset)
