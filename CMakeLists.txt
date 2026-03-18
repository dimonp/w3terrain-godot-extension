# Using the same minimum as the godot-cpp project
cmake_minimum_required(VERSION 3.17)

# Silence unused variable warning when specified from toolchain
if(CMAKE_C_COMPILER)
endif()

if(BUILD_TESTING)
  list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

if(WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set(LIBNAME "gdextension_w3terrain" CACHE STRING "The name of the library")
set(GODOT_PROJECT_DIR "demo" CACHE STRING "The directory of a Godot project folder")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Enable verbose output" FORCE)

option(gdextension_w3terrain_ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(gdextension_w3terrain_ENABLE_IPO "Enable IPO/LTO" ON)
option(gdextension_w3terrain_DEBUG_COUNTERS_ENABLE "Performance monitor counters" ON)
option(gdextension_w3terrain_ENABLE_AVX "Enable AVX instructions" ON)

# Now we can specify our own project which will inherit any global cmake properties or variables that have been defined.
project(gdextension_w3terrain
    VERSION 0.1
    DESCRIPTION ""
    HOMEPAGE_URL ""
    LANGUAGES CXX)

# Make sure all the dependencies are satisfied
find_package(Python3 3.4 REQUIRED)
find_program(GIT git REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

# Ensure godot-cpp submodule has been updated
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/godot-cpp/src")
    message(NOTICE "godot-cpp bindings source not found")
    message(NOTICE "initializing/updating the godot-cpp submodule...")

    # update the c++ bindings submodule to populate it with the necessary source for the library
    execute_process(
        COMMAND git submodule update --init godot-cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND_ERROR_IS_FATAL ANY)
endif()
set(GODOTCPP_TARGET "$<IF:$<CONFIG:Debug>,template_debug,template_release>")
add_subdirectory(godot-cpp SYSTEM)

# Add godot-cpp's module path and include the exported functions.
# This is made available for documentation generation
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${godot-cpp_SOURCE_DIR}/cmake")
include(GodotCPPModule)

set(LRUMM_BUILD_TESTING OFF)
set(LRUMM_ENABLE_SANITIZERS OFF)
set(LRUMM_ENABLE_EXCEPTIONS OFF)

# Ensure lru_memory_manager submodule has been updated
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/lru_memory_manager/src")
    message(NOTICE "lru_memory_manager bindings source not found")
    message(NOTICE "initializing/updating the lru_memory_manager submodule...")

    # update the c++ bindings submodule to populate it with the necessary source for the library
    execute_process(
        COMMAND git submodule update --init lru_memory_manager
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND_ERROR_IS_FATAL ANY)
endif()
add_subdirectory(lru_memory_manager SYSTEM)

set(QUADTREE_BUILD_TESTING OFF)
set(QUADTREE_ENABLE_SANITIZERS OFF)
set(QUADTREE_BUILD_MATH OFF)
set(QUADTREE_ENABLE_EXCEPTIONS OFF)

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/quadtree/src")
    message(NOTICE "quadtree bindings source not found")
    message(NOTICE "initializing/updating the quadtree submodule...")

    # update the c++ bindings submodule to populate it with the necessary source for the library
    execute_process(
        COMMAND git submodule update --init quadtree
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND_ERROR_IS_FATAL ANY)
endif()
add_subdirectory(quadtree SYSTEM)

# The godot-cpp target has some of useful properties attached that can be retrieved like so.
get_target_property(GODOTCPP_SUFFIX godot::cpp GODOTCPP_SUFFIX)
get_target_property(GODOTCPP_PLATFORM godot::cpp GODOTCPP_PLATFORM)

include(ProjectOptions.cmake)
gdextension_w3terrain_options()

add_library(${LIBNAME} SHARED)

target_compile_definitions(${LIBNAME} PRIVATE W3_PROJECT_BUILD_DLL)
if(gdextension_w3terrain_DEBUG_COUNTERS_ENABLE)
    target_compile_definitions(${LIBNAME} PRIVATE W3MAP_STATS_ENABLE)
endif()
target_link_libraries(${LIBNAME} PRIVATE ${LIBNAME}_options)

file(GLOB TILETERRAIN_SOURCE_FILES "src/w3terrain/*.cpp")
file(GLOB TILETERRAIN_HEADERS_FILES "src/w3terrain/*.h")
target_sources(${LIBNAME}
    PRIVATE
        src/register_types.cpp
        ${TILETERRAIN_SOURCE_FILES}

    PRIVATE FILE_SET HEADERS
        FILES
            src/register_types.h
            ${TILETERRAIN_HEADERS_FILES})

target_include_directories(${LIBNAME} PUBLIC src)

# Fetch a list of the xml files to use for documentation and add to our target
file(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/doc_classes/*.xml")

# conditionally add doc data to compile output
if(DOC_XML)
    if(GODOTCPP_TARGET MATCHES "editor|template_debug")
        target_doc_sources(${LIBNAME} ${DOC_XML})
    endif()
endif()

target_link_libraries(${LIBNAME} PRIVATE godot::cpp)
target_link_libraries(${LIBNAME} PRIVATE lrumm::lru_memory_manager)
target_link_libraries(${LIBNAME} PRIVATE quadtree::quadtree)
target_link_libraries(${LIBNAME} PRIVATE Microsoft.GSL::GSL)

set_property(TARGET ${LIBNAME} PROPERTY CXX_STANDARD 23)
set_target_properties(${LIBNAME}
    PROPERTIES
    # The generator expression here prevents msvc from adding a Debug or Release subdir.
    RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/bin/${GODOTCPP_PLATFORM}>"
    PREFIX ""
    OUTPUT_NAME "${LIBNAME}${GODOTCPP_SUFFIX}")

set(GODOT_PROJECT_BINARY_DIR "${PROJECT_SOURCE_DIR}/${GODOT_PROJECT_DIR}/bin/${GODOTCPP_PLATFORM}")

add_custom_command(TARGET ${LIBNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${LIBNAME}>" "${GODOT_PROJECT_BINARY_DIR}/$<TARGET_FILE_NAME:${LIBNAME}>")

if(BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()
