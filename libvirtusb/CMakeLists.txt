add_library(
   virtusb
   src/virtusb.c
)

target_compile_features(
   virtusb
   PUBLIC
      c_std_11
)

target_include_directories(
   virtusb
   PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(
   virtusb
   PRIVATE
      virtusb_uapi
)
