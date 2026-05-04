cmake_minimum_required(VERSION 3.20)

project(embedded_project LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Let toolchains mark target family. Expected: arm or avr
if(NOT DEFINED EMB_TARGET_FAMILY)
  message(FATAL_ERROR "EMB_TARGET_FAMILY is not set. Use a toolchain file from cmake/toolchains.")
endif()

set(ARM_MCU "" CACHE STRING "ARM MCU (e.g. stm32f103, stm32f411)")
set(EMB_PROJECT "" CACHE STRING "Project directory name under projects/ to build only one project")
set(EMB_UPLOAD_PROJECT "" CACHE STRING "Project name used by upload/monitor alias targets")

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/arduino_libs.cmake")
  include(cmake/arduino_libs.cmake)
endif()

if(EMB_TARGET_FAMILY STREQUAL "avr")
  message(STATUS "ARDUINO_CORE_PATH=${ARDUINO_CORE_PATH}")
  if(NOT ARDUINO_CORE_PATH)
    message(FATAL_ERROR "ARDUINO_CORE_PATH is required for AVR builds")
  endif()
  include(cmake/arduino.cmake)
endif()

# Derive Arduino version defines from core path (.../avr/1.8.7/cores/arduino).
set(ARDUINO_VERSION_MAJOR 0)
set(ARDUINO_VERSION_MINOR 0)
set(ARDUINO_VERSION_PATCH 0)
if(EMB_TARGET_FAMILY STREQUAL "avr" AND ARDUINO_CORE_PATH)
  get_filename_component(_core_dir "${ARDUINO_CORE_PATH}" DIRECTORY)
  get_filename_component(_avr_dir "${_core_dir}" DIRECTORY)
  get_filename_component(ARDUINO_VERSION_STRING "${_avr_dir}" NAME)
  if(ARDUINO_VERSION_STRING MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    set(ARDUINO_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(ARDUINO_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(ARDUINO_VERSION_PATCH "${CMAKE_MATCH_3}")
  endif()
endif()
math(EXPR ARDUINO_VERSION "${ARDUINO_VERSION_MAJOR} * 10000 + ${ARDUINO_VERSION_MINOR} * 100 + ${ARDUINO_VERSION_PATCH}")

function(emb_apply_common_target_settings target_name)
  target_compile_definitions(${target_name} PRIVATE
    $<$<STREQUAL:${EMB_TARGET_FAMILY},avr>:EMB_TARGET_AVR>
    $<$<STREQUAL:${EMB_TARGET_FAMILY},arm>:EMB_TARGET_ARM>
  )

  target_include_directories(${target_name} PRIVATE
    src
    src/common
  )

  if(EMB_TARGET_FAMILY STREQUAL "avr")
    target_compile_definitions(${target_name} PRIVATE
      ARDUINO=${ARDUINO_VERSION}
      ARDUINO_AVR_BOARD=1
      ARDUINO_ARCH_AVR
      ARDUINO_VERSION_MAJOR=${ARDUINO_VERSION_MAJOR}
      ARDUINO_VERSION_MINOR=${ARDUINO_VERSION_MINOR}
      ARDUINO_VERSION_PATCH=${ARDUINO_VERSION_PATCH}
    )

    # Keep size optimization enabled for all AVR targets.
    target_compile_options(${target_name} PRIVATE -Os)
    target_link_options(${target_name} PRIVATE -Wl,--gc-sections -Os)

    # Use C++ linker for Arduino libraries even when the project entry file is main.c.
    set_property(TARGET ${target_name} PROPERTY LINKER_LANGUAGE CXX)

    target_link_libraries(${target_name} PRIVATE arduino_core)
    if(TARGET arduino_libs)
      target_link_libraries(${target_name} PRIVATE arduino_libs)
    endif()

    # Enable LTO only on final firmware targets.
    target_compile_options(${target_name} PRIVATE -flto)
    target_link_options(${target_name} PRIVATE -flto)
  endif()

  if(EMB_TARGET_FAMILY STREQUAL "arm")
    set(LINKER_SCRIPT "" CACHE FILEPATH "Path to ARM linker script (.ld)")
    if(ARM_MCU STREQUAL "stm32f103")
      target_sources(${target_name} PRIVATE
        src/arm/stm32f103/startup_stm32f103.c
      )
      target_compile_definitions(${target_name} PRIVATE STM32F103xB)
      if(NOT LINKER_SCRIPT)
        set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/arm/stm32f103/linker.ld" CACHE FILEPATH "Path to STM32F103 linker script" FORCE)
      endif()
    endif()
    if(NOT LINKER_SCRIPT)
      message(FATAL_ERROR "LINKER_SCRIPT is required for ARM builds")
    endif()
    target_link_options(${target_name} PRIVATE
      -T"${LINKER_SCRIPT}"
      -Wl,--gc-sections
    )
  endif()
endfunction()

if(TARGET arduino_libs)
  target_compile_definitions(arduino_libs PRIVATE
    ARDUINO=${ARDUINO_VERSION}
    ARDUINO_AVR_BOARD=1
    ARDUINO_ARCH_AVR
    ARDUINO_VERSION_MAJOR=${ARDUINO_VERSION_MAJOR}
    ARDUINO_VERSION_MINOR=${ARDUINO_VERSION_MINOR}
    ARDUINO_VERSION_PATCH=${ARDUINO_VERSION_PATCH}
  )
  target_compile_options(arduino_libs PRIVATE -Os)
endif()

set(PROJECTS_ROOT "${CMAKE_SOURCE_DIR}/projects")
if(NOT EXISTS "${PROJECTS_ROOT}")
  message(FATAL_ERROR "projects directory not found: ${PROJECTS_ROOT}")
endif()

set(EMB_COMMON_SOURCES)
file(GLOB_RECURSE EMB_COMMON_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/common/*.c"
  "${CMAKE_SOURCE_DIR}/src/common/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/common/*.S"
)
list(FILTER EMB_COMMON_SOURCES EXCLUDE REGEX ".*/main\\.(c|cpp)$")

set(_project_dirs)
file(GLOB _candidate_project_dirs RELATIVE "${PROJECTS_ROOT}" "${PROJECTS_ROOT}/*")
foreach(_project_name ${_candidate_project_dirs})
  if(IS_DIRECTORY "${PROJECTS_ROOT}/${_project_name}" AND EXISTS "${PROJECTS_ROOT}/${_project_name}/main.c")
    list(APPEND _project_dirs "${_project_name}")
  endif()
endforeach()

if(NOT _project_dirs)
  message(FATAL_ERROR "No projects found. Add projects/<name>/main.c")
endif()

if(EMB_PROJECT)
  list(FIND _project_dirs "${EMB_PROJECT}" _project_idx)
  if(_project_idx EQUAL -1)
    message(FATAL_ERROR "Requested EMB_PROJECT='${EMB_PROJECT}' was not found under projects/")
  endif()
  set(_project_dirs "${EMB_PROJECT}")
endif()

set(EMB_FIRMWARE_TARGETS)
set(EMB_PROJECT_NAMES)

foreach(_project_name ${_project_dirs})
  set(_project_main "${PROJECTS_ROOT}/${_project_name}/main.c")
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _project_id "${_project_name}")
  set(_firmware_target "firmware_${_project_id}")

  if(EMB_TARGET_FAMILY STREQUAL "avr")
    # Keep main.c naming convention while allowing Arduino C++ APIs.
    set_source_files_properties(${_project_main} PROPERTIES LANGUAGE CXX)
  endif()

  add_executable(${_firmware_target}
    ${_project_main}
    ${EMB_COMMON_SOURCES}
  )

  emb_apply_common_target_settings(${_firmware_target})

  list(APPEND EMB_FIRMWARE_TARGETS ${_firmware_target})
  list(APPEND EMB_PROJECT_NAMES ${_project_name})
endforeach()

if(EMB_FIRMWARE_TARGETS)
  add_custom_target(firmware_all DEPENDS ${EMB_FIRMWARE_TARGETS})
endif()

if(EMB_TARGET_FAMILY STREQUAL "avr")
  foreach(_firmware_target ${EMB_FIRMWARE_TARGETS})
    add_custom_command(
      TARGET ${_firmware_target}
      POST_BUILD
      COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${_firmware_target}> $<TARGET_FILE_DIR:${_firmware_target}>/${_firmware_target}.hex
      COMMENT "Generating HEX file for ${_firmware_target}"
    )

    add_custom_command(
      TARGET ${_firmware_target}
      POST_BUILD
      COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${_firmware_target}>
      COMMENT "Firmware size for ${_firmware_target}"
    )

    add_custom_command(
      TARGET ${_firmware_target}
      POST_BUILD
      COMMAND ${CMAKE_SIZE} -C --mcu=${AVR_MCU} $<TARGET_FILE:${_firmware_target}>
      COMMENT "AVR memory summary for ${_firmware_target}"
    )
  endforeach()

  set(AVRDUDE_BAUD "19200" CACHE STRING "AVRDUDE baud rate")
  set(AVRDUDE_PROGRAMMER "avrisp" CACHE STRING "AVRDUDE programmer type")
  set(AVRDUDE_PORT "" CACHE STRING "AVRDUDE port (auto-detected if empty)")
  set(SERIAL_TOOL "stty" CACHE STRING "Serial monitor tool (stty, screen, picocom, miniterm)")
  set(SERIAL_BAUD "57600" CACHE STRING "Serial monitor baud rate")

  get_filename_component(_arduino_core_dir "${ARDUINO_CORE_PATH}" DIRECTORY)
  get_filename_component(_arduino_avr_dir "${_arduino_core_dir}" DIRECTORY)
  get_filename_component(_arduino_hw_dir "${_arduino_avr_dir}" DIRECTORY)
  get_filename_component(_arduino_pkg_dir "${_arduino_hw_dir}" DIRECTORY)
  set(_arduino_tools_dir "${_arduino_pkg_dir}/tools")
  file(GLOB _avrdude_dirs "${_arduino_tools_dir}/avrdude/*")
  list(LENGTH _avrdude_dirs _avrdude_count)
  if(_avrdude_count GREATER 0)
    list(SORT _avrdude_dirs)
    list(REVERSE _avrdude_dirs)
    list(GET _avrdude_dirs 0 _avrdude_dir)
    set(ARDUINO_AVRDUDE "${_avrdude_dir}/bin/avrdude" CACHE FILEPATH "Path to avrdude")
    set(ARDUINO_AVRDUDE_CONF "${_avrdude_dir}/etc/avrdude.conf" CACHE FILEPATH "Path to avrdude.conf")
  else()
    message(WARNING "No avrdude found under ${_arduino_tools_dir}. Set ARDUINO_AVRDUDE manually.")
  endif()

  if(NOT ARDUINO_AVRDUDE)
    find_program(ARDUINO_AVRDUDE avrdude)
  endif()
  if(NOT ARDUINO_AVRDUDE_CONF)
    if(EXISTS "/etc/avrdude.conf")
      set(ARDUINO_AVRDUDE_CONF "/etc/avrdude.conf" CACHE FILEPATH "Path to avrdude.conf" FORCE)
    endif()
  endif()

  if(NOT AVRDUDE_PORT)
    file(GLOB _tty_usb "/dev/ttyUSB*")
    file(GLOB _tty_acm "/dev/ttyACM*")
    set(_tty_all ${_tty_usb} ${_tty_acm})
    list(LENGTH _tty_all _tty_count)
    if(_tty_count EQUAL 1)
      list(GET _tty_all 0 AVRDUDE_PORT)
    elseif(_tty_count EQUAL 0)
      message(FATAL_ERROR "No serial ports found. Set AVRDUDE_PORT=/dev/ttyUSB0")
    else()
      message(FATAL_ERROR "Multiple serial ports found: ${_tty_all}. Set AVRDUDE_PORT explicitly.")
    endif()
  endif()

  set(_avrdude_conf_arg)
  if(ARDUINO_AVRDUDE_CONF)
    set(_avrdude_conf_arg -C ${ARDUINO_AVRDUDE_CONF})
  endif()

  list(LENGTH EMB_FIRMWARE_TARGETS _firmware_count)
  if(_firmware_count GREATER 0)
    math(EXPR _last_firmware_idx "${_firmware_count} - 1")
    foreach(_idx RANGE 0 ${_last_firmware_idx})
      list(GET EMB_FIRMWARE_TARGETS ${_idx} _firmware_target)
      list(GET EMB_PROJECT_NAMES ${_idx} _project_name)
      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _project_id "${_project_name}")

      if(ARDUINO_AVRDUDE)
        add_custom_target(upload_${_project_id}
          COMMAND ${ARDUINO_AVRDUDE} ${_avrdude_conf_arg} -v -p ${AVR_MCU} -c ${AVRDUDE_PROGRAMMER} -P ${AVRDUDE_PORT} -b ${AVRDUDE_BAUD} -D -U flash:w:$<TARGET_FILE_DIR:${_firmware_target}>/${_firmware_target}.hex:i
          DEPENDS ${_firmware_target}
          COMMENT "Uploading ${_project_name} via avrdude"
        )
      else()
        add_custom_target(upload_${_project_id}
          COMMAND ${CMAKE_COMMAND} -E echo "avrdude not found. Set ARDUINO_AVRDUDE or install avrdude."
          COMMAND ${CMAKE_COMMAND} -E false
          COMMENT "Uploading ${_project_name} via avrdude"
        )
      endif()

      add_custom_target(monitor_${_project_id}
        COMMAND ${CMAKE_COMMAND} -E echo "Opening ${AVRDUDE_PORT} at ${SERIAL_BAUD} using ${SERIAL_TOOL}"
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} ${CMAKE_COMMAND} -E env
          SERIAL_TOOL=${SERIAL_TOOL}
          SERIAL_PORT=${AVRDUDE_PORT}
          SERIAL_BAUD=${SERIAL_BAUD}
          ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/serial_monitor.cmake
        COMMENT "Starting serial monitor for ${_project_name}"
      )

      add_custom_target(upload_monitor_${_project_id}
        DEPENDS upload_${_project_id} monitor_${_project_id}
        COMMENT "Upload then monitor ${_project_name}"
      )
    endforeach()
  endif()

  if(NOT EMB_UPLOAD_PROJECT)
    list(GET EMB_PROJECT_NAMES 0 EMB_UPLOAD_PROJECT)
  endif()

  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _upload_project_id "${EMB_UPLOAD_PROJECT}")
  if(TARGET upload_${_upload_project_id})
    add_custom_target(upload DEPENDS upload_${_upload_project_id})
  endif()
  if(TARGET monitor_${_upload_project_id})
    add_custom_target(monitor DEPENDS monitor_${_upload_project_id})
  endif()
  if(TARGET upload_monitor_${_upload_project_id})
    add_custom_target(upload_monitor DEPENDS upload_monitor_${_upload_project_id})
  endif()
endif()
