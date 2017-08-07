add_executable(asio_cares_tests
	cancel.cpp
	detail/select.cpp
	done.cpp
	error.cpp
	main.cpp
	process.cpp
	process_one.cpp
	send.cpp
	setup.cpp
)
target_link_libraries(asio_cares_tests
	asio_cares
	Catch
)
add_test(NAME asio_cares COMMAND asio_cares_tests)
