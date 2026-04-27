# Perfetto-lua
Perfetto SDK for Lua


# Build
Unpack latest Perfetto Release `perfetto-cpp-sdk-src.zip` into `sdk`

SDK must be 50.1 or below, or above 54.0 see this issue
https://github.com/google/perfetto/issues/5591

```
mkdir build
cd build
cmake -S .. -B .
cmake --build . --config Release
```