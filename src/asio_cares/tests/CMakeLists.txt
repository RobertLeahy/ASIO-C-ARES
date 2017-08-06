add_executable(asio_cares_tests
	detail/select.cpp
	error.cpp
	main.cpp
)
target_link_libraries(asio_cares_tests
	asio_cares
	Catch
)
add_test(NAME asio_cares COMMAND asio_cares_tests)
