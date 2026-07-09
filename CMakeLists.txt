cmake_minimum_required(VERSION 3.15)
project(Cap)

if(MSVC)
  add_compile_options(/W4 /WX /wd4100 /wd4101)
else()
  add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable)
endif()

add_subdirectory(cap)
