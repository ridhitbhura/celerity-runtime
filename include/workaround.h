#pragma once

#include <cassert>

#include <CL/sycl.hpp>
#include <sycl/sycl.hpp>

#if defined(CELERITY_DPCPP)
#define CELERITY_WORKAROUND_DPCPP 1
#else
#define CELERITY_WORKAROUND_DPCPP 0
#endif

#if defined(__HIPSYCL__)
#define CELERITY_WORKAROUND_HIPSYCL 1
#define CELERITY_WORKAROUND_VERSION_MAJOR HIPSYCL_VERSION_MAJOR
#define CELERITY_WORKAROUND_VERSION_MINOR HIPSYCL_VERSION_MINOR
#define CELERITY_WORKAROUND_VERSION_PATCH HIPSYCL_VERSION_PATCH
#else
#define CELERITY_WORKAROUND_HIPSYCL 0
#endif

#define CELERITY_WORKAROUND_VERSION_LESS_OR_EQUAL_1(major) (CELERITY_WORKAROUND_VERSION_MAJOR <= major)
#define CELERITY_WORKAROUND_VERSION_LESS_OR_EQUAL_2(major, minor)                                                                                              \
	(CELERITY_WORKAROUND_VERSION_MAJOR < major) || (CELERITY_WORKAROUND_VERSION_MAJOR == major && CELERITY_WORKAROUND_VERSION_MINOR <= minor)
#define CELERITY_WORKAROUND_VERSION_LESS_OR_EQUAL_3(major, minor, patch)                                                                                       \
	(CELERITY_WORKAROUND_VERSION_MAJOR < major) || (CELERITY_WORKAROUND_VERSION_MAJOR == major && CELERITY_WORKAROUND_VERSION_MINOR < minor)                   \
	    || (CELERITY_WORKAROUND_VERSION_MAJOR == major && CELERITY_WORKAROUND_VERSION_MINOR == minor && CELERITY_WORKAROUND_VERSION_PATCH <= patch)

#define CELERITY_WORKAROUND_GET_OVERLOAD(_1, _2, _3, NAME, ...) NAME
#define CELERITY_WORKAROUND_MSVC_VA_ARGS_EXPANSION(x) x // Workaround for MSVC PP expansion behavior of __VA_ARGS__
#define CELERITY_WORKAROUND_VERSION_LESS_OR_EQUAL(...)                                                                                                         \
	CELERITY_WORKAROUND_MSVC_VA_ARGS_EXPANSION(CELERITY_WORKAROUND_GET_OVERLOAD(__VA_ARGS__, CELERITY_WORKAROUND_VERSION_LESS_OR_EQUAL_3,                      \
	    CELERITY_WORKAROUND_VERSION_LESS_OR_EQUAL_2, CELERITY_WORKAROUND_VERSION_LESS_OR_EQUAL_1)(__VA_ARGS__))

#define CELERITY_WORKAROUND(impl) (CELERITY_WORKAROUND_##impl == 1)
#define CELERITY_WORKAROUND_LESS_OR_EQUAL(impl, ...) (CELERITY_WORKAROUND(impl) && CELERITY_WORKAROUND_VERSION_LESS_OR_EQUAL(__VA_ARGS__))

#if __has_cpp_attribute(no_unique_address) // C++20, but implemented as an extension for earlier standards in Clang
#define CELERITY_DETAIL_HAS_NO_UNIQUE_ADDRESS true
#define CELERITY_DETAIL_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define CELERITY_DETAIL_HAS_NO_UNIQUE_ADDRESS false
#define CELERITY_DETAIL_NO_UNIQUE_ADDRESS
#endif

#if CELERITY_DETAIL_ENABLE_DEBUG && !defined(__SYCL_DEVICE_ONLY__)
#define CELERITY_DETAIL_ASSERT_ON_HOST(...) assert(__VA_ARGS__)
#else
#define CELERITY_DETAIL_ASSERT_ON_HOST(...)
#endif
