cmake_minimum_required(VERSION 3.15)
project(Tia)

# set this because i hate that windows has multiple runtime libraries
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

if(MSVC)
  add_compile_options(/W4 /WX /wd4100 /wd4101)
else()
  add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable)
endif()

add_subdirectory(base)
add_subdirectory(cap_lang)
add_subdirectory(scratch)
