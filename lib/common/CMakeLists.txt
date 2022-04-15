set(COMMON_SOURCES Types.cpp)

add_library(${TYPEART_PREFIX}_Common OBJECT ${COMMON_SOURCES})

set_target_properties(${TYPEART_PREFIX}_Common PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(
  ${TYPEART_PREFIX}_Common
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/typeart>
)

target_compile_definitions(${TYPEART_PREFIX}_Common PRIVATE TYPEART_LOG_LEVEL=${TYPEART_LOG_LEVEL})

typeart_target_compile_options(${TYPEART_PREFIX}_Common)

typeart_target_coverage_options(${TYPEART_PREFIX}_Common)

make_tidy_check(${TYPEART_PREFIX}_Common "${COMMON_SOURCES}")

add_library(typeart::Common ALIAS ${TYPEART_PREFIX}_Common)
