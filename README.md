# Embedded C/C++ Project (CMake)

This project provides a clean CMake-based structure for building firmware for ARM Cortex-M and AVR (ATmega) microcontrollers. It is designed to integrate Arduino core source code while keeping full control over the build.

## Installation

See the full installation instructions in [readme/INSTALLATION.md](readme/INSTALLATION.md).

## Prerequisites

- CMake 3.20+
- GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`)
- AVR-GCC toolchain (`avr-gcc`)
- Ninja build tool (recommended for presets: `sudo apt install ninja-build`)
- Arduino core source files (optional)

## Project Layout

- `cmake/toolchains/arm-gcc.cmake` - ARM GCC toolchain
- `cmake/toolchains/avr-gcc.cmake` - AVR GCC toolchain
- `cmake/arduino.cmake` - Arduino core integration helper
- `projects/<name>/main.c` - Per-project entry point
- `src/common/` - Shared code used by all projects

## Multi-Project Build Model

- Each firmware project must live in `projects/<project-name>/`.
- Each project directory must contain `main.c`.
- Shared code goes in `src/common/` and is compiled into every project target.

CMake auto-discovers all projects under `projects/`.

Generated target names:

- `firmware_<project-name>` for each project
- `firmware_all` to build all discovered projects

To configure only one project, set `EMB_PROJECT=<project-name>`.

## Build: AVR (ATmega)

```sh
cmake -S . -B build-avr \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/avr-gcc.cmake \
  -DAVR_MCU=atmega168 \
  -DAVR_F_CPU=16000000UL
cmake --build build-avr --target firmware_all
```

To build for ATmega328P, change `-DAVR_MCU=atmega328p`.

Build one project target:

```sh
cmake --build build-avr --target firmware_default
```

Configure for a single project only:

```sh
cmake -S . -B build-avr \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/avr-gcc.cmake \
  -DAVR_MCU=atmega168 \
  -DAVR_F_CPU=16000000UL \
  -DEMB_PROJECT=default
cmake --build build-avr
```

### Short Commands (CMake Presets)

If you want short commands, use presets:

Note: The presets work best with Ninja installed (`sudo apt install ninja-build`).

```sh
cmake --preset avr-168
cmake --build --preset avr-168 --target firmware_all
```

For ATmega328P:

```sh
cmake --preset avr-328p
cmake --build --preset avr-328p --target firmware_all
```

### Upload (AVR)

After building, upload with the per-project upload target.

```sh
cmake --build --preset avr-168
cmake --build --preset avr-168 --target upload_default
```

Alias targets `upload`, `monitor`, and `upload_monitor` point to `EMB_UPLOAD_PROJECT` (defaults to first discovered project).

Defaults: `AVRDUDE_PROGRAMMER=avrisp`, `AVRDUDE_BAUD=19200`.

### Serial Monitor (AVR)

Start a serial monitor (default: `stty` + `cat` at 57600):

```sh
cmake --build --preset avr-168 --target monitor
```

Upload then monitor:

```sh
cmake --build --preset avr-168 --target upload_monitor
```

To change monitor tool or baud:

```sh
cmake --preset avr-168 -DSERIAL_TOOL=picocom -DSERIAL_BAUD=57600
cmake --build --preset avr-168 --target monitor
```

If you have multiple serial ports, set one explicitly:

```sh
cmake --preset avr-168 -DEMB_SERIAL_PORT=/dev/ttyUSB0
cmake --build --preset avr-168 --target upload
```

You can also set port by index instead of full path (`0`, `1`, ... based on sorted `/dev/ttyUSB*` + `/dev/ttyACM*`):

```sh
cmake --preset avr-168 -DEMB_SERIAL_PORT=0
cmake --build --preset avr-168 --target upload_monitor
```

Backward compatibility: `AVRDUDE_PORT` still works, but `EMB_SERIAL_PORT` is preferred.

To override programmer or upload baud:

```sh
cmake --preset avr-168 -DAVRDUDE_PROGRAMMER=arduino -DAVRDUDE_BAUD=57600
cmake --build --preset avr-168 --target upload
```

For STM32F103:

```sh
cmake --preset arm-stm32f103
cmake --build --preset arm-stm32f103
```

## Raspberry Pi Rust Target

A simple Rust application lives in `projects/raspi` and is exposed as the CMake target `raspi`.
By default, this preset cross-builds a static binary for Raspberry Pi 1 using `arm-unknown-linux-musleabihf`.

```sh
cmake --preset raspi
cmake --build --preset raspi
```

Built binary path:

`projects/raspi/target/arm-unknown-linux-musleabihf/release/raspi`

If you previously built the old glibc target (`arm-unknown-linux-gnueabihf`), clear stale artifacts before rebuilding:

```sh
rm -rf build-raspi projects/raspi/target/arm-unknown-linux-gnueabihf
cmake --preset raspi
cmake --build --preset raspi
```

If you see `file in wrong format` from `/usr/bin/ld`, ensure the musl target uses `rust-lld` and reconfigure:

```sh
rm -rf build-raspi
cmake --preset raspi
cmake --build --preset raspi
```

## Build: ARM Cortex-M

You must provide a linker script for ARM targets.

```sh
cmake -S . -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-gcc.cmake \
  -DARM_CPU=cortex-m3 \
  -DARM_MCU=stm32f103
