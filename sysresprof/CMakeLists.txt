cmake_minimum_required(VERSION 3.21)
project(sysresprof CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(boost_nowide CONFIG REQUIRED)

add_library(sysresprof_core STATIC
    src/lib/impl/win32/fs.cpp
    src/lib/impl/win32/global.cpp
    src/lib/impl/win32/hw.cpp
    src/lib/impl/win32/literals.cpp
    src/lib/impl/win32/proc.cpp
    src/lib/impl/win32/reg.cpp
    src/lib/impl/win32/uhandle.cpp

    src/lib/impl/prof/gpu.cpp
    src/lib/impl/prof/sampler.cpp
)

target_include_directories(sysresprof_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib
)

target_compile_definitions(sysresprof_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(sysresprof_core PRIVATE -Wall -Wextra)
target_link_libraries(sysresprof_core PUBLIC Boost::nowide pdh dxgi advapi32)

add_executable(sysresprof src/main.cpp)
target_compile_options(sysresprof PRIVATE -Wall -Wextra)
target_link_libraries(sysresprof PRIVATE sysresprof_core)
target_link_options(sysresprof PRIVATE -static)