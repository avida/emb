add_library(arduino_core)

file(GLOB_RECURSE ARDUINO_CORE_SOURCES
  "${ARDUINO_CORE_PATH}/*.c"
  "${ARDUINO_CORE_PATH}/*.cpp"
  "${ARDUINO_CORE_PATH}/*.S"
)

target_sources(arduino_core PRIVATE ${ARDUINO_CORE_SOURCES})

target_compile_options(arduino_core PRIVATE -Os)

target_include_directories(arduino_core PUBLIC
  "${ARDUINO_CORE_PATH}"
)

# Some AVR toolchains do not define DECIMAL_DIG, but Arduino core uses it.
target_compile_definitions(arduino_core PRIVATE DECIMAL_DIG=10)

if(ARDUINO_VARIANT_PATH)
  target_include_directories(arduino_core PUBLIC
    "${ARDUINO_VARIANT_PATH}"
  )
endif()
