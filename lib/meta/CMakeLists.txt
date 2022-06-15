set(META_LIB_SOURCES Database.cpp LLVMMetadataConverter.cpp Meta.cpp YAML.cpp)

add_library(${TYPEART_PREFIX}_MetaObj OBJECT ${META_LIB_SOURCES})

set_target_properties(${TYPEART_PREFIX}_MetaObj PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(${TYPEART_PREFIX}_MetaObj SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

target_precompile_headers(${TYPEART_PREFIX}_MetaObj
  PUBLIC ${CMAKE_SOURCE_DIR}/include/typeart/meta/Meta.hpp
)

target_include_directories(
  ${TYPEART_PREFIX}_MetaObj
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/typeart>
          ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${TYPEART_PREFIX}_MetaObj
  PRIVATE typeart::SystemStatic
          fmt::fmt
)

target_compile_definitions(${TYPEART_PREFIX}_MetaObj PRIVATE TYPEART_LOG_LEVEL=${TYPEART_LOG_LEVEL})

typeart_target_compile_options(${TYPEART_PREFIX}_MetaObj)

typeart_target_coverage_options(${TYPEART_PREFIX}_MetaObj)

make_tidy_check(${TYPEART_PREFIX}_Meta "${META_LIB_SOURCES}")

add_library(${TYPEART_PREFIX}_Meta SHARED $<TARGET_OBJECTS:${TYPEART_PREFIX}_MetaObj>)
add_library(typeart::Meta ALIAS ${TYPEART_PREFIX}_Meta)
set_target_properties(
  ${TYPEART_PREFIX}_Meta
  PROPERTIES
  OUTPUT_NAME "${TYPEART_PREFIX}Meta"
  EXPORT_NAME "Meta"
)
target_include_directories(
  ${TYPEART_PREFIX}_Meta
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/typeart>
)

target_link_libraries(${TYPEART_PREFIX}_Meta PRIVATE fmt::fmt typeart::SystemStatic LLVMCore LLVMSupport)

typeart_target_coverage_options(${TYPEART_PREFIX}_Meta)

add_library(${TYPEART_PREFIX}_MetaStatic STATIC $<TARGET_OBJECTS:${TYPEART_PREFIX}_MetaObj>)
add_library(typeart::MetaStatic ALIAS ${TYPEART_PREFIX}_MetaStatic)
set_target_properties(
  ${TYPEART_PREFIX}_MetaStatic
  PROPERTIES
  OUTPUT_NAME "${TYPEART_PREFIX}MetaStatic"
  EXPORT_NAME "MetaStatic"
)
target_include_directories(
  ${TYPEART_PREFIX}_MetaStatic
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
)

target_link_libraries(${TYPEART_PREFIX}_MetaStatic PRIVATE typeart::SystemStatic)

typeart_target_coverage_options(${TYPEART_PREFIX}_MetaStatic)

set(CONFIG_NAME ${PROJECT_NAME}Meta)
set(TARGETS_EXPORT_NAME ${CONFIG_NAME}Targets)

install(
  TARGETS ${TYPEART_PREFIX}_Meta ${TYPEART_PREFIX}_MetaStatic
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
