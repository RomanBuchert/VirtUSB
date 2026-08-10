add_executable(
   virtusbctl
   main.c
)

target_compile_features(
   virtusbctl
   PRIVATE
      c_std_11
)

target_link_libraries(
   virtusbctl
   PRIVATE
      virtusb
)
