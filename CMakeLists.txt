cmake_minimum_required(VERSION 3.0)

include_directories(
	build/deploy/open62541-compat/include
	build/deploy/LogIt/include
)

link_directories(
	build/deploy/open62541-compat/lib
	build/deploy/LogIt/lib)

add_executable (uasak_browse
	src/uasak_browse.cpp)

target_link_libraries(uasak_browse -lopen62541-compat -lLogIt -lboost_program_options)
