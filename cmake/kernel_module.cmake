include_guard(GLOBAL)

include(CMakeParseArguments)

function(add_linux_kernel_module)
   set(options)

   set(one_value_args
      TARGET
      MODULE_NAME
      SOURCE_DIR
      OUTPUT_DIR
      KERNEL_BUILD_DIR
      COMPILE_COMMANDS_OUTPUT
   )

   set(multi_value_args
      SOURCES
      INCLUDE_DIRECTORIES
   )

   cmake_parse_arguments(
      KMOD
      "${options}"
      "${one_value_args}"
      "${multi_value_args}"
      ${ARGN}
   )

   foreach(required_arg IN ITEMS TARGET MODULE_NAME SOURCE_DIR OUTPUT_DIR)
      if(NOT KMOD_${required_arg})
         message(
            FATAL_ERROR
            "add_linux_kernel_module(): missing required argument ${required_arg}"
         )
      endif()
   endforeach()

   if(NOT KMOD_SOURCES)
      message(
         FATAL_ERROR
         "add_linux_kernel_module(): no source files specified for "
         "${KMOD_MODULE_NAME}"
      )
   endif()

   find_program(
      KMOD_MAKE_EXECUTABLE
      NAMES make gmake
      REQUIRED
   )

   find_package(
      Python3
      REQUIRED
      COMPONENTS Interpreter
   )

   if(NOT KMOD_KERNEL_BUILD_DIR)
      execute_process(
         COMMAND uname -r
         OUTPUT_VARIABLE KMOD_RUNNING_KERNEL_RELEASE
         OUTPUT_STRIP_TRAILING_WHITESPACE
         COMMAND_ERROR_IS_FATAL ANY
      )

      set(
         KMOD_KERNEL_BUILD_DIR
         "/lib/modules/${KMOD_RUNNING_KERNEL_RELEASE}/build"
      )
   endif()

   if(NOT EXISTS "${KMOD_KERNEL_BUILD_DIR}/Makefile")
      message(
         FATAL_ERROR
         "Linux kernel build directory is unavailable:\n"
         "  ${KMOD_KERNEL_BUILD_DIR}\n"
         "Install the matching kernel headers or pass KERNEL_BUILD_DIR explicitly."
      )
   endif()

   set(
      KMOD_GEN_COMPILE_COMMANDS
      "${KMOD_KERNEL_BUILD_DIR}/scripts/clang-tools/gen_compile_commands.py"
   )

   if(NOT EXISTS "${KMOD_GEN_COMPILE_COMMANDS}")
      message(
         FATAL_ERROR
         "Kernel compile commands generator is unavailable:\n"
         "  ${KMOD_GEN_COMPILE_COMMANDS}"
      )
   endif()

   if(NOT KMOD_COMPILE_COMMANDS_OUTPUT)
      set(
         KMOD_COMPILE_COMMANDS_OUTPUT
         "${CMAKE_BINARY_DIR}/compile_commands.json"
      )
   endif()

   get_filename_component(
      KMOD_SOURCE_DIR
      "${KMOD_SOURCE_DIR}"
      ABSOLUTE
   )

   get_filename_component(
      KMOD_OUTPUT_DIR
      "${KMOD_OUTPUT_DIR}"
      ABSOLUTE
   )

   file(MAKE_DIRECTORY "${KMOD_OUTPUT_DIR}")

   set(kbuild_source_files)
   set(kernel_object_files)

   foreach(source_file IN LISTS KMOD_SOURCES)
      if(IS_ABSOLUTE "${source_file}")
         message(
            FATAL_ERROR
            "add_linux_kernel_module(): SOURCES must be relative to SOURCE_DIR:\n"
            "  ${source_file}"
         )
      endif()

      if(NOT source_file MATCHES "\\.c$")
         message(
            FATAL_ERROR
            "add_linux_kernel_module(): kernel source file must use the .c extension:\n"
            "  ${source_file}"
         )
      endif()

      set(
         source_path
         "${KMOD_SOURCE_DIR}/${source_file}"
      )

      if(NOT EXISTS "${source_path}")
         message(
            FATAL_ERROR
            "Kernel source file does not exist:\n"
            "  ${source_path}"
         )
      endif()

      set(
         link_path
         "${KMOD_OUTPUT_DIR}/${source_file}"
      )

      get_filename_component(
         link_dir
         "${link_path}"
         DIRECTORY
      )

      file(MAKE_DIRECTORY "${link_dir}")
      file(REMOVE "${link_path}")

      file(
         CREATE_LINK
         "${source_path}"
         "${link_path}"
         SYMBOLIC
         RESULT link_result
      )

      if(NOT link_result STREQUAL "0")
         message(
            FATAL_ERROR
            "Failed to create kernel source link:\n"
            "  ${link_path}\n"
            "Reason: ${link_result}"
         )
      endif()

      string(
         REGEX REPLACE
         "\\.c$"
         ".o"
         object_file
         "${source_file}"
      )

      list(
         APPEND
         kbuild_source_files
         "${link_path}"
      )

      list(
         APPEND
         kernel_object_files
         "${object_file}"
      )
   endforeach()

   list(
      JOIN
      kernel_object_files
      " "
      kernel_object_list
   )

   set(kbuild_include_flags)

   foreach(include_dir IN LISTS KMOD_INCLUDE_DIRECTORIES)
      get_filename_component(
         include_path
         "${include_dir}"
         ABSOLUTE
      )

      if(NOT IS_DIRECTORY "${include_path}")
         message(
            FATAL_ERROR
            "Kernel include directory does not exist:\n"
            "  ${include_path}"
         )
      endif()

      string(
         APPEND
         kbuild_include_flags
         "ccflags-y += -I${include_path}\n"
      )
   endforeach()

   set(
      kbuild_file
      "${KMOD_OUTPUT_DIR}/Kbuild"
   )

   file(
      CONFIGURE
      OUTPUT "${kbuild_file}"
      CONTENT
"obj-m := ${KMOD_MODULE_NAME}.o

${KMOD_MODULE_NAME}-y := ${kernel_object_list}

${kbuild_include_flags}"
      NEWLINE_STYLE UNIX
   )

   add_custom_target(
      "${KMOD_TARGET}"
      ALL
      COMMAND
         "${KMOD_MAKE_EXECUTABLE}"
         -C "${KMOD_KERNEL_BUILD_DIR}"
         "M=${KMOD_OUTPUT_DIR}"
         modules
      COMMAND
         "${Python3_EXECUTABLE}"
         "${KMOD_GEN_COMPILE_COMMANDS}"
         -d "${KMOD_OUTPUT_DIR}"
         -o "${KMOD_COMPILE_COMMANDS_OUTPUT}"
         modules.order
      WORKING_DIRECTORY
         "${KMOD_OUTPUT_DIR}"
      DEPENDS
         "${kbuild_file}"
         ${kbuild_source_files}
      COMMENT
         "Building Linux kernel module ${KMOD_MODULE_NAME}"
      USES_TERMINAL
      VERBATIM
   )

   add_custom_target(
      "${KMOD_TARGET}_clean"
      COMMAND
         "${KMOD_MAKE_EXECUTABLE}"
         -C "${KMOD_KERNEL_BUILD_DIR}"
         "M=${KMOD_OUTPUT_DIR}"
         clean
      WORKING_DIRECTORY
         "${KMOD_OUTPUT_DIR}"
      COMMENT
         "Cleaning Linux kernel module ${KMOD_MODULE_NAME}"
      USES_TERMINAL
      VERBATIM
   )
endfunction()
