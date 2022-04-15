add_subdirectory(typegen)
add_subdirectory(filter)
add_subdirectory(analysis)

set(PASS_SOURCES
  CommandLine.cpp
  TypeARTPass.cpp
  support/TypeUtil.cpp
  instrumentation/allocator/ArgumentParser.cpp
  instrumentation/allocator/InstrumentationStrategy.cpp
  instrumentation/common/InstrumentationHelper.cpp
  instrumentation/common/TypeARTFunctions.cpp
  instrumentation/tracker/ArgumentParser.cpp
  instrumentation/tracker/InstrumentationStrategy.cpp
  instrumentation/TypeARTInstrumentation.cpp
)

typeart_make_llvm_module(
  ${TYPEART_PREFIX}_TransformPass "${PASS_SOURCES}"
  LINK_LIBS typeart::MemInstFinder
            typeart::DatabaseStatic
            typeart::TypeGenStatic
  INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/lib/
               ${PROJECT_SOURCE_DIR}/lib/passes/
               ${PROJECT_SOURCE_DIR}/include/typeart
)
mark_as_advanced(LLVM_TYPEART_TRANSFORMPASS_LINK_INTO_TOOLS)

set_target_properties(
  ${TYPEART_PREFIX}_TransformPass
  PROPERTIES
  OUTPUT_NAME "${PROJECT_NAME}TransformPass"
  EXPORT_NAME "TransformPass"
)
add_library(typeart::TransformPass ALIAS ${TYPEART_PREFIX}_TransformPass)

target_link_libraries(
  ${TYPEART_PREFIX}_TransformPass
  PRIVATE fmt::fmt
)

target_compile_definitions(
  ${TYPEART_PREFIX}_TransformPass PRIVATE TYPEART_LOG_LEVEL=${TYPEART_LOG_LEVEL}
                        $<$<BOOL:${TYPEART_USE_ALLOCATOR}>:TYPEART_USE_ALLOCATOR>
                        $<$<BOOL:${TYPEART_SHOW_STATS}>:LLVM_FORCE_ENABLE_STATS=1>
)

typeart_target_compile_options(${TYPEART_PREFIX}_TransformPass)
typeart_target_coverage_options(${TYPEART_PREFIX}_TransformPass)

set(CONFIG_NAME ${PROJECT_NAME}TransformPass)
set(TARGETS_EXPORT_NAME ${CONFIG_NAME}Targets)

install(
  TARGETS ${TYPEART_PREFIX}_TransformPass
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

#configure_package_config_file(
#  ${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in
#  ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}Config.cmake
#  INSTALL_DESTINATION ${TYPEART_INSTALL_CONFIGDIR}
#)
#
#install(FILES
#  ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}Config.cmake
#  DESTINATION ${TYPEART_INSTALL_CONFIGDIR}
#)
