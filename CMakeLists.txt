cmake_minimum_required(VERSION 3.30)
project(UaSwissArmyKnife LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(STANDALONE_BUILD "Legacy compatibility option (no-op)" ON)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

execute_process(
  COMMAND git describe --tags --always
  OUTPUT_VARIABLE git_describe_output
  RESULT_VARIABLE git_describe_result
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT git_describe_result EQUAL 0 OR git_describe_output STREQUAL "")
  set(git_describe_output "0.0.0")
endif()
message(STATUS "git describe returned ${git_describe_output}")
file(WRITE "${PROJECT_BINARY_DIR}/uasak_version.h" "#define UASAK_VERSION \"${git_describe_output}\"")

set(CPACK_PACKAGE_NAME "UaSwissArmyKnife")
set(CPACK_PACKAGE_VENDOR "CERN-QUASAR-TEAM")
if(WIN32)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/")
else()
  set(CPACK_GENERATOR "RPM")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/UaSwissArmyKnife")
  set(CPACK_RPM_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
endif()

string(REGEX MATCH "([0-9]+)\\.([0-9]+)(\\.([0-9]+))?" _version_match "${git_describe_output}")
if(_version_match)
  set(CPACK_PACKAGE_VERSION_MAJOR "${CMAKE_MATCH_1}")
  set(CPACK_PACKAGE_VERSION_MINOR "${CMAKE_MATCH_2}")
  if(CMAKE_MATCH_4)
    set(CPACK_PACKAGE_VERSION_PATCH "${CMAKE_MATCH_4}")
  else()
    set(CPACK_PACKAGE_VERSION_PATCH "0")
  endif()
else()
  set(CPACK_PACKAGE_VERSION_MAJOR "0")
  set(CPACK_PACKAGE_VERSION_MINOR "0")
  set(CPACK_PACKAGE_VERSION_PATCH "0")
endif()

if(DEFINED ENV{BOOST_HOME} AND NOT "$ENV{BOOST_HOME}" STREQUAL "")
  set(BOOST_ROOT "$ENV{BOOST_HOME}")
  list(PREPEND CMAKE_PREFIX_PATH "$ENV{BOOST_HOME}")
  set(Boost_USE_STATIC_RUNTIME ON)
  message(STATUS "Using BOOST_HOME from environment: $ENV{BOOST_HOME}")
endif()

find_package(Boost REQUIRED COMPONENTS regex program_options)
find_program(XSDCXX_EXECUTABLE NAMES xsdcxx)

if(DEFINED ENV{XERCES_C_HOME} AND NOT "$ENV{XERCES_C_HOME}" STREQUAL "")
  list(PREPEND CMAKE_PREFIX_PATH "$ENV{XERCES_C_HOME}")
  message(STATUS "Using XERCES_C_HOME from environment: $ENV{XERCES_C_HOME}")
endif()

find_package(XercesC QUIET)
if(TARGET XercesC::XercesC)
  set(UASAK_XERCES_TARGET XercesC::XercesC)
else()
  find_path(UASAK_XERCES_INCLUDE_DIR
    NAMES xercesc/util/XercesDefs.hpp
    HINTS
      "$ENV{XERCES_C_HOME}/include"
  )
  find_library(UASAK_XERCES_LIBRARY
    NAMES xerces-c
    HINTS
      "$ENV{XERCES_C_HOME}/lib"
      "$ENV{XERCES_C_HOME}/lib64"
  )
  if(NOT UASAK_XERCES_INCLUDE_DIR OR NOT UASAK_XERCES_LIBRARY)
    message(FATAL_ERROR "Failed to resolve Xerces-C. Set XERCES_C_HOME or provide a CMake package for XercesC.")
  endif()
  add_library(UASAK::XercesC UNKNOWN IMPORTED)
  set_target_properties(UASAK::XercesC PROPERTIES
    IMPORTED_LOCATION "${UASAK_XERCES_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${UASAK_XERCES_INCLUDE_DIR}"
  )
  set(UASAK_XERCES_TARGET UASAK::XercesC)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrepareOpen6Compat.cmake")
set(LOGIT_INCLUDE "${LOGIT_INCLUDE_DIR}")
if(NOT LOGIT_INCLUDE OR NOT EXISTS "${LOGIT_INCLUDE}")
  message(FATAL_ERROR "Failed to resolve LogIt include directory from open62541-compat")
endif()

add_executable(uasak_browse src/uasak_browse.cpp)

target_include_directories(uasak_browse PRIVATE
  "${PROJECT_BINARY_DIR}"
  ${UASAK_OPEN6COMPAT_INCLUDE_DIRS}
  "${LOGIT_INCLUDE}"
)
target_link_libraries(uasak_browse PRIVATE
  Open6Compat::open62541-compat
  Boost::regex
  Boost::program_options
)

if(XSDCXX_EXECUTABLE)
  add_custom_command(
    OUTPUT "${PROJECT_BINARY_DIR}/UANodeSet.cxx" "${PROJECT_BINARY_DIR}/UANodeSet.hxx"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND "${XSDCXX_EXECUTABLE}" cxx-tree --std c++11 --generate-serialization
            --namespace-map http://opcfoundation.org/UA/2011/03/UANodeSet.xsd=UANodeSet
            --omit-default-attributes --root-element UANodeSet
            --output-dir "${PROJECT_BINARY_DIR}" nodeset_xsd/UANodeSet.xsd
    DEPENDS "${PROJECT_SOURCE_DIR}/nodeset_xsd/UANodeSet.xsd"
  )

  add_executable(uasak_dump src/uasak_dump.cpp "${PROJECT_BINARY_DIR}/UANodeSet.cxx")
  target_include_directories(uasak_dump PRIVATE
    "${PROJECT_BINARY_DIR}"
    ${UASAK_OPEN6COMPAT_INCLUDE_DIRS}
    "${LOGIT_INCLUDE}"
  )
  if(WIN32 AND DEFINED ENV{XSD_HOME} AND NOT "$ENV{XSD_HOME}" STREQUAL "")
    target_include_directories(uasak_dump PRIVATE "$ENV{XSD_HOME}/include")
    message(STATUS "Using XSD_HOME from environment: $ENV{XSD_HOME}")
  endif()
  target_link_libraries(uasak_dump PRIVATE
    Open6Compat::open62541-compat
    ${UASAK_XERCES_TARGET}
    Boost::regex
    Boost::program_options
  )
else()
  message(WARNING "xsdcxx not found: target uasak_dump will not be built")
endif()

install(TARGETS uasak_browse RUNTIME DESTINATION "bin")
if(TARGET uasak_dump)
  install(TARGETS uasak_dump RUNTIME DESTINATION "bin")
endif()

include(CPack)
