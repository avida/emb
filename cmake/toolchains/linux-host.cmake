set(CMAKE_SYSTEM_NAME Linux)

# Host/Linux builds are used for utility targets such as the Raspberry Pi Rust app.
set(EMB_TARGET_FAMILY host CACHE STRING "Target family" FORCE)