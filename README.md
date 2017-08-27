# ASIO C-ARES

[![Build Status](https://travis-ci.org/RobertLeahy/ASIO-C-ARES.svg?branch=master)](https://travis-ci.org/RobertLeahy/ASIO-C-ARES) [![Build status](https://ci.appveyor.com/api/projects/status/3ense6wg8vp96bf3/branch/master?svg=true)](https://ci.appveyor.com/project/RobertLeahy/asio-c-ares/branch/master)

## What is ASIO C-ARES

ASIO C-ARES is a C++14 library which allows the use of [libcares](https://c-ares.haxx.se/) with Boost.Asio. No need to manage `select` just use the provided composed asynchronous operations and run a `boost::asio::io_service`.

## How does it work?

1. Create an `asio_cares::library` object (an RAII wrapper for libcares global initialization/cleanup)
1. Create an `asio_cares::channel` object
2. Dispatch one or more questions on the channel using `async_send`
3. Call `async_process` or call `async_process_one` until `done` returns `true`

### Functions

- `done`

### Types

- `channel`
- `library`
- `string`

### Operations

- `async_process`
- `async_process_one`
- `async_send`
- `cancel`

## Dependencies

- Boost 1.58.0+
- libcares 1.13.0+

Other libraries are depended on, but are header only and are downloaded by CMake:

- Beast
- Catch
- MPark Variant

## Requirements

- Clang 4+ or GCC 6.2.0+ or Microsoft Visual C++ 2017+ (may work with earlier compilers, these are the oldest used for CI)
- CMake 3.5+

## Example

See unit tests.
