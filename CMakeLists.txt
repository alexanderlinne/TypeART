cmake_minimum_required(VERSION 3.20)

project(typeart
  VERSION 1.7
  HOMEPAGE_URL https://github.com/tudasc/TypeART
  DESCRIPTION "LLVM-based type and memory allocation tracking sanitizer"
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # CMake 3.21 offers PROJECT_IS_TOP_LEVEL
  set(TYPEART_IS_TOP_LEVEL true)
else()
  set(TYPEART_IS_TOP_LEVEL false)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(
  APPEND
  CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(typeartToolchainOptions)

if(TYPEART_USE_ALLOCATOR)
  if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64")
    message(STATUS "Building for system architecture ${CMAKE_SYSTEM_PROCESSOR}")
    enable_language(ASM)
  else()
    message(FATAL_ERROR "The allocator instrumentation currently only supports x86_64! (Current processor is ${CMAKE_SYSTEM_PROCESSOR})")
  endif()
endif()

typeart_add_format_target(
  format-sources "Formats project source files"
  TARGETS lib/**/*.h
          lib/**/*.hpp
          lib/**/*.c
          lib/**/*.cpp
          "include/**/*.hpp"
          "include/**/*.h"
          demo/*.c
          demo/*.h
)

add_subdirectory(externals)
add_subdirectory(lib)
add_subdirectory(scripts)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion
)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(LIBRARY_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
set(BINARY_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
set(CMAKE_INSTALL_DIR ${TYPEART_INSTALL_CONFIGDIR})

configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/typeartConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${TYPEART_INSTALL_CONFIGDIR}
  PATH_VARS INCLUDE_INSTALL_DIR
            LIBRARY_INSTALL_DIR
            BINARY_INSTALL_DIR
            CMAKE_INSTALL_DIR
)

install(DIRECTORY include/
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${TYPEART_INSTALL_CONFIGDIR}
)

# export(PACKAGE ${PROJECT_NAME})

if(TYPEART_IS_TOP_LEVEL)
  enable_testing()
  add_subdirectory(test)
endif()

feature_summary(WHAT ENABLED_FEATURES PACKAGES_FOUND PACKAGES_NOT_FOUND
  DESCRIPTION "TypeART ${PROJECT_VERSION} package and feature info:"
  INCLUDE_QUIET_PACKAGES
  QUIET_ON_EMPTY
)
