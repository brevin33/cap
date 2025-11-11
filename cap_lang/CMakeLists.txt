project(cap_lang C)

set(CMAKE_C_STANDARD 11)

find_package(LLVM CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs Support Core IRReader)

add_library(cap_lang
	src/cap.c
	src/tokens.c
	src/log.c
	src/project.c
	src/lists.c
	src/ast.c
	src/semantic.c
)


target_include_directories(cap_lang
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(cap_lang
	PUBLIC
        ${LLVM_DEFINITIONS}
)

# Link with LLVM C API libraries
target_link_libraries(cap_lang
	PUBLIC
	    ${llvm_libs}
		LLVM-C
		base
)
