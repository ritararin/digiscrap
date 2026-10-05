cmake_minimum_required(VERSION 3.10)
project(DigiScrap CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(scrapbook_modules
	src/lines/lines.cpp
	src/polygons/polygons.cpp
	src/clipping/clipping.cpp
	src/circles/circles.cpp
	src/curves/curves.cpp
	src/colour/colour.cpp)
target_include_directories(scrapbook_modules PUBLIC src)
if(MSVC)
	# The existing shape generators and demo use M_PI from <cmath>.
	target_compile_definitions(scrapbook_modules PUBLIC _USE_MATH_DEFINES)
endif()

add_executable(scrapbook src/main.cpp)
target_link_libraries(scrapbook PRIVATE scrapbook_modules)
add_executable(polygons_test src/polygons/test_polygons.cpp)
target_link_libraries(polygons_test PRIVATE scrapbook_modules)
add_executable(clipping_colour_test src/clipping/test_clipping_colour.cpp)
target_link_libraries(clipping_colour_test PRIVATE scrapbook_modules)

enable_testing()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output")
add_test(NAME polygons COMMAND polygons_test)
add_test(NAME clipping_colour COMMAND clipping_colour_test)
set_tests_properties(polygons clipping_colour PROPERTIES
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_target(check
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
	DEPENDS polygons_test clipping_colour_test
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
add_custom_target(run
	COMMAND $<TARGET_FILE:scrapbook>
	DEPENDS scrapbook
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")