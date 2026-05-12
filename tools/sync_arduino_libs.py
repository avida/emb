#!/usr/bin/env python3
import json
import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIB_LIST = ROOT / "arduino-libs.txt"
CMAKE_OUT = ROOT / "cmake" / "arduino_libs.cmake"
VSCODE_DIR = ROOT / ".vscode"
VSCODE_CPP = VSCODE_DIR / "c_cpp_properties.json"
DUMMY_SRC = ROOT / "cmake" / "arduino_libs_dummy.c"

KNOWN_DEPENDENCIES = {
    "rf24": ["SPI"],
}


def run(cmd):
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"Command failed: {' '.join(cmd)}\n{result.stderr.strip()}")
    return result.stdout


def get_cli_data_dir():
    env_override = os.environ.get("ARDUINO_CLI_DATA_DIR", "").strip()
    if env_override:
        return env_override
    out = run(["arduino-cli", "config", "get", "directories.data"]).strip()
    if not out:
        raise RuntimeError("arduino-cli did not return directories.data")
    return out


def read_requested_libs():
    if not LIB_LIST.exists():
        return []
    libs = []
    for line in LIB_LIST.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        libs.append(line)
    return libs


def expand_known_dependencies(requested):
    expanded = list(requested)
    present = {name.lower() for name in expanded}

    for lib_name in requested:
        deps = KNOWN_DEPENDENCIES.get(lib_name.lower(), [])
        for dep in deps:
            if dep.lower() in present:
                continue
            expanded.append(dep)
            present.add(dep.lower())
            print(f"Added dependency: {lib_name} -> {dep}")

    return expanded


def load_installed_libs():
    out = run(["arduino-cli", "lib", "list", "--format", "json"])
    data = json.loads(out)
    libs = {}
    for item in data.get("libraries", []):
        name = item.get("name")
        path = item.get("path") or item.get("location") or item.get("install_dir")
        if name and path:
            libs[name.lower()] = Path(path)

    # Also search in ~/Arduino/libraries
    user_lib_dir = Path.home() / "Arduino" / "libraries"
    if user_lib_dir.exists():
        for lib_folder in user_lib_dir.iterdir():
            if lib_folder.is_dir():
                # Try to get library name from folder
                lib_name = lib_folder.name.lower()
                if lib_name not in libs:
                    libs[lib_name] = lib_folder
    # Also search for default libraries in packages/arduino/hardware/avr/<version>/libraries
    data_dir = get_cli_data_dir()
    avr_root = Path(data_dir) / "packages" / "arduino" / "hardware" / "avr"
    if avr_root.exists():
        versions = [p for p in avr_root.iterdir() if p.is_dir()]
        if versions:
            versions.sort()
            version_dir = versions[-1]
            default_libs_dir = version_dir / "libraries"
            if default_libs_dir.exists():
                for lib_folder in default_libs_dir.iterdir():
                    if lib_folder.is_dir():
                        lib_name = lib_folder.name.lower()
                        if lib_name not in libs:
                            libs[lib_name] = lib_folder
    return libs


def collect_sources(lib_path):
    lib_name = lib_path.name.lower()

    if lib_name == "rf24":
        # For Arduino builds, RF24.cpp uses SPI.h and should not compile Linux/XMega drivers.
        rf24_sources = [lib_path / "RF24.cpp"]
        return [p for p in rf24_sources if p.exists()]

    patterns = ["*.c", "*.cpp", "*.s", "*.S"]
    sources = []
    seen = set()

    # Keep examples/tests out of firmware builds.
    excluded_prefixes = ("example", "test", "doc", "extra", "benchmark", "py")

    def should_skip(path):
        return any(part.lower().startswith(excluded_prefixes) for part in path.parts)

    for base in [lib_path / "src", lib_path / "utility", lib_path]:
        if not base.exists():
            continue
        for pattern in patterns:
            for src in base.rglob(pattern):
                if should_skip(src.relative_to(lib_path)):
                    continue
                src_key = src.as_posix()
                if src_key in seen:
                    continue
                seen.add(src_key)
                sources.append(src)
    return sources


def collect_includes(lib_path):
    includes = [lib_path]
    if (lib_path / "src").exists():
        includes.append(lib_path / "src")
    if (lib_path / "utility").exists():
        includes.append(lib_path / "utility")
    return includes


