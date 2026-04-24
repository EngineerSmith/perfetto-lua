# referenced https://perfetto.dev/docs/instrumentation/tracing-sdk#:~:text=CMakeLists%2Etxt
cmake_minimum_required(VERSION 3.13)
project(perfetto-lua LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(perfetto_sdk STATIC
    sdk/perfetto.cc
)

target_include_directories(perfetto_sdk PUBLIC sdk)

if(MSVC)
    target_compile_options(perfetto_sdk PRIVATE "/bigobj")
    target_compile_definitions(perfetto_sdk PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(perfetto_sdk INTERFACE ws2_32)
endif()

add_library(perfetto-lua SHARED
    perfetto_lua.cpp
    perfetto_categories.cpp
)

if (MSVC)
    target_compile_options(perfetto-lua PRIVATE "/bigobj" "/permissive-")
endif()

target_link_libraries(perfetto-lua PRIVATE
    perfetto_sdk
    Threads::Threads
)