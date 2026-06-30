set (container_compile_sources
    iterator.rel_ops.compile.pass.cpp
)

set_source_files_properties(iterator.rel_ops.compile.pass.cpp
    PROPERTIES
    COMPILE_DEFINITIONS "-D_LIBCPP_DISABLE_DEPRECATION_WARNINGS"
)

add_library(container_compile OBJECT ${container_compile_sources})
target_link_libraries(container_compile PRIVATE llvm_test_interface)

add_subdirectory(unord)
