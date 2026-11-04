# Building

### Summary

 - C++20
 - CMake >= 3.24.0
 - Targets:
	 - Windows 10+
	 - Linux

### Dependencies
- Qt6
- [Qx](https://github.com/oblivioncth/Qx/)
- [libfp](https://github.com/oblivioncth/libfp/)
- [QI-QMP](https://github.com/oblivioncth/QI-QMP/)
- [QuaZip](https://github.com/stachenov/quazip)
- [Neargye's Magic Enum](https://github.com/Neargye/magic_enum)
- [OBCMake](https://github.com/oblivioncth/OBCmake)

## Details
The source for this project is managed by a sensible CMake configuration that allows for straightforward compilation and consumption of its target(s), either as a sub-project or as an imported package. All required dependencies except for Qt6 are automatically acquired via CMake's FetchContent mechanism.

### Building
Ensure Qt6 is installed and locatable by CMake (or alternatively use the `qt-cmake` script that comes with Qt in-place of the`cmake` command).

Should work with MSVC, MINGW64, clang, and gcc.

```
# Acquire source
git clone https://github.com/oblivioncth/CLIFp

# Configure (ninja optional, but recommended)
cmake -S CLIFp -B build-CLIFp -G "Ninja Multi-config"

# Build
cmake --build build-CLIFp

# Install
cmake --install build-CLIFp

# Run
cd "build-CLIFp/out/install/bin"
clifp
```