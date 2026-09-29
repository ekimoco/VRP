cmake_minimum_required(VERSION 3.21)
project(resprof CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(boost_nowide CONFIG REQUIRED)

add_library(resprof_core STATIC
    src/lib/impl/gpu.cpp
    src/lib/impl/process.cpp
    src/lib/impl/win32.cpp
    src/lib/impl/winfs.cpp
)

target_include_directories(resprof_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/src        # only if lib code includes constants.hpp
)

target_compile_definitions(resprof_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(resprof_core PRIVATE -Wall -Wextra)
target_link_libraries(resprof_core PUBLIC Boost::nowide pdh dxgi advapi32)

add_executable(resprof src/main.cpp)
target_compile_options(resprof PRIVATE -Wall -Wextra)
target_link_libraries(resprof PRIVATE resprof_core)
target_link_options(resprof PRIVATE -static)