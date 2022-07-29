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


include_directories(
	/afs/cern.ch/user/p/pnikiel/PrepareEcosystem/PrepareEcosystem/deploy/LogIt/include
	/afs/cern.ch/user/p/pnikiel/PrepareEcosystem/PrepareEcosystem/deploy/open62541-compat/include
	build/deploy/open62541-compat/include
	build/deploy/LogIt/include
	${PROJECT_BINARY_DIR}
)

link_directories(
	/afs/cern.ch/user/p/pnikiel/PrepareEcosystem/PrepareEcosystem/deploy/LogIt/lib
	/afs/cern.ch/user/p/pnikiel/PrepareEcosystem/PrepareEcosystem/deploy/open62541-compat/lib
	build/deploy/open62541-compat/lib
	build/deploy/LogIt/lib)

add_executable (uasak_browse
	src/uasak_browse.cpp
	)

add_executable (uasak_dump
	src/uasak_dump.cpp
	${PROJECT_BINARY_DIR}/UANodeSet.cxx
)

target_link_libraries(uasak_browse -lopen62541-compat -lLogIt -lboost_program_options)
target_link_libraries(uasak_dump -lopen62541-compat -lLogIt -lboost_program_options -lxerces-c)

install(TARGETS uasak_browse uasak_dump RUNTIME DESTINATION "${CPACK_PACKAGING_INSTALL_PREFIX}/bin")

include(CPack)




