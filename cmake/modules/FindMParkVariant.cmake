include(ExternalProject)
find_package(Git REQUIRED)
ExternalProject_Add(
	mpark_variant
	PREFIX ${CMAKE_BINARY_DIR}/mpark_variant
	GIT_REPOSITORY https://github.com/mpark/variant.git
	TIMEOUT 10
	UPDATE_COMMAND ${GIT_EXECUTABLE} pull
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	LOG_DOWNLOAD ON
)
ExternalProject_Get_Property(mpark_variant source_dir)
add_library(MParkVariant INTERFACE)
add_dependencies(MParkVariant mpark_variant)
target_include_directories(MParkVariant SYSTEM INTERFACE ${source_dir}/include)
