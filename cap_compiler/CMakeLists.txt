add_executable(cap
  compiler.c
)

target_link_libraries(cap
  PRIVATE
    cap_lib
)

