add_executable(
   virtusb-test-backend-unregister
   main.c
)

target_compile_features(
   virtusb-test-backend-unregister
   PRIVATE
      c_std_11
)

target_link_libraries(
   virtusb-test-backend-unregister
   PRIVATE
      virtusb
)