def find_library_cmake(lib_path):
    cmake_file = lib_path / "CMakeLists.txt"
    if cmake_file.exists():
        return cmake_file
    return None


def is_library_cmake_compatible(cmake_file):
    force_cmake = os.environ.get("ARDUINO_LIBS_FORCE_CMAKE", "").strip().lower()
    if force_cmake in {"1", "true", "yes", "on"}:
        return True

    content = cmake_file.read_text(errors="ignore").lower()
    incompatible_tokens = [
        "-pthread",
        "spidev",
        "wiringpi",
        "pigpio",
        "mraa",
        "littlewire",
    ]
    return not any(token in content for token in incompatible_tokens)


def find_arduino_avr_core(data_dir):
    avr_root = Path(data_dir) / "packages" / "arduino" / "hardware" / "avr"
    if not avr_root.exists():
        return None, None

    versions = [p for p in avr_root.iterdir() if p.is_dir()]
    if not versions:
        return None, None

    versions.sort()
    version_dir = versions[-1]

    core_dir = version_dir / "cores" / "arduino"
    variant_dir = version_dir / "variants" / "standard"
    if not core_dir.exists():
        return None, None
    if not variant_dir.exists():
        variant_dir = None
    return core_dir, variant_dir


def write_cmake(includes, sources, core_dir, variant_dir, cmake_libs):
    lines = []
    lines.append("# This file is generated by tools/sync_arduino_libs.py")
    if core_dir:
        lines.append("if(NOT DEFINED ARDUINO_CORE_PATH)")
        lines.append(f"  set(ARDUINO_CORE_PATH \"{core_dir.as_posix()}\" CACHE PATH \"Arduino core path\" FORCE)")
        lines.append("endif()")
    if variant_dir:
        lines.append("if(NOT DEFINED ARDUINO_VARIANT_PATH)")
        lines.append(f"  set(ARDUINO_VARIANT_PATH \"{variant_dir.as_posix()}\" CACHE PATH \"Arduino variant path\" FORCE)")
        lines.append("endif()")
    lines.append("add_library(arduino_libs STATIC)")

    if not sources:
        DUMMY_SRC.write_text("/* Generated by tools/sync_arduino_libs.py */\nvoid emb_arduino_libs_dummy(void) {}\n")
        sources = [DUMMY_SRC]

    if sources:
        lines.append("target_sources(arduino_libs PRIVATE")
        for src in sources:
            lines.append(f"  {src.as_posix()}")
        lines.append(")")
    if includes:
        lines.append("target_include_directories(arduino_libs PUBLIC")
        for inc in includes:
            lines.append(f"  {inc.as_posix()}")
        lines.append(")")

    if cmake_libs:
        lines.append("")
        lines.append("# Prefer library-provided CMake targets when available.")
        lines.append("set(BUILD_TESTING OFF CACHE BOOL \"Disable tests from imported Arduino libs\" FORCE)")
        lines.append("set(ENABLE_TESTING OFF CACHE BOOL \"Disable tests from imported Arduino libs\" FORCE)")
        lines.append("set(BUILD_EXAMPLES OFF CACHE BOOL \"Disable examples from imported Arduino libs\" FORCE)")
        lines.append("set(ENABLE_FUZZING OFF CACHE BOOL \"Disable fuzz builds from imported Arduino libs\" FORCE)")
        for lib_name, lib_path, fallback_sources in cmake_libs:
            safe_name = re.sub(r"[^A-Za-z0-9_]", "_", lib_name.lower())
            lines.append("")
            lines.append(f"# Imported from: {lib_name}")
            lines.append(f"add_subdirectory({lib_path.as_posix()} ${{CMAKE_BINARY_DIR}}/arduino_libs/{safe_name} EXCLUDE_FROM_ALL)")
            lines.append(f"get_property(_emb_subdir_targets DIRECTORY {lib_path.as_posix()} PROPERTY BUILDSYSTEM_TARGETS)")
            lines.append("set(_emb_new_lib_targets)")
            lines.append("foreach(_emb_t ${_emb_subdir_targets})")
            lines.append("  if(NOT _emb_t STREQUAL \"arduino_libs\")")
            lines.append("    get_target_property(_emb_t_type ${_emb_t} TYPE)")
            lines.append("    if(_emb_t_type STREQUAL \"STATIC_LIBRARY\" OR _emb_t_type STREQUAL \"SHARED_LIBRARY\" OR _emb_t_type STREQUAL \"INTERFACE_LIBRARY\")")
            lines.append("      list(APPEND _emb_new_lib_targets ${_emb_t})")
            lines.append("    endif()")
            lines.append("  endif()")
            lines.append("endforeach()")
            lines.append("if(_emb_new_lib_targets)")
            lines.append("  target_link_libraries(arduino_libs PUBLIC ${_emb_new_lib_targets})")
            lines.append(f"  message(STATUS \"Linked CMake targets from {lib_name}: ${{_emb_new_lib_targets}}\")")
            lines.append("else()")
            if fallback_sources:
                lines.append(f"  message(WARNING \"No library targets detected from {lib_name} CMakeLists.txt, using source fallback\")")
                lines.append("  target_sources(arduino_libs PRIVATE")
                for src in fallback_sources:
                    lines.append(f"    {src.as_posix()}")
                lines.append("  )")
            else:
                lines.append(f"  message(WARNING \"No library targets or fallback sources for {lib_name}\")")
            lines.append("endif()")
    lines.append("")
    CMAKE_OUT.write_text("\n".join(lines))


