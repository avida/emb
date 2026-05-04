set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)


# Try to auto-detect Arduino CLI data dir and AVR GCC toolchain
if(NOT DEFINED ARDUINO_CLI_DATA_DIR)
  if(DEFINED ENV{ARDUINO_CLI_DATA_DIR})
    set(ARDUINO_CLI_DATA_DIR $ENV{ARDUINO_CLI_DATA_DIR})
  else()
    set(ARDUINO_CLI_DATA_DIR "$ENV{HOME}/.arduino15")
  endif()
endif()

# Find latest AVR GCC toolchain under Arduino packages
file(GLOB _avr_gcc_dirs
  "${ARDUINO_CLI_DATA_DIR}/packages/arduino/tools/avr-gcc/*"
)
list(SORT _avr_gcc_dirs)
list(REVERSE _avr_gcc_dirs)
list(GET _avr_gcc_dirs 0 _avr_gcc_dir)

if(EXISTS "${_avr_gcc_dir}/bin/avr-gcc")
  set(CMAKE_C_COMPILER "${_avr_gcc_dir}/bin/avr-gcc")
  set(CMAKE_CXX_COMPILER "${_avr_gcc_dir}/bin/avr-g++")
  set(CMAKE_ASM_COMPILER "${_avr_gcc_dir}/bin/avr-gcc")
  set(CMAKE_OBJCOPY "${_avr_gcc_dir}/bin/avr-objcopy" CACHE STRING "objcopy")
  set(CMAKE_OBJDUMP "${_avr_gcc_dir}/bin/avr-objdump" CACHE STRING "objdump")
  set(CMAKE_SIZE "${_avr_gcc_dir}/bin/avr-size" CACHE STRING "size")
else()
  message(WARNING "Arduino AVR GCC toolchain not found, falling back to system avr-gcc.")
  set(CMAKE_C_COMPILER avr-gcc)
  set(CMAKE_CXX_COMPILER avr-g++)
  set(CMAKE_ASM_COMPILER avr-gcc)
  set(CMAKE_OBJCOPY avr-objcopy CACHE STRING "objcopy")
  set(CMAKE_OBJDUMP avr-objdump CACHE STRING "objdump")
  set(CMAKE_SIZE avr-size CACHE STRING "size")
endif()

set(EMB_TARGET_FAMILY avr CACHE STRING "Target family" FORCE)

set(AVR_MCU "atmega328p" CACHE STRING "AVR MCU (e.g. atmega328p, atmega2560)")
set(AVR_F_CPU "16000000UL" CACHE STRING "CPU frequency in Hz (e.g. 16000000UL)")

add_compile_options(
  -mmcu=${AVR_MCU}
  -DF_CPU=${AVR_F_CPU}
  -ffunction-sections
  -fdata-sections
)

add_link_options(
  -mmcu=${AVR_MCU}
)

set(CMAKE_OBJCOPY avr-objcopy CACHE STRING "objcopy")
set(CMAKE_OBJDUMP avr-objdump CACHE STRING "objdump")
set(CMAKE_SIZE avr-size CACHE STRING "size")
