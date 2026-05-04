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

add_executable(firmware
  src/common/main.cpp
)

# Add Arduino version defines after target is created
if(EMB_TARGET_FAMILY STREQUAL "avr")
  # Extract Arduino version from ARDUINO_CORE_PATH (e.g., .../avr/1.8.7/cores/arduino)
  get_filename_component(_core_dir "${ARDUINO_CORE_PATH}" DIRECTORY)
  get_filename_component(_avr_dir "${_core_dir}" DIRECTORY)
  get_filename_component(_version_dir "${_avr_dir}" NAME)
  set(ARDUINO_VERSION_STRING "${_version_dir}")
  string(REPLACE "." ";" _ver_list "${ARDUINO_VERSION_STRING}")
  list(GET _ver_list 0 ARDUINO_VERSION_MAJOR)
  list(GET _ver_list 1 ARDUINO_VERSION_MINOR)
  list(GET _ver_list 2 ARDUINO_VERSION_PATCH)
  math(EXPR ARDUINO_VERSION "${ARDUINO_VERSION_MAJOR} * 10000 + ${ARDUINO_VERSION_MINOR} * 100 + ${ARDUINO_VERSION_PATCH}")
  foreach(_tgt firmware arduino_libs)
    if(TARGET ${_tgt})
      target_compile_definitions(${_tgt} PRIVATE
        ARDUINO=${ARDUINO_VERSION}
        ARDUINO_AVR_BOARD=1
        ARDUINO_ARCH_AVR
        ARDUINO_VERSION_MAJOR=${ARDUINO_VERSION_MAJOR}
        ARDUINO_VERSION_MINOR=${ARDUINO_VERSION_MINOR}
        ARDUINO_VERSION_PATCH=${ARDUINO_VERSION_PATCH}
      )
      target_compile_options(${_tgt} PRIVATE -Os)
      target_link_options(${_tgt} PRIVATE -Os)
    endif()
  endforeach()
  # Only apply LTO to the final executable
  if(TARGET firmware)
    target_compile_options(firmware PRIVATE -flto)
    target_link_options(firmware PRIVATE -flto)
  endif()
endif()


target_compile_definitions(firmware PRIVATE
  $<$<STREQUAL:${EMB_TARGET_FAMILY},avr>:EMB_TARGET_AVR>
  $<$<STREQUAL:${EMB_TARGET_FAMILY},arm>:EMB_TARGET_ARM>
)

target_include_directories(firmware PRIVATE
  src
)

if(EMB_TARGET_FAMILY STREQUAL "avr")
  target_link_libraries(firmware PRIVATE arduino_core)
endif()

if(TARGET arduino_libs)
  target_link_libraries(firmware PRIVATE arduino_libs)
endif()

if(EMB_TARGET_FAMILY STREQUAL "arm")
  set(LINKER_SCRIPT "" CACHE FILEPATH "Path to ARM linker script (.ld)")
  if(ARM_MCU STREQUAL "stm32f103")
    target_sources(firmware PRIVATE
      src/arm/stm32f103/startup_stm32f103.c
    )
    target_compile_definitions(firmware PRIVATE STM32F103xB)
    if(NOT LINKER_SCRIPT)
      set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/arm/stm32f103/linker.ld" CACHE FILEPATH "Path to STM32F103 linker script" FORCE)
    endif()
  endif()
  if(NOT LINKER_SCRIPT)
    message(FATAL_ERROR "LINKER_SCRIPT is required for ARM builds")
  endif()
  target_link_options(firmware PRIVATE
    -T"${LINKER_SCRIPT}"
    -Wl,--gc-sections
  )
endif()

if(EMB_TARGET_FAMILY STREQUAL "avr")
  target_compile_options(firmware PRIVATE -Os)
  target_link_options(firmware PRIVATE
    -Wl,--gc-sections
  )

  add_custom_command(
    TARGET firmware
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:firmware> $<TARGET_FILE_DIR:firmware>/firmware.hex
    COMMENT "Generating HEX file"
  )
    add_custom_command(
      TARGET firmware
      POST_BUILD
      COMMAND ${CMAKE_SIZE} $<TARGET_FILE:firmware>
      COMMENT "Firmware size"
    )

    add_custom_command(
      TARGET firmware
      POST_BUILD
      COMMAND ${CMAKE_SIZE} -C --mcu=${AVR_MCU} $<TARGET_FILE:firmware>
      COMMENT "AVR memory summary"
    )

  set(AVRDUDE_BAUD "19200" CACHE STRING "AVRDUDE baud rate")
  set(AVRDUDE_PROGRAMMER "avrisp" CACHE STRING "AVRDUDE programmer type")
  set(AVRDUDE_PORT "" CACHE STRING "AVRDUDE port (auto-detected if empty)")
  set(SERIAL_TOOL "stty" CACHE STRING "Serial monitor tool (stty, screen, picocom, miniterm)")
  set(SERIAL_BAUD "57600" CACHE STRING "Serial monitor baud rate")
  set(AVRDUDE_AUTO_MONITOR ON CACHE BOOL "Open serial monitor after upload")

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

  if(ARDUINO_AVRDUDE)
    if(AVRDUDE_AUTO_MONITOR)
      add_custom_target(upload
        COMMAND ${ARDUINO_AVRDUDE} ${_avrdude_conf_arg} -v -p ${AVR_MCU} -c ${AVRDUDE_PROGRAMMER} -P ${AVRDUDE_PORT} -b ${AVRDUDE_BAUD} -D -U flash:w:$<TARGET_FILE_DIR:firmware>/firmware.hex:i
        COMMAND ${CMAKE_COMMAND} -E echo "Opening ${AVRDUDE_PORT} at ${SERIAL_BAUD} using ${SERIAL_TOOL}"
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} ${CMAKE_COMMAND} -E env
          SERIAL_TOOL=${SERIAL_TOOL}
          SERIAL_PORT=${AVRDUDE_PORT}
          SERIAL_BAUD=${SERIAL_BAUD}
          ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/serial_monitor.cmake
        DEPENDS firmware
        COMMENT "Uploading firmware via avrdude"
      )
    else()
      add_custom_target(upload
        COMMAND ${ARDUINO_AVRDUDE} ${_avrdude_conf_arg} -v -p ${AVR_MCU} -c ${AVRDUDE_PROGRAMMER} -P ${AVRDUDE_PORT} -b ${AVRDUDE_BAUD} -D -U flash:w:$<TARGET_FILE_DIR:firmware>/firmware.hex:i
        DEPENDS firmware
        COMMENT "Uploading firmware via avrdude"
      )
    endif()
  else()
    add_custom_target(upload
      COMMAND ${CMAKE_COMMAND} -E echo "avrdude not found. Set ARDUINO_AVRDUDE or install avrdude."
      COMMAND ${CMAKE_COMMAND} -E false
      COMMENT "Uploading firmware via avrdude"
    )
  endif()

  add_custom_target(monitor
    COMMAND ${CMAKE_COMMAND} -E echo "Opening ${AVRDUDE_PORT} at ${SERIAL_BAUD} using ${SERIAL_TOOL}"
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} ${CMAKE_COMMAND} -E env
      SERIAL_TOOL=${SERIAL_TOOL}
      SERIAL_PORT=${AVRDUDE_PORT}
      SERIAL_BAUD=${SERIAL_BAUD}
      ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/serial_monitor.cmake
    COMMENT "Starting serial monitor"
  )

  add_custom_target(upload_monitor
    DEPENDS upload monitor
    COMMENT "Upload then monitor"
  )
endif()
