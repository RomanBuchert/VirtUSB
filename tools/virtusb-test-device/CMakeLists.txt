add_executable(
   virtusb-test-device
   main.c
)

target_compile_features(
   virtusb-test-device
   PRIVATE
      c_std_11
)

target_link_libraries(
   virtusb-test-device
   PRIVATE
      virtusb
)
