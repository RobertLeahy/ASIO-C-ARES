include(ExternalProject)
find_package(Git REQUIRED)
ExternalProject_Add(
	beast
	PREFIX ${CMAKE_BINARY_DIR}/beast
	GIT_REPOSITORY https://github.com/RobertLeahy/Beast.git
	TIMEOUT 10
	UPDATE_COMMAND ${GIT_EXECUTABLE} pull
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	LOG_DOWNLOAD ON
)
ExternalProject_Get_Property(beast source_dir)
add_library(Beast INTERFACE)
add_dependencies(Beast beast)
target_include_directories(Beast SYSTEM INTERFACE ${source_dir}/include)
target_link_libraries(Beast INTERFACE Asio Boost::boost Boost::system)
add_library(BeastExtras INTERFACE)
target_include_directories(BeastExtras SYSTEM INTERFACE ${source_dir}/extras)
target_link_libraries(BeastExtras INTERFACE Beast)
