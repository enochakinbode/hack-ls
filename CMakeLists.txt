cmake_minimum_required(VERSION 3.16)

# ✅ Generate compile_commands.json (clangd / Zed use this)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# Default to Debug builds unless explicitly set
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

project(hack-ls LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# nlohmann/json submodule
add_subdirectory(external/nlohmann_json)

# Collect all source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_executable(hack-ls ${SOURCES})

# Add include directories for headers
target_include_directories(hack-ls PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# HackAssembler (C) frontend as a static library
set(HACKASM_FRONTEND_DIR ${CMAKE_SOURCE_DIR}/external/HackAssembler/frontend)
file(GLOB_RECURSE HACKASM_FRONTEND_SOURCES
    ${HACKASM_FRONTEND_DIR}/core/*.c
    ${HACKASM_FRONTEND_DIR}/helpers/*.c
    ${HACKASM_FRONTEND_DIR}/structures/*.c
    ${HACKASM_FRONTEND_DIR}/utils/*.c
)

add_library(hackassembler_frontend STATIC ${HACKASM_FRONTEND_SOURCES})
set_target_properties(hackassembler_frontend PROPERTIES C_STANDARD 11)
target_include_directories(hackassembler_frontend PUBLIC
    ${HACKASM_FRONTEND_DIR}/include
)

target_link_libraries(hack-ls PRIVATE hackassembler_frontend)

# Link against nlohmann/json
target_link_libraries(hack-ls PRIVATE nlohmann_json::nlohmann_json)

# Common warnings
target_compile_options(hack-ls PRIVATE -Wall -Wextra -Wpedantic -Weffc++)

# Debug vs Release flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_options(hack-ls PRIVATE -g -O0)
  message(STATUS "Building in Debug mode with debug symbols (-g -O0)")
else()
  target_compile_options(hack-ls PRIVATE -O2)
endif()
