find_program(
   VIRTUSB_MAKE_EXECUTABLE
   NAMES make gmake
   REQUIRED
)

find_package(
   Python3
   REQUIRED
   COMPONENTS Interpreter
)

execute_process(
   COMMAND uname -r
   OUTPUT_VARIABLE VIRTUSB_RUNNING_KERNEL_RELEASE
   OUTPUT_STRIP_TRAILING_WHITESPACE
   COMMAND_ERROR_IS_FATAL ANY
)

set(
   VIRTUSB_KERNEL_BUILD_DIR
   "/lib/modules/${VIRTUSB_RUNNING_KERNEL_RELEASE}/build"
   CACHE PATH
   "Linux kernel build directory used to build the VirtUSB module"
)

if(NOT EXISTS "${VIRTUSB_KERNEL_BUILD_DIR}/Makefile")
   message(
      FATAL_ERROR
      "Linux kernel build directory is unavailable:\n"
      "  ${VIRTUSB_KERNEL_BUILD_DIR}\n"
      "Install the matching kernel headers or set "
      "VIRTUSB_KERNEL_BUILD_DIR explicitly."
   )
endif()

set(
   VIRTUSB_GEN_COMPILE_COMMANDS
   "${VIRTUSB_KERNEL_BUILD_DIR}/scripts/clang-tools/gen_compile_commands.py"
)

if(NOT EXISTS "${VIRTUSB_GEN_COMPILE_COMMANDS}")
   message(
      FATAL_ERROR
      "Kernel compile commands generator is unavailable:\n"
      "  ${VIRTUSB_GEN_COMPILE_COMMANDS}"
   )
endif()

set(
   VIRTUSB_KERNEL_SOURCE_DIR
   "${CMAKE_CURRENT_SOURCE_DIR}"
)

set(
   VIRTUSB_KERNEL_OUTPUT_DIR
   "${CMAKE_CURRENT_BINARY_DIR}"
)

#
# Source of truth for all source and header files belonging to the kernel
# module.
#

set(
   VIRTUSB_KERNEL_SOURCE_FILES
   src/virtusb_module.c
   src/virtusb_hcd.c
   src/virtusb_hub.c
   src/virtusb_root_hub.c
)

set(
   VIRTUSB_KERNEL_HEADER_FILES
   include/virtusb_hcd.h
   include/virtusb_hub.h
   include/virtusb_root_hub.h
)

#
# Create the generated Kbuild source view.
#
# Original sources and headers remain below kernel/. Kbuild operates
# exclusively in the current CMake binary directory so that all generated
# files and build artifacts stay below build/.
#

set(VIRTUSB_KBUILD_SOURCE_FILES)
set(VIRTUSB_KERNEL_OBJECT_FILES)

foreach(VIRTUSB_KERNEL_SOURCE_FILE IN LISTS VIRTUSB_KERNEL_SOURCE_FILES)
   set(
      VIRTUSB_KERNEL_SOURCE_PATH
      "${VIRTUSB_KERNEL_SOURCE_DIR}/${VIRTUSB_KERNEL_SOURCE_FILE}"
   )

   set(
      VIRTUSB_KERNEL_LINK_PATH
      "${VIRTUSB_KERNEL_OUTPUT_DIR}/${VIRTUSB_KERNEL_SOURCE_FILE}"
   )

   if(NOT EXISTS "${VIRTUSB_KERNEL_SOURCE_PATH}")
      message(
         FATAL_ERROR
         "Kernel source file does not exist:\n"
         "  ${VIRTUSB_KERNEL_SOURCE_PATH}"
      )
   endif()

   get_filename_component(
      VIRTUSB_KERNEL_LINK_DIR
      "${VIRTUSB_KERNEL_LINK_PATH}"
      DIRECTORY
   )

   file(MAKE_DIRECTORY "${VIRTUSB_KERNEL_LINK_DIR}")
   file(REMOVE "${VIRTUSB_KERNEL_LINK_PATH}")

   file(
      CREATE_LINK
      "${VIRTUSB_KERNEL_SOURCE_PATH}"
      "${VIRTUSB_KERNEL_LINK_PATH}"
      SYMBOLIC
      RESULT VIRTUSB_KERNEL_LINK_RESULT
   )

   if(NOT VIRTUSB_KERNEL_LINK_RESULT STREQUAL "0")
      message(
         FATAL_ERROR
         "Failed to create kernel source link:\n"
         "  ${VIRTUSB_KERNEL_LINK_PATH}\n"
         "Reason: ${VIRTUSB_KERNEL_LINK_RESULT}"
      )
   endif()

   string(
      REGEX REPLACE
      "\\.c$"
      ".o"
      VIRTUSB_KERNEL_OBJECT_FILE
      "${VIRTUSB_KERNEL_SOURCE_FILE}"
   )

   if(VIRTUSB_KERNEL_OBJECT_FILE STREQUAL VIRTUSB_KERNEL_SOURCE_FILE)
      message(
         FATAL_ERROR
         "Kernel source file does not use the expected .c extension:\n"
         "  ${VIRTUSB_KERNEL_SOURCE_FILE}"
      )
   endif()

   list(
      APPEND
      VIRTUSB_KBUILD_SOURCE_FILES
      "${VIRTUSB_KERNEL_LINK_PATH}"
   )

   list(
      APPEND
      VIRTUSB_KERNEL_OBJECT_FILES
      "${VIRTUSB_KERNEL_OBJECT_FILE}"
   )
