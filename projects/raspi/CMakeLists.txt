set(RASPI_RUST_DIR "${CMAKE_SOURCE_DIR}/projects/raspi" CACHE PATH "Path to Raspberry Pi Rust project")
if(EXISTS "${RASPI_RUST_DIR}/Cargo.toml")
  set(RASPI_RUST_TARGET "arm-unknown-linux-musleabihf" CACHE STRING "Rust target triple for Raspberry Pi build (empty means native host build)")
  set(RASPI_RUST_PROFILE "release" CACHE STRING "Rust profile for Raspberry Pi build (debug/release)")
  set(RASPI_RUST_LINKER "rust-lld" CACHE STRING "Optional linker override for the selected Rust target")

  set(_raspi_profile_arg)
  if(RASPI_RUST_PROFILE STREQUAL "release")
    set(_raspi_profile_arg --release)
  endif()

  set(_raspi_target_arg)
  if(RASPI_RUST_TARGET)
    set(_raspi_target_arg --target ${RASPI_RUST_TARGET})
  endif()

  set(_raspi_env CARGO_TERM_COLOR=always)
  if(RASPI_RUST_TARGET AND RASPI_RUST_LINKER)
    string(REPLACE "-" "_" _raspi_target_env "${RASPI_RUST_TARGET}")
    string(TOUPPER "${_raspi_target_env}" _raspi_target_env)
    list(APPEND _raspi_env "CARGO_TARGET_${_raspi_target_env}_LINKER=${RASPI_RUST_LINKER}")
  endif()

  find_program(CARGO_EXECUTABLE cargo)
  if(CARGO_EXECUTABLE)
    add_custom_target(raspi
      COMMAND ${CMAKE_COMMAND} -E env
        ${_raspi_env}
        ${CARGO_EXECUTABLE} build --manifest-path "${RASPI_RUST_DIR}/Cargo.toml" ${_raspi_target_arg} ${_raspi_profile_arg}
      WORKING_DIRECTORY "${RASPI_RUST_DIR}"
      COMMENT "Building Raspberry Pi Rust app (${RASPI_RUST_PROFILE}) from ${RASPI_RUST_DIR}"
      VERBATIM
    )
  else()
    add_custom_target(raspi
      COMMAND ${CMAKE_COMMAND} -E echo "cargo not found. Install Rust toolchain on the Raspberry Pi (https://rustup.rs/)."
      COMMAND ${CMAKE_COMMAND} -E false
      COMMENT "Building Raspberry Pi Rust app from ${RASPI_RUST_DIR}"
    )
  endif()
endif()
