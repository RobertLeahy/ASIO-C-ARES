#	This file's contents are based on the one found here:
#	https://github.com/philsquared/Catch/blob/master/docs/build-systems.md#cmake
include(ExternalProject)
find_package(Git REQUIRED)
ExternalProject_Add(
	catch
	PREFIX ${CMAKE_BINARY_DIR}/catch
	GIT_REPOSITORY https://github.com/philsquared/Catch.git
	TIMEOUT 10
	UPDATE_COMMAND ${GIT_EXECUTABLE} pull
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	LOG_DOWNLOAD ON
)
ExternalProject_Get_Property(catch source_dir)
add_library(Catch INTERFACE)
add_dependencies(Catch catch)
target_include_directories(Catch SYSTEM INTERFACE ${source_dir}/include)
