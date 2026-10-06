add_executable(
   virtusb-test-fetch-rollback
   main.c
)

target_compile_features(
   virtusb-test-fetch-rollback
   PRIVATE
      c_std_11
)

target_link_libraries(
   virtusb-test-fetch-rollback
   PRIVATE
      virtusb_uapi
)
