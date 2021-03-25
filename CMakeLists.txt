cmake_minimum_required(VERSION 3.0)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/UANodeSet.cxx ${PROJECT_BINARY_DIR}/UANodeSet.hxx
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND xsdcxx cxx-tree --std c++11 --generate-serialization --namespace-map http://opcfoundation.org/UA/2011/03/UANodeSet.xsd=UANodeSet --omit-default-attributes --root-element UANodeSet --output-dir ${PROJECT_BINARY_DIR} nodeset_xsd/UANodeSet.xsd
        DEPENDS ${PROJECT_SOURCE_DIR}/nodeset_xsd/UANodeSet.xsd
)


include_directories(
	build/deploy/open62541-compat/include
	build/deploy/LogIt/include
	${PROJECT_BINARY_DIR}
)

link_directories(
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

install(TARGETS uasak_browse uasak_dump RUNTIME DESTINATION bin)
