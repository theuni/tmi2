set (container_compile_sources
    iterator.rel_ops.compile.pass.cpp
)

set_source_files_properties(iterator.rel_ops.compile.pass.cpp
    PROPERTIES
    COMPILE_OPTIONS "-Wno-deprecated-declarations"
)

add_library(container_compile OBJECT ${container_compile_sources})
target_link_libraries(container_compile PRIVATE llvm_test_interface)

add_subdirectory(unord)
add_subdirectory(associative)
