find_package(LLVM CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs Support Core IRReader)

add_library(cap_lib STATIC
  src/lib.c
)

target_include_directories(cap_lib
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(cap_lib
	PUBLIC
        ${LLVM_DEFINITIONS}
)

# Link with LLVM C API libraries
target_link_libraries(cap_lib
	PUBLIC
	    ${llvm_libs}
		LLVM-C
)
