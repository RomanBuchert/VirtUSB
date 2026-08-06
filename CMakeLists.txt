cmake_minimum_required(VERSION 3.31)

project(
   VirtUSB
   VERSION 0.0.1
   DESCRIPTION "Virtual USB Host Controller for Linux"
   LANGUAGES C
)

include(CTest)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
   set(
      VIRTUSB_COMPILE_COMMANDS_LINK
      "${CMAKE_SOURCE_DIR}/build/compile_commands.json"
   )

   file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

   file(
      CREATE_LINK
      "${CMAKE_BINARY_DIR}/compile_commands.json"
      "${VIRTUSB_COMPILE_COMMANDS_LINK}"
      SYMBOLIC
      RESULT VIRTUSB_COMPILE_COMMANDS_LINK_RESULT
   )

   if(NOT VIRTUSB_COMPILE_COMMANDS_LINK_RESULT STREQUAL "0")
      message(
         WARNING
         "Failed to create compile_commands.json link: "
         "${VIRTUSB_COMPILE_COMMANDS_LINK_RESULT}"
      )
   endif()
endif()

add_subdirectory(kernel)
