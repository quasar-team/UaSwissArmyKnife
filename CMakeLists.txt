cmake_minimum_required(VERSION 3.0)
add_definitions(-std=c++11)
execute_process(COMMAND git describe --tags OUTPUT_VARIABLE git_describe_output OUTPUT_STRIP_TRAILING_WHITESPACE)
message("NOTE: git describe returned ${git_describe_output}")
file(WRITE ${PROJECT_BINARY_DIR}/uasak_version.h "#define UASAK_VERSION \"${git_describe_output}\"")

set(CPACK_PACKAGE_NAME "UaSwissArmyKnife" )
set(CPACK_PACKAGE_VENDOR "CERN/ATLAS-DCS/pnikiel" )
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/UaSwissArmyKnife")
set(CPACK_RPM_PACKAGE_NAME "${CPACK_PACKAGE_NAME}" )

string(REPLACE "." ";" versions_dot_sep "${git_describe_output}")
list(GET versions_dot_sep 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET versions_dot_sep 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET versions_dot_sep 2 CPACK_PACKAGE_VERSION_PATCH)


add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/UANodeSet.cxx ${PROJECT_BINARY_DIR}/UANodeSet.hxx
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND xsdcxx cxx-tree --std c++11 --generate-serialization --namespace-map http://opcfoundation.org/UA/2011/03/UANodeSet.xsd=UANodeSet --omit-default-attributes --root-element UANodeSet --output-dir ${PROJECT_BINARY_DIR} nodeset_xsd/UANodeSet.xsd
        DEPENDS ${PROJECT_SOURCE_DIR}/nodeset_xsd/UANodeSet.xsd
)

find_package( Boost REQUIRED COMPONENTS regex program_options )
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Failed to find boost installation")
else()
    message(STATUS "Found system boost, version [${Boost_VERSION}], include dir [${Boost_INCLUDE_DIRS}] library dir [${Boost_LIBRARY_DIRS}], libs [${Boost_LIBRARIES}]")
    set(BOOST_HEADERS ${Boost_INCLUDE_DIRS})
    set(BOOST_LIBS ${Boost_LIBRARIES})
    set(BOOST_LIB_DIRECTORIES ${Boost_LIBRARY_DIRS})
endif()

include_directories(
	build/deploy/open62541-compat/include
	build/deploy/LogIt/include
	${BOOST_HEADERS}
	${PROJECT_BINARY_DIR}
)

link_directories(
	build/deploy/open62541-compat/lib
	build/deploy/LogIt/lib
	${Boost_LIBRARY_DIRS}
)

add_executable (uasak_browse
	src/uasak_browse.cpp
	)

add_executable (uasak_dump
	src/uasak_dump.cpp
	${PROJECT_BINARY_DIR}/UANodeSet.cxx
)

add_executable (uasak_browse_read
	src/uasak_browse_read.cpp
)

target_link_libraries(uasak_browse -lopen62541-compat -lLogIt ${BOOST_LIBS})
target_link_libraries(uasak_dump -lopen62541-compat -lLogIt -lxerces-c ${BOOST_LIBS})
target_link_libraries(uasak_browse_read -lopen62541-compat -lLogIt ${BOOST_LIBS})

install(TARGETS uasak_browse uasak_dump RUNTIME DESTINATION bin)