endforeach()

#
# Add internal kernel headers to the generated Kbuild source view.
#

set(VIRTUSB_KBUILD_HEADER_FILES)

foreach(VIRTUSB_KERNEL_HEADER_FILE IN LISTS VIRTUSB_KERNEL_HEADER_FILES)
   set(
      VIRTUSB_KERNEL_HEADER_PATH
      "${VIRTUSB_KERNEL_SOURCE_DIR}/${VIRTUSB_KERNEL_HEADER_FILE}"
   )

   set(
      VIRTUSB_KERNEL_LINK_PATH
      "${VIRTUSB_KERNEL_OUTPUT_DIR}/${VIRTUSB_KERNEL_HEADER_FILE}"
   )

   if(NOT EXISTS "${VIRTUSB_KERNEL_HEADER_PATH}")
      message(
         FATAL_ERROR
         "Kernel header file does not exist:\n"
         "  ${VIRTUSB_KERNEL_HEADER_PATH}"
      )
   endif()

   get_filename_component(
      VIRTUSB_KERNEL_LINK_DIR
      "${VIRTUSB_KERNEL_LINK_PATH}"
      DIRECTORY
   )

   file(MAKE_DIRECTORY "${VIRTUSB_KERNEL_LINK_DIR}")
   file(REMOVE "${VIRTUSB_KERNEL_LINK_PATH}")

   file(
      CREATE_LINK
      "${VIRTUSB_KERNEL_HEADER_PATH}"
      "${VIRTUSB_KERNEL_LINK_PATH}"
      SYMBOLIC
      RESULT VIRTUSB_KERNEL_LINK_RESULT
   )

   if(NOT VIRTUSB_KERNEL_LINK_RESULT STREQUAL "0")
      message(
         FATAL_ERROR
         "Failed to create kernel header link:\n"
         "  ${VIRTUSB_KERNEL_LINK_PATH}\n"
         "Reason: ${VIRTUSB_KERNEL_LINK_RESULT}"
      )
   endif()

   list(
      APPEND
      VIRTUSB_KBUILD_HEADER_FILES
      "${VIRTUSB_KERNEL_LINK_PATH}"
   )
endforeach()

#
# Generate Kbuild from the CMake source list.
#

list(
   JOIN
   VIRTUSB_KERNEL_OBJECT_FILES
   " "
   VIRTUSB_KERNEL_OBJECT_LIST
)

set(
   VIRTUSB_KBUILD_FILE
   "${VIRTUSB_KERNEL_OUTPUT_DIR}/Kbuild"
)

file(
   CONFIGURE
   OUTPUT "${VIRTUSB_KBUILD_FILE}"
   CONTENT
"obj-m := virtusb.o

virtusb-y := ${VIRTUSB_KERNEL_OBJECT_LIST}

ccflags-y += -I\$(src)/include
"
   NEWLINE_STYLE UNIX
)

#
# Build the external kernel module.
#
# Kbuild selects the compiler and toolchain associated with the target kernel
# build tree. CMake compiler selection applies only to targets built directly
# by CMake.
#
# Kbuild performs its own dependency tracking. The CMake target invokes Kbuild
# whenever the target is selected, while Kbuild recompiles only files whose
# dependencies have changed.
#
# After the module has been built, the kernel-provided compile commands
# generator creates a compilation database from the Kbuild .cmd files.
#

add_custom_target(
   virtusb_module ALL
   COMMAND
      "${VIRTUSB_MAKE_EXECUTABLE}"
      -C "${VIRTUSB_KERNEL_BUILD_DIR}"
      "M=${VIRTUSB_KERNEL_OUTPUT_DIR}"
      modules
   COMMAND
      "${Python3_EXECUTABLE}"
      "${VIRTUSB_GEN_COMPILE_COMMANDS}"
      -d "${VIRTUSB_KERNEL_OUTPUT_DIR}"
      -o "${CMAKE_BINARY_DIR}/compile_commands.json"
      modules.order
   WORKING_DIRECTORY
      "${VIRTUSB_KERNEL_OUTPUT_DIR}"
   DEPENDS
      "${VIRTUSB_KBUILD_FILE}"
      ${VIRTUSB_KBUILD_SOURCE_FILES}
      ${VIRTUSB_KBUILD_HEADER_FILES}
   COMMENT
      "Building the VirtUSB kernel module"
   USES_TERMINAL
   VERBATIM
)

add_custom_target(
   virtusb_module_clean
   COMMAND
      "${VIRTUSB_MAKE_EXECUTABLE}"
      -C "${VIRTUSB_KERNEL_BUILD_DIR}"
      "M=${VIRTUSB_KERNEL_OUTPUT_DIR}"
      clean
   WORKING_DIRECTORY
      "${VIRTUSB_KERNEL_OUTPUT_DIR}"
   COMMENT
      "Cleaning the VirtUSB kernel module"
   USES_TERMINAL
   VERBATIM
)
