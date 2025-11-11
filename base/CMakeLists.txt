project(cap_lang C)

set(CMAKE_C_STANDARD 11)

add_library(base
	src/arena.c
	src/basic.c
	src/base.c
)

target_include_directories(base
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
