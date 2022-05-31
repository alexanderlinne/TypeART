set(DATABASE_LIB_SOURCES Database.cpp LLVMMetadataConverter.cpp Types.cpp YAML.cpp)

add_library(${TYPEART_PREFIX}_DatabaseObj OBJECT ${DATABASE_LIB_SOURCES})

set_target_properties(${TYPEART_PREFIX}_DatabaseObj PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(${TYPEART_PREFIX}_DatabaseObj SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

target_precompile_headers(${TYPEART_PREFIX}_DatabaseObj
  PUBLIC ${CMAKE_SOURCE_DIR}/include/typeart/db/Types.hpp
)

target_include_directories(
  ${TYPEART_PREFIX}_DatabaseObj
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/typeart>
          ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${TYPEART_PREFIX}_DatabaseObj
  PRIVATE typeart::SystemStatic
          fmt::fmt
)

target_compile_definitions(${TYPEART_PREFIX}_DatabaseObj PRIVATE TYPEART_LOG_LEVEL=${TYPEART_LOG_LEVEL})

typeart_target_compile_options(${TYPEART_PREFIX}_DatabaseObj)

typeart_target_coverage_options(${TYPEART_PREFIX}_DatabaseObj)

make_tidy_check(${TYPEART_PREFIX}_Database "${DATABASE_LIB_SOURCES}")

add_library(${TYPEART_PREFIX}_Database SHARED $<TARGET_OBJECTS:${TYPEART_PREFIX}_DatabaseObj>)
add_library(typeart::Database ALIAS ${TYPEART_PREFIX}_Database)
set_target_properties(
  ${TYPEART_PREFIX}_Database
  PROPERTIES
  OUTPUT_NAME "${TYPEART_PREFIX}Database"
  EXPORT_NAME "Database"
)
target_include_directories(
  ${TYPEART_PREFIX}_Database
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/typeart>
)

target_link_libraries(${TYPEART_PREFIX}_Database PRIVATE fmt::fmt typeart::SystemStatic LLVMCore LLVMSupport)

typeart_target_coverage_options(${TYPEART_PREFIX}_Database)

add_library(${TYPEART_PREFIX}_DatabaseStatic STATIC $<TARGET_OBJECTS:${TYPEART_PREFIX}_DatabaseObj>)
add_library(typeart::DatabaseStatic ALIAS ${TYPEART_PREFIX}_DatabaseStatic)
set_target_properties(
  ${TYPEART_PREFIX}_DatabaseStatic
  PROPERTIES
  OUTPUT_NAME "${TYPEART_PREFIX}DatabaseStatic"
  EXPORT_NAME "DatabaseStatic"
)
target_include_directories(
  ${TYPEART_PREFIX}_DatabaseStatic
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
)

target_link_libraries(${TYPEART_PREFIX}_DatabaseStatic PRIVATE typeart::SystemStatic)

typeart_target_coverage_options(${TYPEART_PREFIX}_DatabaseStatic)

set(CONFIG_NAME ${PROJECT_NAME}Database)
set(TARGETS_EXPORT_NAME ${CONFIG_NAME}Targets)

install(
  TARGETS ${TYPEART_PREFIX}_Database ${TYPEART_PREFIX}_DatabaseStatic
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