cmake --build build-arm
```

The STM32F103 example uses the in-repo linker script and startup file. Adjust memory sizes in the linker script if your exact part differs.

For STM32F411, you will need a different linker script and startup file; see the Notes section.

## Arduino Core Integration (AVR)

AVR builds expect the Arduino core sources and include paths. You can provide the core path and (optionally) the variant path manually, or use the sync script below to auto-detect them.

```sh
cmake -S . -B build-avr \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/avr-gcc.cmake \
  -DARDUINO_CORE_PATH=/absolute/path/to/arduino/hardware/arduino/avr/cores/arduino \
  -DARDUINO_VARIANT_PATH=/absolute/path/to/arduino/hardware/arduino/avr/variants/standard
cmake --build build-avr
```

## Arduino Libraries via Arduino CLI

This project keeps CMake as the build tool, while Arduino CLI manages library downloads. After installing libraries with Arduino CLI, run the sync script to add include paths and compile sources.

1) Install libraries:

```sh
arduino-cli lib install <LibraryName>
```

2) List libraries to compile in `arduino-libs.txt` (one per line).

3) Sync into CMake + VS Code:

```sh
python3 tools/sync_arduino_libs.py
```

The script generates `cmake/arduino_libs.cmake` and updates `.vscode/c_cpp_properties.json` ("Arduino" configuration).
It also adds Arduino AVR core include paths (from `arduino:avr`) and writes `ARDUINO_CORE_PATH` / `ARDUINO_VARIANT_PATH` into `cmake/arduino_libs.cmake` if found in the Arduino CLI data directory.

## Helper Script: build.sh

A convenience script `build.sh` is provided to simplify common tasks. It wraps the CMake build, upload, and monitor commands for AVR firmware projects.

**Usage:**

```sh
./build.sh <target>
```
- Builds, uploads, and starts the serial monitor for the given target (e.g., `hamster`, `sunflower`).

```sh
./build.sh clean
```
- Cleans all build outputs (equivalent to `cmake --build <build-dir> --target clean`).

**Tab Completion:**
- Bash completion hints are available. Source `build.sh.completion` in your shell or add it to your `.bashrc` for autocompletion of targets and the `clean` command.

**Examples:**
```sh
./build.sh hamster      # Build, upload, and monitor hamster firmware
./build.sh sunflower   # Build, upload, and monitor sunflower firmware
./build.sh clean       # Clean all build outputs
```

If multiple USB serial devices are connected, select ports explicitly when using the helper script:

```sh
EMB_SERIAL_PORT=1 ./build.sh hamster avr-168
```

See the script for more details or to add new targets.

## Notes

- For ARM targets, you typically need a startup file and linker script for your specific MCU.
- The STM32F103 sample files live under `src/arm/stm32f103/` and are a starting point, not production-ready.
- For STM32F411, add a new `src/arm/stm32f411/` with its own startup file and linker script, then set `-DARM_MCU=stm32f411` and `-DLINKER_SCRIPT=...`.
- For AVR targets, the AVR-GCC toolchain provides the standard linker specs per MCU.
- Convert the output to HEX/BIN with `objcopy` if needed.
