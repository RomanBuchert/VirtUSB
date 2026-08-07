add_library(
   virtusb_uapi
   INTERFACE
)

target_include_directories(
   virtusb_uapi
   INTERFACE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
