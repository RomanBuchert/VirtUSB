include(
   "${PROJECT_SOURCE_DIR}/cmake/kernel_module.cmake"
)

add_linux_kernel_module(
   TARGET
      virtusb_module

   MODULE_NAME
      virtusb

   SOURCE_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}"

   OUTPUT_DIR
      "${CMAKE_CURRENT_BINARY_DIR}"

   SOURCES
      src/virtusb_module.c
      src/virtusb_control.c
      src/virtusb_object.c
      src/virtusb_object_manager.c
      src/virtusb_port.c
      src/virtusb_device.c
      src/virtusb_hub.c
      src/virtusb_root_hub.c
      src/virtusb_hcd.c

   INCLUDE_DIRECTORIES
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
      "${PROJECT_SOURCE_DIR}/uapi/include"
)
