set(LIB_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/mpi_interceptor_rt.c)

add_custom_command(
  OUTPUT ${LIB_SOURCE}
  COMMAND ${Python3_EXECUTABLE} wrap.py mpi_interceptor_tmpl.impl -o
          ${LIB_SOURCE}
  MAIN_DEPENDENCY mpi_interceptor_tmpl.impl
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generate PMPI wrapper"
)

add_library(${TYPEART_PREFIX}_MPITool SHARED
  ${LIB_SOURCE}
  Config.cpp
  InterceptorFunctions.cpp
  Logger.cpp
  TypeCheck.cpp
  Util.cpp
)
set_property(TARGET ${TYPEART_PREFIX}_MPITool PROPERTY OUTPUT_NAME "${PROJECT_NAME}MPITool")
add_library(typeart::MPITool ALIAS ${TYPEART_PREFIX}_MPITool)

set_property(TARGET ${TYPEART_PREFIX}_MPITool PROPERTY C_STANDARD 11)
set_property(TARGET ${TYPEART_PREFIX}_MPITool PROPERTY C_STANDARD_REQUIRED TRUE)

typeart_target_define_file_basename(${TYPEART_PREFIX}_MPITool)

target_include_directories(${TYPEART_PREFIX}_MPITool SYSTEM
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(${TYPEART_PREFIX}_MPITool
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_include_directories(${TYPEART_PREFIX}_MPITool
  SYSTEM
  PRIVATE ${LLVM_INCLUDE_DIRS}
)

if(TYPEART_TSAN)
  typeart_target_tsan_options(${TYPEART_PREFIX}_MPITool)
endif()

target_link_libraries(${TYPEART_PREFIX}_MPITool
  PRIVATE typeart::Runtime
          typeart::SystemStatic
          MPI::MPI_CXX
          result::result
          fmt::fmt
          spdlog::spdlog
)

target_compile_definitions(
  ${TYPEART_PREFIX}_MPITool
  PRIVATE TYPEART_LOG_LEVEL=${TYPEART_LOG_LEVEL_RT}
)

set(CONFIG_NAME ${PROJECT_NAME}${TYPEART_PREFIX}_MPITool)
set(TARGETS_EXPORT_NAME ${CONFIG_NAME}Targets)

install(
  TARGETS ${TYPEART_PREFIX}_MPITool
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
