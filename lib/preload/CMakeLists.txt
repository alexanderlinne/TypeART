set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(PRELOAD_LIB_SOURCES
    Lib.cpp
    Lib.S
)

add_library(${TYPEART_PREFIX}_Preload SHARED ${PRELOAD_LIB_SOURCES})
set_target_properties(
  ${TYPEART_PREFIX}_Preload
  PROPERTIES
  OUTPUT_NAME "${PROJECT_NAME}Preload"
  EXPORT_NAME "Preload"
)
add_library(typeart::Preload ALIAS ${TYPEART_PREFIX}_Preload)

target_link_libraries(
  ${TYPEART_PREFIX}_Preload
  ${TYPEART_PREFIX}_Runtime
  dl
)

target_include_directories(
  ${TYPEART_PREFIX}_Preload
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/passes/support>
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/typeart>
)

target_include_directories(${TYPEART_PREFIX}_Preload SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

target_compile_definitions(
  ${TYPEART_PREFIX}_Preload
  PRIVATE LOG_LEVEL=${LOG_LEVEL_RT}
)

typeart_target_compile_options(${TYPEART_PREFIX}_Preload)
typeart_target_define_file_basename(${TYPEART_PREFIX}_Preload)
typeart_target_coverage_options(${TYPEART_PREFIX}_Preload)

make_tidy_check(${TYPEART_PREFIX}_Preload "${RUNTIME_LIB_SOURCES}")

set(CONFIG_NAME ${PROJECT_NAME}Preload)
set(TARGETS_EXPORT_NAME ${CONFIG_NAME}Targets)

install(
  TARGETS ${TYPEART_PREFIX}_Preload
  EXPORT ${TARGETS_EXPORT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  EXPORT ${TARGETS_EXPORT_NAME}
  NAMESPACE typeart::
  DESTINATION ${TYPEART_INSTALL_CONFIGDIR}
)

export(
  EXPORT ${TARGETS_EXPORT_NAME}
  FILE ${CMAKE_BINARY_DIR}/${TARGETS_EXPORT_NAME}.cmake
  NAMESPACE typeart::
)
