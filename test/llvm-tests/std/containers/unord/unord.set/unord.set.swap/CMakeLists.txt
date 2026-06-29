set (unordered_set_swap_sources
    swap_non_member.pass.cpp
    swap_noexcept.pass.cpp
)

foreach(source_file IN LISTS unordered_set_swap_sources)
    cmake_path(REMOVE_EXTENSION source_file LAST_ONLY OUTPUT_VARIABLE target_name)
    string(PREPEND target_name "set.swap.")
    add_executable("${target_name}" "${source_file}")
    target_link_libraries(${target_name} PRIVATE llvm_test_interface)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

