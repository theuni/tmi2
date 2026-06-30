set (multiset_special_sources
    member_swap.pass.cpp
    non_member_swap.pass.cpp
    swap_noexcept.pass.cpp
)

foreach(source_file IN LISTS multiset_special_sources)
    cmake_path(REMOVE_EXTENSION source_file LAST_ONLY OUTPUT_VARIABLE target_name)
    string(PREPEND target_name "multiset.special.")
    add_executable("${target_name}" "${source_file}")
    target_link_libraries(${target_name} PRIVATE llvm_test_interface)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()

