set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE STRING "" FORCE)
include(FetchContent)

mark_as_advanced(
  FETCHCONTENT_BASE_DIR
  FETCHCONTENT_QUIET
  FETCHCONTENT_FULLY_DISCONNECTED
  FETCHCONTENT_UPDATES_DISCONNECTED
)

if(TYPEART_MPI_INTERCEPT_LIB OR TYPEART_ABSEIL)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(TYPEART_MPI_INTERCEPT_LIB)
  add_subdirectory(result)
  add_subdirectory(fmt)
  add_subdirectory(spdlog)
endif()

if(TYPEART_PHMAP)
  #set(CMAKE_POLICY_DEFAULT_CMP0076 NEW)
  add_subdirectory(phmap)
endif()

if(TYPEART_ABSEIL)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  add_subdirectory(abseil)
endif()

if (TYPEART_SAFEPTR)
  add_subdirectory(safe_ptr)
endif()
