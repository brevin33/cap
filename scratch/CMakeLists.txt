project(scratch C)

set(CMAKE_C_STANDARD 11)

add_executable(cap_scratch main.c)

target_link_libraries(cap_scratch 
	cap_lang
)
