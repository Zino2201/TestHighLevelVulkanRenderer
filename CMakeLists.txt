cmake_minimum_required(VERSION 3.20)

project(CityBuilder CXX)

# I AM SPEED
set(CMAKE_UNITY_BUILD ON)

# Remove exceptions (enabled by default by CMake)
string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CB_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CB_BINS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CB_MODULE_PREFIX "cb-")

# Options
option(CB_WITH_VULKAN "Build with Vulkan support (requires Vulkan SDK)" ON)
option(CB_MONOLITHIC "Monolithic mode (statc libs)" OFF)

message(STATUS "With Vulkan: ${CB_WITH_VULKAN}")

function(cb_add_module TARGET)
    if(CB_MONOLITHIC)
        add_library(${TARGET} STATIC ${ARGN})
    else()
        add_library(${TARGET} SHARED ${ARGN})
    endif()

    set_target_properties(${TARGET} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
        LIBRARY_OUTPUT_DIRECTORY "${CB_BINS_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${CB_BINS_DIR}"
        PREFIX "${CB_MODULE_PREFIX}")
endfunction()

add_subdirectory(src)
