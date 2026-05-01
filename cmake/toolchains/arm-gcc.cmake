set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(EMB_TARGET_FAMILY arm CACHE STRING "Target family" FORCE)

set(ARM_CPU "cortex-m3" CACHE STRING "ARM CPU (e.g. cortex-m0, cortex-m3, cortex-m4)")
set(ARM_FPU "" CACHE STRING "ARM FPU (e.g. fpv4-sp-d16)")
set(ARM_FLOAT_ABI "" CACHE STRING "ARM float ABI (soft, softfp, hard)")

add_compile_options(
  -mcpu=${ARM_CPU}
  -mthumb
  -ffunction-sections
  -fdata-sections
)

if(ARM_FPU)
  add_compile_options(-mfpu=${ARM_FPU})
endif()
if(ARM_FLOAT_ABI)
  add_compile_options(-mfloat-abi=${ARM_FLOAT_ABI})
endif()

add_link_options(
  -mcpu=${ARM_CPU}
  -mthumb
)

set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE STRING "objcopy")
set(CMAKE_OBJDUMP arm-none-eabi-objdump CACHE STRING "objdump")
set(CMAKE_SIZE arm-none-eabi-size CACHE STRING "size")
