set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_ASM_COMPILER avr-gcc)

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
