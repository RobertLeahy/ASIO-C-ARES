cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(asio_cares LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DBOOST_ALL_NO_LIB)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
find_package(Beast REQUIRED)
find_package(Boost 1.58.0 REQUIRED system)
find_package(Catch REQUIRED)
find_package(Doxygen)
find_package(MParkVariant)
find_package(CARES REQUIRED)
add_library(Asio INTERFACE)
target_link_libraries(Asio INTERFACE Boost::boost Boost::system)
if(WIN32)
	target_link_libraries(Asio INTERFACE wsock32 ws2_32)
endif()
add_library(CARES INTERFACE)
target_include_directories(CARES SYSTEM INTERFACE ${CARES_INCLUDE_DIR})
target_link_libraries(CARES INTERFACE ${CARES_LIBRARIES})
#	The below works around static linking against
#	libcares: When you build libcares statically its
#	symbols are still all marked __declspec(dllimport)
#	in ares.h which means that the compiler emits
#	references to them prefixed by "__imp_" (which won't
#	exist since you're linking against a static library)
#	therefore we need to workaround this by defining
#	CARES_BUILDING_LIBRARY in this case which switches
#	those symbols to being prefixed by __declspec(dllexport)
#	which makes the references the compiler emits match up
#	to the symbols the linker receives
get_filename_component(CARES_LIBRARY_EXT ${CARES_LIBRARIES} EXT)
string(TOLOWER ${CARES_LIBRARY_EXT} CARES_LIBRARY_EXT)
if(${CARES_LIBRARY_EXT} STREQUAL ".lib")
	target_compile_definitions(CARES INTERFACE -DCARES_BUILDING_LIBRARY)
endif()
enable_testing() 
add_subdirectory(src)
if(DOXYGEN_FOUND)
	if(DOXYGEN_DOT_FOUND)
		set(DOXYGEN_CONFIGURE_HAVE_DOT "HAVE_DOT=YES")
		get_filename_component(DOXYGEN_DOT_PATH "${DOXYGEN_DOT_EXECUTABLE}" DIRECTORY)
		file(TO_NATIVE_PATH "${DOXYGEN_DOT_PATH}" DOXYGEN_DOT_PATH)
		set(DOXYGEN_CONFIGURE_DOT_PATH "DOT_PATH=${DOXYGEN_DOT_PATH}")
	endif()
	configure_file(Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile)
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "Running Doxygen" VERBATIM
	)
endif()
