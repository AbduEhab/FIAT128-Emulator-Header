# Overview

This is the base base library project for the Fiat128 Emulator. It contains the basic functionality for the emulator. It is a header only library that can be compiled as a static library. It is also a submodule of the [Fiat128 Emulator](

## Requirements (for compiling)
- [CMake](https://cmake.org/)
- x86 or x64 compiler (only tested with `MSVC`, `MinGW` on windows and `GCC` for UNIX)

## Profiling

You can use https://ui.perfetto.dev/ to view the profiling data. The data is stored in `profiling/` and is named `perfetto_trace.json`.