def update_vscode(includes):
    VSCODE_DIR.mkdir(parents=True, exist_ok=True)
    if VSCODE_CPP.exists():
        data = json.loads(VSCODE_CPP.read_text())
    else:
        data = {
            "version": 4,
            "configurations": [
                {
                    "name": "Arduino",
                    "includePath": [],
                    "defines": [],
                    "compilerPath": "",
                    "cStandard": "c11",
                    "cppStandard": "c++17",
                    "intelliSenseMode": "gcc-x64"
                }
            ]
        }

    configs = data.get("configurations", [])
    if not configs:
        configs = [{"name": "Arduino", "includePath": []}]
        data["configurations"] = configs

    cfg = None
    for entry in configs:
        if entry.get("name") == "Arduino":
            cfg = entry
            break
    if cfg is None:
        cfg = {"name": "Arduino", "includePath": []}
        configs.append(cfg)

    include_set = {p for p in cfg.get("includePath", [])}
    for inc in includes:
        include_set.add(inc.as_posix())
    cfg["includePath"] = sorted(include_set)

    VSCODE_CPP.write_text(json.dumps(data, indent=2))


def main():
    data_dir = get_cli_data_dir()
    print(f"Arduino CLI data dir: {data_dir}")

    requested = read_requested_libs()
    if not requested:
        print("No libraries listed in arduino-libs.txt. Will sync core include paths only.")
    else:
        requested = expand_known_dependencies(requested)

    installed = load_installed_libs()
    includes = []
    sources = []
    cmake_libs = []
    missing = []

    core_dir, variant_dir = find_arduino_avr_core(data_dir)
    if core_dir:
        includes.append(core_dir)
        if variant_dir:
            includes.append(variant_dir)
        print("Found Arduino AVR core include paths:")
        print(f"  - {core_dir}")
        if variant_dir:
            print(f"  - {variant_dir}")
    else:
        print("Arduino AVR core not found under Arduino CLI data directory.")

    for name in requested:
        lib_path = installed.get(name.lower())
        if not lib_path:
            missing.append(name)
            continue
        includes.extend(collect_includes(lib_path))
        cmake_file = find_library_cmake(lib_path)
        if cmake_file and is_library_cmake_compatible(cmake_file):
            fallback_sources = collect_sources(lib_path)
            cmake_libs.append((name, lib_path, fallback_sources))
            print(f"Using CMakeLists for {name}: {cmake_file}")
        else:
            if cmake_file:
                print(f"Skipping CMakeLists for {name} (looks host-specific): {cmake_file}")
            sources.extend(collect_sources(lib_path))

    includes = list(dict.fromkeys(includes))
    sources = list(dict.fromkeys(sources))

    write_cmake(includes, sources, core_dir, variant_dir, cmake_libs)
    update_vscode(includes)

    if missing:
        print("Missing libraries:")
        for name in missing:
            print(f"  - {name}")

    print(f"Wrote {CMAKE_OUT}")
    print(f"Updated {VSCODE_CPP}")


if __name__ == "__main__":
    main()
