// The MIT License (MIT)

// Copyright (c) 2013-2020 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// This file was generated with a script.
// Generated 2022-06-25 08:14:19.328625 UTC
// This header was generated with sol v3.3.0 (revision eba86625)
// https://github.com/ThePhD/sol2

#ifndef SOL_SINGLE_INCLUDE_FORWARD_HPP
#define SOL_SINGLE_INCLUDE_FORWARD_HPP

// beginning of sol/forward.hpp

#ifndef SOL_FORWARD_HPP
#define SOL_FORWARD_HPP

// beginning of sol/version.hpp

#include <sol/config.hpp>

#define SOL_VERSION_MAJOR 3
#define SOL_VERSION_MINOR 2
#define SOL_VERSION_PATCH 3
#define SOL_VERSION_STRING "3.2.3"
#define SOL_VERSION ((SOL_VERSION_MAJOR * 100000) + (SOL_VERSION_MINOR * 100) + (SOL_VERSION_PATCH))

#define SOL_TOKEN_TO_STRING_POST_EXPANSION_I_(_TOKEN) #_TOKEN
#define SOL_TOKEN_TO_STRING_I_(_TOKEN) SOL_TOKEN_TO_STRING_POST_EXPANSION_I_(_TOKEN)

#define SOL_CONCAT_TOKENS_POST_EXPANSION_I_(_LEFT, _RIGHT) _LEFT##_RIGHT
#define SOL_CONCAT_TOKENS_I_(_LEFT, _RIGHT) SOL_CONCAT_TOKENS_POST_EXPANSION_I_(_LEFT, _RIGHT)

#define SOL_RAW_IS_ON(OP_SYMBOL) ((3 OP_SYMBOL 3) != 0)
#define SOL_RAW_IS_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3) == 0)
#define SOL_RAW_IS_DEFAULT_ON(OP_SYMBOL) ((3 OP_SYMBOL 3) > 3)
#define SOL_RAW_IS_DEFAULT_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3 OP_SYMBOL 3) < 0)

#define SOL_IS_ON(OP_SYMBOL) SOL_RAW_IS_ON(OP_SYMBOL ## _I_)
#define SOL_IS_OFF(OP_SYMBOL) SOL_RAW_IS_OFF(OP_SYMBOL ## _I_)
#define SOL_IS_DEFAULT_ON(OP_SYMBOL) SOL_RAW_IS_DEFAULT_ON(OP_SYMBOL ## _I_)
#define SOL_IS_DEFAULT_OFF(OP_SYMBOL) SOL_RAW_IS_DEFAULT_OFF(OP_SYMBOL ## _I_)

#define SOL_ON          |
#define SOL_OFF         ^
#define SOL_DEFAULT_ON  +
#define SOL_DEFAULT_OFF -

#if defined(SOL_BUILD_CXX_MODE)
	#if (SOL_BUILD_CXX_MODE != 0)
		#define SOL_BUILD_CXX_MODE_I_ SOL_ON
	#else
		#define SOL_BUILD_CXX_MODE_I_ SOL_OFF
	#endif
#elif defined(__cplusplus)
	#define SOL_BUILD_CXX_MODE_I_ SOL_DEFAULT_ON
#else
	#define SOL_BUILD_CXX_MODE_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_BUILD_C_MODE)
	#if (SOL_BUILD_C_MODE != 0)
		#define SOL_BUILD_C_MODE_I_ SOL_ON
	#else
		#define SOL_BUILD_C_MODE_I_ SOL_OFF
	#endif
#elif defined(__STDC__)
	#define SOL_BUILD_C_MODE_I_ SOL_DEFAULT_ON
#else
	#define SOL_BUILD_C_MODE_I_ SOL_DEFAULT_OFF
#endif

#if SOL_IS_ON(SOL_BUILD_C_MODE)
	#include <stddef.h>
	#include <stdint.h>
	#include <limits.h>
#else
	#include <cstddef>
	#include <cstdint>
	#include <climits>
#endif

#if defined(SOL_COMPILER_VCXX)
	#if defined(SOL_COMPILER_VCXX != 0)
		#define SOL_COMPILER_VCXX_I_ SOL_ON
	#else
		#define SOL_COMPILER_VCXX_I_ SOL_OFF
	#endif
#elif defined(_MSC_VER)
	#define SOL_COMPILER_VCXX_I_ SOL_DEFAULT_ON
#else
	#define SOL_COMPILER_VCXX_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_COMPILER_GCC)
	#if defined(SOL_COMPILER_GCC != 0)
		#define SOL_COMPILER_GCC_I_ SOL_ON
	#else
		#define SOL_COMPILER_GCC_I_ SOL_OFF
	#endif
#elif defined(__GNUC__)
	#define SOL_COMPILER_GCC_I_ SOL_DEFAULT_ON
#else
	#define SOL_COMPILER_GCC_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_COMPILER_CLANG)
	#if defined(SOL_COMPILER_CLANG != 0)
		#define SOL_COMPILER_CLANG_I_ SOL_ON
	#else
		#define SOL_COMPILER_CLANG_I_ SOL_OFF
	#endif
#elif defined(__clang__)
	#define SOL_COMPILER_CLANG_I_ SOL_DEFAULT_ON
#else
	#define SOL_COMPILER_CLANG_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_COMPILER_EDG)
	#if defined(SOL_COMPILER_EDG != 0)
		#define SOL_COMPILER_EDG_I_ SOL_ON
	#else
		#define SOL_COMPILER_EDG_I_ SOL_OFF
	#endif
#else
	#define SOL_COMPILER_EDG_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_COMPILER_MINGW)
	#if (SOL_COMPILER_MINGW != 0)
		#define SOL_COMPILER_MINGW_I_ SOL_ON
	#else
		#define SOL_COMPILER_MINGW_I_ SOL_OFF
	#endif
#elif defined(__MINGW32__)
	#define SOL_COMPILER_MINGW_I_ SOL_DEFAULT_ON
#else
	#define SOL_COMPILER_MINGW_I_ SOL_DEFAULT_OFF
#endif

#if SIZE_MAX <= 0xFFFFULL
	#define SOL_PLATFORM_X16_I_ SOL_ON
	#define SOL_PLATFORM_X86_I_ SOL_OFF
	#define SOL_PLATFORM_X64_I_ SOL_OFF
#elif SIZE_MAX <= 0xFFFFFFFFULL
	#define SOL_PLATFORM_X16_I_ SOL_OFF
	#define SOL_PLATFORM_X86_I_ SOL_ON
	#define SOL_PLATFORM_X64_I_ SOL_OFF
#else
	#define SOL_PLATFORM_X16_I_ SOL_OFF
	#define SOL_PLATFORM_X86_I_ SOL_OFF
	#define SOL_PLATFORM_X64_I_ SOL_ON
#endif

#define SOL_PLATFORM_ARM32_I_ SOL_OFF
#define SOL_PLATFORM_ARM64_I_ SOL_OFF

#if defined(SOL_PLATFORM_WINDOWS)
	#if (SOL_PLATFORM_WINDOWS != 0)
		#define SOL_PLATFORM_WINDOWS_I_ SOL_ON
	#else
		#define SOL_PLATFORM_WINDOWS_I_ SOL_OFF
	#endif
#elif defined(_WIN32)
	#define SOL_PLATFORM_WINDOWS_I_ SOL_DEFAULT_ON
#else
	#define SOL_PLATFORM_WINDOWS_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_PLATFORM_CYGWIN)
	#if (SOL_PLATFORM_CYGWIN != 0)
		#define SOL_PLATFORM_CYGWIN_I_ SOL_ON
	#else
		#define SOL_PLATFORM_CYGWIN_I_ SOL_ON
	#endif
#elif defined(__CYGWIN__)
	#define SOL_PLATFORM_CYGWIN_I_ SOL_DEFAULT_ON
#else
	#define SOL_PLATFORM_CYGWIN_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_PLATFORM_APPLE)
	#if (SOL_PLATFORM_APPLE != 0)
		#define SOL_PLATFORM_APPLE_I_ SOL_ON
	#else
		#define SOL_PLATFORM_APPLE_I_ SOL_OFF
	#endif
#elif defined(__APPLE__)
	#define SOL_PLATFORM_APPLE_I_ SOL_DEFAULT_ON
#else
	#define SOL_PLATFORM_APPLE_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_PLATFORM_UNIX)
	#if (SOL_PLATFORM_UNIX != 0)
		#define SOL_PLATFORM_UNIXLIKE_I_ SOL_ON
	#else
		#define SOL_PLATFORM_UNIXLIKE_I_ SOL_OFF
	#endif
#elif defined(__unix__)
	#define SOL_PLATFORM_UNIXLIKE_I_ SOL_DEFAUKT_ON
#else
	#define SOL_PLATFORM_UNIXLIKE_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_PLATFORM_LINUX)
	#if (SOL_PLATFORM_LINUX != 0)
		#define SOL_PLATFORM_LINUXLIKE_I_ SOL_ON
	#else
		#define SOL_PLATFORM_LINUXLIKE_I_ SOL_OFF
	#endif
#elif defined(__LINUX__)
	#define SOL_PLATFORM_LINUXLIKE_I_ SOL_DEFAUKT_ON
#else
	#define SOL_PLATFORM_LINUXLIKE_I_ SOL_DEFAULT_OFF
#endif

#define SOL_PLATFORM_APPLE_IPHONE_I_ SOL_OFF
#define SOL_PLATFORM_BSDLIKE_I_      SOL_OFF

#if defined(SOL_IN_DEBUG_DETECTED)
	#if SOL_IN_DEBUG_DETECTED != 0
		#define SOL_DEBUG_BUILD_I_ SOL_ON
	#else
		#define SOL_DEBUG_BUILD_I_ SOL_OFF
	#endif
#elif !defined(NDEBUG)
	#if SOL_IS_ON(SOL_COMPILER_VCXX) && defined(_DEBUG)
		#define SOL_DEBUG_BUILD_I_ SOL_ON
	#elif (SOL_IS_ON(SOL_COMPILER_CLANG) || SOL_IS_ON(SOL_COMPILER_GCC)) && !defined(__OPTIMIZE__)
		#define SOL_DEBUG_BUILD_I_ SOL_ON
	#else
		#define SOL_DEBUG_BUILD_I_ SOL_OFF
	#endif
#else
	#define SOL_DEBUG_BUILD_I_ SOL_DEFAULT_OFF
#endif // We are in a debug mode of some sort

#if defined(SOL_NO_EXCEPTIONS)
	#if (SOL_NO_EXCEPTIONS != 0)
		#define SOL_EXCEPTIONS_I_ SOL_OFF
	#else
		#define SOL_EXCEPTIONS_I_ SOL_ON
	#endif
#elif SOL_IS_ON(SOL_COMPILER_VCXX)
	#if !defined(_CPPUNWIND)
		#define SOL_EXCEPTIONS_I_ SOL_OFF
	#else
		#define SOL_EXCEPTIONS_I_ SOL_ON
	#endif
#elif SOL_IS_ON(SOL_COMPILER_CLANG) || SOL_IS_ON(SOL_COMPILER_GCC)
	#if !defined(__EXCEPTIONS)
		#define SOL_EXCEPTIONS_I_ SOL_OFF
	#else
		#define SOL_EXCEPTIONS_I_ SOL_ON
	#endif
#else
	#define SOL_EXCEPTIONS_I_ SOL_DEFAULT_ON
#endif

#if defined(SOL_NO_RTTI)
	#if (SOL_NO_RTTI != 0)
		#define SOL_RTTI_I_ SOL_OFF
	#else
		#define SOL_RTTI_I_ SOL_ON
	#endif
#elif SOL_IS_ON(SOL_COMPILER_VCXX)
	#if !defined(_CPPRTTI)
		#define SOL_RTTI_I_ SOL_OFF
	#else
		#define SOL_RTTI_I_ SOL_ON
	#endif
#elif SOL_IS_ON(SOL_COMPILER_CLANG) || SOL_IS_ON(SOL_COMPILER_GCC)
	#if !defined(__GXX_RTTI)
		#define SOL_RTTI_I_ SOL_OFF
	#else
		#define SOL_RTTI_I_ SOL_ON
	#endif
#else
	#define SOL_RTTI_I_ SOL_DEFAULT_ON
#endif

#if defined(SOL_NO_THREAD_LOCAL)
	#if SOL_NO_THREAD_LOCAL != 0
		#define SOL_USE_THREAD_LOCAL_I_ SOL_OFF
	#else
		#define SOL_USE_THREAD_LOCAL_I_ SOL_ON
	#endif
#else
	#define SOL_USE_THREAD_LOCAL_I_ SOL_DEFAULT_ON
#endif // thread_local keyword is bjorked on some platforms

#if defined(SOL_ALL_SAFETIES_ON)
	#if SOL_ALL_SAFETIES_ON != 0
		#define SOL_ALL_SAFETIES_ON_I_ SOL_ON
	#else
		#define SOL_ALL_SAFETIES_ON_I_ SOL_OFF
	#endif
#else
	#define SOL_ALL_SAFETIES_ON_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_SAFE_GETTER)
	#if SOL_SAFE_GETTER != 0
		#define SOL_SAFE_GETTER_I_ SOL_ON
	#else
		#define SOL_SAFE_GETTER_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_SAFE_GETTER_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_SAFE_GETTER_I_ SOL_DEFAULT_ON
	#else
		#define SOL_SAFE_GETTER_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_SAFE_USERTYPE)
	#if SOL_SAFE_USERTYPE != 0
		#define SOL_SAFE_USERTYPE_I_ SOL_ON
	#else
		#define SOL_SAFE_USERTYPE_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_SAFE_USERTYPE_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_SAFE_USERTYPE_I_ SOL_DEFAULT_ON
	#else
		#define SOL_SAFE_USERTYPE_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_SAFE_REFERENCES)
	#if SOL_SAFE_REFERENCES != 0
		#define SOL_SAFE_REFERENCES_I_ SOL_ON
	#else
		#define SOL_SAFE_REFERENCES_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_SAFE_REFERENCES_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_SAFE_REFERENCES_I_ SOL_DEFAULT_ON
	#else
		#define SOL_SAFE_REFERENCES_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_SAFE_FUNCTIONS)
	#if SOL_SAFE_FUNCTIONS != 0
		#define SOL_SAFE_FUNCTION_OBJECTS_I_ SOL_ON
	#else
		#define SOL_SAFE_FUNCTION_OBJECTS_I_ SOL_OFF
	#endif
#elif defined (SOL_SAFE_FUNCTION_OBJECTS)
	#if SOL_SAFE_FUNCTION_OBJECTS != 0
		#define SOL_SAFE_FUNCTION_OBJECTS_I_ SOL_ON
	#else
		#define SOL_SAFE_FUNCTION_OBJECTS_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_SAFE_FUNCTION_OBJECTS_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_SAFE_FUNCTION_OBJECTS_I_ SOL_DEFAULT_ON
	#else
		#define SOL_SAFE_FUNCTION_OBJECTS_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_SAFE_FUNCTION_CALLS)
	#if SOL_SAFE_FUNCTION_CALLS != 0
		#define SOL_SAFE_FUNCTION_CALLS_I_ SOL_ON
	#else
		#define SOL_SAFE_FUNCTION_CALLS_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_SAFE_FUNCTION_CALLS_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_SAFE_FUNCTION_CALLS_I_ SOL_DEFAULT_ON
	#else
		#define SOL_SAFE_FUNCTION_CALLS_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_SAFE_PROXIES)
	#if SOL_SAFE_PROXIES != 0
		#define SOL_SAFE_PROXIES_I_ SOL_ON
	#else
		#define SOL_SAFE_PROXIES_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_SAFE_PROXIES_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_SAFE_PROXIES_I_ SOL_DEFAULT_ON
	#else
		#define SOL_SAFE_PROXIES_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_SAFE_NUMERICS)
	#if SOL_SAFE_NUMERICS != 0
		#define SOL_SAFE_NUMERICS_I_ SOL_ON
	#else
		#define SOL_SAFE_NUMERICS_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_SAFE_NUMERICS_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_SAFE_NUMERICS_I_ SOL_DEFAULT_ON
	#else
		#define SOL_SAFE_NUMERICS_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_ALL_INTEGER_VALUES_FIT)
	#if (SOL_ALL_INTEGER_VALUES_FIT != 0)
		#define SOL_ALL_INTEGER_VALUES_FIT_I_ SOL_ON
	#else
		#define SOL_ALL_INTEGER_VALUES_FIT_I_ SOL_OFF
	#endif
#elif !SOL_IS_DEFAULT_OFF(SOL_SAFE_NUMERICS) && SOL_IS_OFF(SOL_SAFE_NUMERICS)
	// if numerics is intentionally turned off, flip this on
	#define SOL_ALL_INTEGER_VALUES_FIT_I_ SOL_DEFAULT_ON
#else
	// default to off
	#define SOL_ALL_INTEGER_VALUES_FIT_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_SAFE_STACK_CHECK)
	#if SOL_SAFE_STACK_CHECK != 0
		#define SOL_SAFE_STACK_CHECK_I_ SOL_ON
	#else
		#define SOL_SAFE_STACK_CHECK_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_SAFE_STACK_CHECK_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_SAFE_STACK_CHECK_I_ SOL_DEFAULT_ON
	#else
		#define SOL_SAFE_STACK_CHECK_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_NO_CHECK_NUMBER_PRECISION)
	#if SOL_NO_CHECK_NUMBER_PRECISION != 0
		#define SOL_NUMBER_PRECISION_CHECKS_I_ SOL_OFF
	#else
		#define SOL_NUMBER_PRECISION_CHECKS_I_ SOL_ON
	#endif
#elif defined(SOL_NO_CHECKING_NUMBER_PRECISION)
	#if SOL_NO_CHECKING_NUMBER_PRECISION != 0
		#define SOL_NUMBER_PRECISION_CHECKS_I_ SOL_OFF
	#else
		#define SOL_NUMBER_PRECISION_CHECKS_I_ SOL_ON
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_NUMBER_PRECISION_CHECKS_I_ SOL_ON
	#elif SOL_IS_ON(SOL_SAFE_NUMERICS)
		#define SOL_NUMBER_PRECISION_CHECKS_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_NUMBER_PRECISION_CHECKS_I_ SOL_DEFAULT_ON
	#else
		#define SOL_NUMBER_PRECISION_CHECKS_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_STRINGS_ARE_NUMBERS)
	#if (SOL_STRINGS_ARE_NUMBERS != 0)
		#define SOL_STRINGS_ARE_NUMBERS_I_ SOL_ON
	#else
		#define SOL_STRINGS_ARE_NUMBERS_I_ SOL_OFF
	#endif
#else
	#define SOL_STRINGS_ARE_NUMBERS_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_ENABLE_INTEROP)
	#if SOL_ENABLE_INTEROP != 0
		#define SOL_USE_INTEROP_I_ SOL_ON
	#else
		#define SOL_USE_INTEROP_I_ SOL_OFF
	#endif
#elif defined(SOL_USE_INTEROP)
	#if SOL_USE_INTEROP != 0
		#define SOL_USE_INTEROP_I_ SOL_ON
	#else
		#define SOL_USE_INTEROP_I_ SOL_OFF
	#endif
#else
	#define SOL_USE_INTEROP_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_NO_NIL)
	#if (SOL_NO_NIL != 0)
		#define SOL_NIL_I_ SOL_OFF
	#else
		#define SOL_NIL_I_ SOL_ON
	#endif
#elif defined(__MAC_OS_X_VERSION_MAX_ALLOWED) || defined(__OBJC__) || defined(nil)
	#define SOL_NIL_I_ SOL_DEFAULT_OFF
#else
	#define SOL_NIL_I_ SOL_DEFAULT_ON
#endif

#if defined(SOL_USERTYPE_TYPE_BINDING_INFO)
	#if (SOL_USERTYPE_TYPE_BINDING_INFO != 0)
		#define SOL_USERTYPE_TYPE_BINDING_INFO_I_ SOL_ON
	#else
		#define SOL_USERTYPE_TYPE_BINDING_INFO_I_ SOL_OFF
	#endif
#else
	#define SOL_USERTYPE_TYPE_BINDING_INFO_I_ SOL_DEFAULT_ON
#endif // We should generate a my_type.__type table with lots of class information for usertypes

#if defined(SOL_AUTOMAGICAL_TYPES_BY_DEFAULT)
	#if (SOL_AUTOMAGICAL_TYPES_BY_DEFAULT != 0)
		#define SOL_DEFAULT_AUTOMAGICAL_USERTYPES_I_ SOL_ON
	#else
		#define SOL_DEFAULT_AUTOMAGICAL_USERTYPES_I_ SOL_OFF
	#endif
#elif defined(SOL_DEFAULT_AUTOMAGICAL_USERTYPES)
	#if (SOL_DEFAULT_AUTOMAGICAL_USERTYPES != 0)
		#define SOL_DEFAULT_AUTOMAGICAL_USERTYPES_I_ SOL_ON
	#else
		#define SOL_DEFAULT_AUTOMAGICAL_USERTYPES_I_ SOL_OFF
	#endif
#else
	#define SOL_DEFAULT_AUTOMAGICAL_USERTYPES_I_ SOL_DEFAULT_ON
#endif // make is_automagical on/off by default

#if defined(SOL_STD_VARIANT)
	#if (SOL_STD_VARIANT != 0)
		#define SOL_STD_VARIANT_I_ SOL_ON
	#else
		#define SOL_STD_VARIANT_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_COMPILER_CLANG) && SOL_IS_ON(SOL_PLATFORM_APPLE)
		#if defined(__has_include)
			#if __has_include(<variant>)
				#define SOL_STD_VARIANT_I_ SOL_DEFAULT_ON
			#else
				#define SOL_STD_VARIANT_I_ SOL_DEFAULT_OFF
			#endif
		#else
			#define SOL_STD_VARIANT_I_ SOL_DEFAULT_OFF
		#endif
	#else
		#define SOL_STD_VARIANT_I_ SOL_DEFAULT_ON
	#endif
#endif // make is_automagical on/off by default

#if defined(SOL_NOEXCEPT_FUNCTION_TYPE)
	#if (SOL_NOEXCEPT_FUNCTION_TYPE != 0)
		#define SOL_USE_NOEXCEPT_FUNCTION_TYPE_I_ SOL_ON
	#else
		#define SOL_USE_NOEXCEPT_FUNCTION_TYPE_I_ SOL_OFF
	#endif
#else
	#if defined(__cpp_noexcept_function_type)
		#define SOL_USE_NOEXCEPT_FUNCTION_TYPE_I_ SOL_ON
	#elif SOL_IS_ON(SOL_COMPILER_VCXX) && (defined(_MSVC_LANG) && (_MSVC_LANG < 201403L))
		// There is a bug in the VC++ compiler??
		// on /std:c++latest under x86 conditions (VS 15.5.2),
		// compiler errors are tossed for noexcept markings being on function types
		// that are identical in every other way to their non-noexcept marked types function types...
		// VS 2019: There is absolutely a bug.
		#define SOL_USE_NOEXCEPT_FUNCTION_TYPE_I_ SOL_OFF
	#else
		#define SOL_USE_NOEXCEPT_FUNCTION_TYPE_I_ SOL_DEFAULT_ON
	#endif
#endif // noexcept is part of a function's type

#if defined(SOL_STACK_STRING_OPTIMIZATION_SIZE) && SOL_STACK_STRING_OPTIMIZATION_SIZE > 0
	#define SOL_OPTIMIZATION_STRING_CONVERSION_STACK_SIZE_I_ SOL_STACK_STRING_OPTIMIZATION_SIZE
#else
	#define SOL_OPTIMIZATION_STRING_CONVERSION_STACK_SIZE_I_ 1024
#endif

#if defined(SOL_ID_SIZE) && SOL_ID_SIZE > 0
	#define SOL_ID_SIZE_I_ SOL_ID_SIZE
#else
	#define SOL_ID_SIZE_I_ 512
#endif

#if defined(LUA_IDSIZE) && LUA_IDSIZE > 0
	#define SOL_FILE_ID_SIZE_I_ LUA_IDSIZE
#elif defined(SOL_ID_SIZE) && SOL_ID_SIZE > 0
	#define SOL_FILE_ID_SIZE_I_ SOL_FILE_ID_SIZE
#else
	#define SOL_FILE_ID_SIZE_I_ 2048
#endif

#if defined(SOL_PRINT_ERRORS)
	#if (SOL_PRINT_ERRORS != 0)
		#define SOL_PRINT_ERRORS_I_ SOL_ON
	#else
		#define SOL_PRINT_ERRORS_I_ SOL_OFF
	#endif
#else
	#if SOL_IS_ON(SOL_ALL_SAFETIES_ON)
		#define SOL_PRINT_ERRORS_I_ SOL_ON
	#elif SOL_IS_ON(SOL_DEBUG_BUILD)
		#define SOL_PRINT_ERRORS_I_ SOL_DEFAULT_ON
	#else
		#define SOL_PRINT_ERRORS_I_ SOL_OFF
	#endif
#endif

#if defined(SOL_DEFAULT_PASS_ON_ERROR)
	#if (SOL_DEFAULT_PASS_ON_ERROR != 0)
		#define SOL_DEFAULT_PASS_ON_ERROR_I_ SOL_ON
	#else
		#define SOL_DEFAULT_PASS_ON_ERROR_I_ SOL_OFF
	#endif
#else
	#define SOL_DEFAULT_PASS_ON_ERROR_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_USING_CXX_LUA)
	#if (SOL_USING_CXX_LUA != 0)
		#define SOL_USE_CXX_LUA_I_ SOL_ON
	#else
		#define SOL_USE_CXX_LUA_I_ SOL_OFF
	#endif
#elif defined(SOL_USE_CXX_LUA)
	#if (SOL_USE_CXX_LUA != 0)
		#define SOL_USE_CXX_LUA_I_ SOL_ON
	#else
		#define SOL_USE_CXX_LUA_I_ SOL_OFF
	#endif
#else
	#define SOL_USE_CXX_LUA_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_USING_CXX_LUAJIT)
	#if (SOL_USING_CXX_LUA != 0)
		#define SOL_USE_CXX_LUAJIT_I_ SOL_ON
	#else
		#define SOL_USE_CXX_LUAJIT_I_ SOL_OFF
	#endif
#elif defined(SOL_USE_CXX_LUAJIT)
	#if (SOL_USE_CXX_LUA != 0)
		#define SOL_USE_CXX_LUAJIT_I_ SOL_ON
	#else
		#define SOL_USE_CXX_LUAJIT_I_ SOL_OFF
	#endif
#else
	#define SOL_USE_CXX_LUAJIT_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_NO_LUA_HPP)
	#if (SOL_NO_LUA_HPP != 0)
		#define SOL_USE_LUA_HPP_I_ SOL_OFF
	#else
		#define SOL_USE_LUA_HPP_I_ SOL_ON
	#endif
#elif defined(SOL_USING_CXX_LUA)
	#define SOL_USE_LUA_HPP_I_ SOL_OFF
#elif defined(__has_include)
	#if __has_include(<lua.hpp>)
		#define SOL_USE_LUA_HPP_I_ SOL_ON
	#else
		#define SOL_USE_LUA_HPP_I_ SOL_OFF
	#endif
#else
	#define SOL_USE_LUA_HPP_I_ SOL_DEFAULT_ON
#endif

#if defined(SOL_CONTAINERS_START)
	#define SOL_CONTAINER_START_INDEX_I_ SOL_CONTAINERS_START
#elif defined(SOL_CONTAINERS_START_INDEX)
	#define SOL_CONTAINER_START_INDEX_I_ SOL_CONTAINERS_START_INDEX
#elif defined(SOL_CONTAINER_START_INDEX)
	#define SOL_CONTAINER_START_INDEX_I_ SOL_CONTAINER_START_INDEX
#else
	#define SOL_CONTAINER_START_INDEX_I_ 1
#endif

#if defined (SOL_NO_MEMORY_ALIGNMENT)
	#if (SOL_NO_MEMORY_ALIGNMENT != 0)
		#define SOL_ALIGN_MEMORY_I_ SOL_OFF
	#else
		#define SOL_ALIGN_MEMORY_I_ SOL_ON
	#endif
#else
	#define SOL_ALIGN_MEMORY_I_ SOL_DEFAULT_ON
#endif

#if defined(SOL_USE_BOOST)
	#if (SOL_USE_BOOST != 0)
		#define SOL_USE_BOOST_I_ SOL_ON
	#else
		#define SOL_USE_BOOST_I_ SOL_OFF
	#endif
#else
		#define SOL_USE_BOOST_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_USE_UNSAFE_BASE_LOOKUP)
	#if (SOL_USE_UNSAFE_BASE_LOOKUP != 0)
		#define SOL_USE_UNSAFE_BASE_LOOKUP_I_ SOL_ON
	#else
		#define SOL_USE_UNSAFE_BASE_LOOKUP_I_ SOL_OFF
	#endif
#else
	#define SOL_USE_UNSAFE_BASE_LOOKUP_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_INSIDE_UNREAL)
	#if (SOL_INSIDE_UNREAL != 0)
		#define SOL_INSIDE_UNREAL_ENGINE_I_ SOL_ON
	#else
		#define SOL_INSIDE_UNREAL_ENGINE_I_ SOL_OFF
	#endif
#else
	#if defined(UE_BUILD_DEBUG) || defined(UE_BUILD_DEVELOPMENT) || defined(UE_BUILD_TEST) || defined(UE_BUILD_SHIPPING) || defined(UE_SERVER)
		#define SOL_INSIDE_UNREAL_ENGINE_I_ SOL_DEFAULT_ON
	#else
		#define SOL_INSIDE_UNREAL_ENGINE_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if defined(SOL_NO_COMPAT)
	#if (SOL_NO_COMPAT != 0)
		#define SOL_USE_COMPATIBILITY_LAYER_I_ SOL_OFF
	#else
		#define SOL_USE_COMPATIBILITY_LAYER_I_ SOL_ON
	#endif
#else
	#define SOL_USE_COMPATIBILITY_LAYER_I_ SOL_DEFAULT_ON
#endif

#if defined(SOL_GET_FUNCTION_POINTER_UNSAFE)
	#if (SOL_GET_FUNCTION_POINTER_UNSAFE != 0)
		#define SOL_GET_FUNCTION_POINTER_UNSAFE_I_ SOL_ON
	#else
		#define SOL_GET_FUNCTION_POINTER_UNSAFE_I_ SOL_OFF
	#endif
#else
	#define SOL_GET_FUNCTION_POINTER_UNSAFE_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_FUNCTION_CALL_VALUE_SEMANTICS)
	#if (SOL_FUNCTION_CALL_VALUE_SEMANTICS != 0)
		#define SOL_FUNCTION_CALL_VALUE_SEMANTICS_I_ SOL_ON
	#else
		#define SOL_FUNCTION_CALL_VALUE_SEMANTICS_I_ SOL_OFF
	#endif
#else
	#define SOL_FUNCTION_CALL_VALUE_SEMANTICS_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_MINGW_CCTYPE_IS_POISONED)
	#if (SOL_MINGW_CCTYPE_IS_POISONED != 0)
		#define SOL_MINGW_CCTYPE_IS_POISONED_I_ SOL_ON
	#else
		#define SOL_MINGW_CCTYPE_IS_POISONED_I_ SOL_OFF
	#endif
#elif SOL_IS_ON(SOL_COMPILER_MINGW) && defined(__GNUC__) && (__GNUC__ < 6)
	// MinGW is off its rocker in some places...
	#define SOL_MINGW_CCTYPE_IS_POISONED_I_ SOL_DEFAULT_ON
#else
	#define SOL_MINGW_CCTYPE_IS_POISONED_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_CHAR8_T)
	#if (SOL_CHAR8_T != 0)
		#define SOL_CHAR8_T_I_ SOL_ON
	#else
		#define SOL_CHAR8_T_I_ SOL_OFF
	#endif
#else
	#if defined(__cpp_char8_t)
		#define SOL_CHAR8_T_I_ SOL_DEFAULT_ON
	#else
		#define SOL_CHAR8_T_I_ SOL_DEFAULT_OFF
	#endif
#endif

#if SOL_IS_ON(SOL_USE_BOOST)
	#include <boost/version.hpp>

	#if BOOST_VERSION >= 107500 // Since Boost 1.75.0 boost::none is constexpr
		#define SOL_BOOST_NONE_CONSTEXPR_I_ constexpr
	#else
		#define SOL_BOOST_NONE_CONSTEXPR_I_ const
	#endif // BOOST_VERSION
#else
	// assume boost isn't using a garbage version
	#define SOL_BOOST_NONE_CONSTEXPR_I_ constexpr
#endif

#if defined(SOL2_CI)
	#if (SOL2_CI != 0)
		#define SOL2_CI_I_ SOL_ON
	#else
		#define SOL2_CI_I_ SOL_OFF
	#endif
#else
	#define SOL2_CI_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_C_ASSERT)
	#define SOL_USER_C_ASSERT_I_ SOL_ON
#else
	#define SOL_USER_C_ASSERT_I_ SOL_DEFAULT_OFF
#endif

#if defined(SOL_M_ASSERT)
	#define SOL_USER_M_ASSERT_I_ SOL_ON
#else
	#define SOL_USER_M_ASSERT_I_ SOL_DEFAULT_OFF
#endif

// beginning of sol/prologue.hpp

#if defined(SOL_PROLOGUE_I_)
	#error "[sol2] Library Prologue was already included in translation unit and not properly ended with an epilogue."
#endif

#define SOL_PROLOGUE_I_ 1

#if SOL_IS_ON(SOL_BUILD_CXX_MODE)
	#define _FWD(...) static_cast<decltype( __VA_ARGS__ )&&>( __VA_ARGS__ )

	#if SOL_IS_ON(SOL_COMPILER_GCC) || SOL_IS_ON(SOL_COMPILER_CLANG)
		#define _MOVE(...) static_cast<__typeof( __VA_ARGS__ )&&>( __VA_ARGS__ )
	#else
		#include <type_traits>
		
		#define _MOVE(...) static_cast<::std::remove_reference_t<( __VA_ARGS__ )>&&>( __VA_OPT__(,) )
	#endif
#endif

// end of sol/prologue.hpp

// beginning of sol/epilogue.hpp

#if !defined(SOL_PROLOGUE_I_)
	#error "[sol2] Library Prologue is missing from this translation unit."
#else
	#undef SOL_PROLOGUE_I_
#endif

#if SOL_IS_ON(SOL_BUILD_CXX_MODE)
	#undef _FWD
	#undef _MOVE
#endif

// end of sol/epilogue.hpp

// beginning of sol/detail/build_version.hpp

#if defined(SOL_DLL)
	#if (SOL_DLL != 0)
		#define SOL_DLL_I_ SOL_ON
	#else
		#define SOL_DLL_I_ SOL_OFF
	#endif
#elif SOL_IS_ON(SOL_COMPILER_VCXX) && (defined(DLL_) || defined(_DLL))
	#define SOL_DLL_I_ SOL_DEFAULT_ON
#else
	#define SOL_DLL_I_ SOL_DEFAULT_OFF
#endif // DLL definition

#if defined(SOL_HEADER_ONLY)
	#if (SOL_HEADER_ONLY != 0)
		#define SOL_HEADER_ONLY_I_ SOL_ON
	#else
		#define SOL_HEADER_ONLY_I_ SOL_OFF
	#endif
#else
	#define SOL_HEADER_ONLY_I_ SOL_DEFAULT_OFF
#endif // Header only library

#if defined(SOL_BUILD)
	#if (SOL_BUILD != 0)
		#define SOL_BUILD_I_ SOL_ON
	#else
		#define SOL_BUILD_I_ SOL_OFF
	#endif
#elif SOL_IS_ON(SOL_HEADER_ONLY)
	#define SOL_BUILD_I_ SOL_DEFAULT_OFF
#else
	#define SOL_BUILD_I_ SOL_DEFAULT_ON
#endif

#if defined(SOL_UNITY_BUILD)
	#if (SOL_UNITY_BUILD != 0)
		#define SOL_UNITY_BUILD_I_ SOL_ON
	#else
		#define SOL_UNITY_BUILD_I_ SOL_OFF
	#endif
#else
	#define SOL_UNITY_BUILD_I_ SOL_DEFAULT_OFF
#endif // Header only library

#if defined(SOL_C_FUNCTION_LINKAGE)
	#define SOL_C_FUNCTION_LINKAGE_I_ SOL_C_FUNCTION_LINKAGE
#else
	#if SOL_IS_ON(SOL_BUILD_CXX_MODE)
		// C++
		#define SOL_C_FUNCTION_LINKAGE_I_ extern "C"
	#else
		// normal
		#define SOL_C_FUNCTION_LINKAGE_I_
	#endif // C++ or not
#endif // Linkage specification for C functions

#if defined(SOL_API_LINKAGE)
	#define SOL_API_LINKAGE_I_ SOL_API_LINKAGE
#else
	#if SOL_IS_ON(SOL_DLL)
		#if SOL_IS_ON(SOL_COMPILER_VCXX) || SOL_IS_ON(SOL_PLATFORM_WINDOWS) || SOL_IS_ON(SOL_PLATFORM_CYGWIN)
			// MSVC Compiler; or, Windows, or Cygwin platforms
			#if SOL_IS_ON(SOL_BUILD)
				// Building the library
				#if SOL_IS_ON(SOL_COMPILER_GCC)
					// Using GCC
					#define SOL_API_LINKAGE_I_ __attribute__((dllexport))
				#else
					// Using Clang, MSVC, etc...
					#define SOL_API_LINKAGE_I_ __declspec(dllexport)
				#endif
			#else
				#if SOL_IS_ON(SOL_COMPILER_GCC)
					#define SOL_API_LINKAGE_I_ __attribute__((dllimport))
				#else
					#define SOL_API_LINKAGE_I_ __declspec(dllimport)
				#endif
			#endif
		#else
			// extern if building normally on non-MSVC
			#define SOL_API_LINKAGE_I_ extern
		#endif
	#elif SOL_IS_ON(SOL_UNITY_BUILD)
		// Built-in library, like how stb typical works
		#if SOL_IS_ON(SOL_HEADER_ONLY)
			// Header only, so functions are defined "inline"
			#define SOL_API_LINKAGE_I_ inline
		#else
			// Not header only, so seperately compiled files
			#define SOL_API_LINKAGE_I_ extern
		#endif
	#else
		// Normal static library
		#if SOL_IS_ON(SOL_BUILD_CXX_MODE)
			#define SOL_API_LINKAGE_I_
		#else
			#define SOL_API_LINKAGE_I_ extern
		#endif
	#endif // DLL or not
#endif // Build definitions

#if defined(SOL_PUBLIC_FUNC_DECL)
	#define SOL_PUBLIC_FUNC_DECL_I_ SOL_PUBLIC_FUNC_DECL
#else
	#define SOL_PUBLIC_FUNC_DECL_I_ SOL_API_LINKAGE_I_
#endif

#if defined(SOL_INTERNAL_FUNC_DECL_)
	#define SOL_INTERNAL_FUNC_DECL_I_ SOL_INTERNAL_FUNC_DECL_
#else
	#define SOL_INTERNAL_FUNC_DECL_I_ SOL_API_LINKAGE_I_
#endif

#if defined(SOL_PUBLIC_FUNC_DEF)
	#define SOL_PUBLIC_FUNC_DEF_I_ SOL_PUBLIC_FUNC_DEF
#else
	#define SOL_PUBLIC_FUNC_DEF_I_ SOL_API_LINKAGE_I_
#endif

#if defined(SOL_INTERNAL_FUNC_DEF)
	#define SOL_INTERNAL_FUNC_DEF_I_ SOL_INTERNAL_FUNC_DEF
#else
	#define SOL_INTERNAL_FUNC_DEF_I_ SOL_API_LINKAGE_I_
#endif

#if defined(SOL_FUNC_DECL)
	#define SOL_FUNC_DECL_I_ SOL_FUNC_DECL
#elif SOL_IS_ON(SOL_HEADER_ONLY)
	#define SOL_FUNC_DECL_I_ 
#elif SOL_IS_ON(SOL_DLL)
	#if SOL_IS_ON(SOL_COMPILER_VCXX)
		#if SOL_IS_ON(SOL_BUILD)
			#define SOL_FUNC_DECL_I_ extern __declspec(dllexport)
		#else
			#define SOL_FUNC_DECL_I_ extern __declspec(dllimport)
		#endif
	#elif SOL_IS_ON(SOL_COMPILER_GCC) || SOL_IS_ON(SOL_COMPILER_CLANG)
		#define SOL_FUNC_DECL_I_ extern __attribute__((visibility("default")))
	#else
		#define SOL_FUNC_DECL_I_ extern
	#endif
#endif

#if defined(SOL_FUNC_DEFN)
	#define SOL_FUNC_DEFN_I_ SOL_FUNC_DEFN
#elif SOL_IS_ON(SOL_HEADER_ONLY)
	#define SOL_FUNC_DEFN_I_ inline
#elif SOL_IS_ON(SOL_DLL)
	#if SOL_IS_ON(SOL_COMPILER_VCXX)
		#if SOL_IS_ON(SOL_BUILD)
			#define SOL_FUNC_DEFN_I_ __declspec(dllexport)
		#else
			#define SOL_FUNC_DEFN_I_ __declspec(dllimport)
		#endif
	#elif SOL_IS_ON(SOL_COMPILER_GCC) || SOL_IS_ON(SOL_COMPILER_CLANG)
		#define SOL_FUNC_DEFN_I_ __attribute__((visibility("default")))
	#else
		#define SOL_FUNC_DEFN_I_
	#endif
#endif

#if defined(SOL_HIDDEN_FUNC_DECL)
	#define SOL_HIDDEN_FUNC_DECL_I_ SOL_HIDDEN_FUNC_DECL
#elif SOL_IS_ON(SOL_HEADER_ONLY)
	#define SOL_HIDDEN_FUNC_DECL_I_ 
#elif SOL_IS_ON(SOL_DLL)
	#if SOL_IS_ON(SOL_COMPILER_VCXX)
		#if SOL_IS_ON(SOL_BUILD)
			#define SOL_HIDDEN_FUNC_DECL_I_ extern __declspec(dllexport)
		#else
			#define SOL_HIDDEN_FUNC_DECL_I_ extern __declspec(dllimport)
		#endif
	#elif SOL_IS_ON(SOL_COMPILER_GCC) || SOL_IS_ON(SOL_COMPILER_CLANG)
		#define SOL_HIDDEN_FUNC_DECL_I_ extern __attribute__((visibility("default")))
	#else
		#define SOL_HIDDEN_FUNC_DECL_I_ extern
	#endif
#endif

#if defined(SOL_HIDDEN_FUNC_DEFN)
	#define SOL_HIDDEN_FUNC_DEFN_I_ SOL_HIDDEN_FUNC_DEFN
#elif SOL_IS_ON(SOL_HEADER_ONLY)
	#define SOL_HIDDEN_FUNC_DEFN_I_ inline
#elif SOL_IS_ON(SOL_DLL)
	#if SOL_IS_ON(SOL_COMPILER_VCXX)
		#if SOL_IS_ON(SOL_BUILD)
			#define SOL_HIDDEN_FUNC_DEFN_I_ 
		#else
			#define SOL_HIDDEN_FUNC_DEFN_I_ 
		#endif
	#elif SOL_IS_ON(SOL_COMPILER_GCC) || SOL_IS_ON(SOL_COMPILER_CLANG)
		#define SOL_HIDDEN_FUNC_DEFN_I_ __attribute__((visibility("hidden")))
	#else
		#define SOL_HIDDEN_FUNC_DEFN_I_
	#endif
#endif

// end of sol/detail/build_version.hpp

// end of sol/version.hpp

#include <utility>
#include <type_traits>
#include <string_view>

#if SOL_IS_ON(SOL_USE_CXX_LUA) || SOL_IS_ON(SOL_USE_CXX_LUAJIT)
struct lua_State;
#else
extern "C" {
struct lua_State;
}
#endif // C++ Mangling for Lua vs. Not

namespace sol {

	enum class type;

	class stateless_reference;
	template <bool b>
	class basic_reference;
	using reference = basic_reference<false>;
	using main_reference = basic_reference<true>;
	class stateless_stack_reference;
	class stack_reference;

	template <typename A>
	class basic_bytecode;

	struct lua_value;

	struct proxy_base_tag;
	template <typename>
	struct proxy_base;
	template <typename, typename>
	struct table_proxy;

	template <bool, typename>
	class basic_table_core;
	template <bool b>
	using table_core = basic_table_core<b, reference>;
	template <bool b>
	using main_table_core = basic_table_core<b, main_reference>;
	template <bool b>
	using stack_table_core = basic_table_core<b, stack_reference>;
	template <typename base_type>
	using basic_table = basic_table_core<false, base_type>;
	using table = table_core<false>;
	using global_table = table_core<true>;
	using main_table = main_table_core<false>;
	using main_global_table = main_table_core<true>;
	using stack_table = stack_table_core<false>;
	using stack_global_table = stack_table_core<true>;

	template <typename>
	struct basic_lua_table;
	using lua_table = basic_lua_table<reference>;
	using stack_lua_table = basic_lua_table<stack_reference>;

	template <typename T, typename base_type>
	class basic_usertype;
	template <typename T>
	using usertype = basic_usertype<T, reference>;
	template <typename T>
	using stack_usertype = basic_usertype<T, stack_reference>;

	template <typename base_type>
	class basic_metatable;
	using metatable = basic_metatable<reference>;
	using stack_metatable = basic_metatable<stack_reference>;

	template <typename base_t>
	struct basic_environment;
	using environment = basic_environment<reference>;
	using main_environment = basic_environment<main_reference>;
	using stack_environment = basic_environment<stack_reference>;

	template <typename T, bool>
	class basic_function;
	template <typename T, bool, typename H>
	class basic_protected_function;
	using unsafe_function = basic_function<reference, false>;
	using safe_function = basic_protected_function<reference, false, reference>;
	using main_unsafe_function = basic_function<main_reference, false>;
	using main_safe_function = basic_protected_function<main_reference, false, reference>;
	using stack_unsafe_function = basic_function<stack_reference, false>;
	using stack_safe_function = basic_protected_function<stack_reference, false, reference>;
	using stack_aligned_unsafe_function = basic_function<stack_reference, true>;
	using stack_aligned_safe_function = basic_protected_function<stack_reference, true, reference>;
	using protected_function = safe_function;
	using main_protected_function = main_safe_function;
	using stack_protected_function = stack_safe_function;
	using stack_aligned_protected_function = stack_aligned_safe_function;
#if SOL_IS_ON(SOL_SAFE_FUNCTION_OBJECTS)
	using function = protected_function;
	using main_function = main_protected_function;
	using stack_function = stack_protected_function;
	using stack_aligned_function = stack_aligned_safe_function;
#else
	using function = unsafe_function;
	using main_function = main_unsafe_function;
	using stack_function = stack_unsafe_function;
	using stack_aligned_function = stack_aligned_unsafe_function;
#endif
	using stack_aligned_stack_handler_function = basic_protected_function<stack_reference, true, stack_reference>;

	struct unsafe_function_result;
	struct protected_function_result;
	using safe_function_result = protected_function_result;
#if SOL_IS_ON(SOL_SAFE_FUNCTION_OBJECTS)
	using function_result = safe_function_result;
#else
	using function_result = unsafe_function_result;
#endif

	template <typename base_t>
	class basic_object_base;
	template <typename base_t>
	class basic_object;
	template <typename base_t>
	class basic_userdata;
	template <typename base_t>
	class basic_lightuserdata;
	template <typename base_t>
	class basic_coroutine;
	template <typename base_t>
	class basic_packaged_coroutine;
	template <typename base_t>
	class basic_thread;

	using object = basic_object<reference>;
	using userdata = basic_userdata<reference>;
	using lightuserdata = basic_lightuserdata<reference>;
	using thread = basic_thread<reference>;
	using coroutine = basic_coroutine<reference>;
	using packaged_coroutine = basic_packaged_coroutine<reference>;
	using main_object = basic_object<main_reference>;
	using main_userdata = basic_userdata<main_reference>;
	using main_lightuserdata = basic_lightuserdata<main_reference>;
	using main_coroutine = basic_coroutine<main_reference>;
	using stack_object = basic_object<stack_reference>;
	using stack_userdata = basic_userdata<stack_reference>;
	using stack_lightuserdata = basic_lightuserdata<stack_reference>;
	using stack_thread = basic_thread<stack_reference>;
	using stack_coroutine = basic_coroutine<stack_reference>;

	struct stack_proxy_base;
	struct stack_proxy;
	struct variadic_args;
	struct variadic_results;
	struct stack_count;
	struct this_state;
	struct this_main_state;
	struct this_environment;

	class state_view;
	class state;

	template <typename T>
	struct as_table_t;
	template <typename T>
	struct as_container_t;
	template <typename T>
	struct nested;
	template <typename T>
	struct light;
	template <typename T>
	struct user;
	template <typename T>
	struct as_args_t;
	template <typename T>
	struct protect_t;
	template <typename F, typename... Policies>
	struct policy_wrapper;

	template <typename T>
	struct usertype_traits;
	template <typename T>
	struct unique_usertype_traits;

	template <typename... Args>
	struct types {
		typedef std::make_index_sequence<sizeof...(Args)> indices;
		static constexpr std::size_t size() {
			return sizeof...(Args);
		}
	};

	template <typename T>
	struct derive : std::false_type {
		typedef types<> type;
	};

	template <typename T>
	struct base : std::false_type {
		typedef types<> type;
	};

	template <typename T>
	struct weak_derive {
		static bool value;
	};

	template <typename T>
	bool weak_derive<T>::value = false;

	namespace stack {
		struct record;
	}

#if SOL_IS_OFF(SOL_USE_BOOST)
	template <class T>
	class optional;

	template <class T>
	class optional<T&>;
#endif

	using check_handler_type = int(lua_State*, int, type, type, const char*);

} // namespace sol

#define SOL_BASE_CLASSES(T, ...)                       \
	namespace sol {                                   \
		template <>                                  \
		struct base<T> : std::true_type {            \
			typedef ::sol::types<__VA_ARGS__> type; \
		};                                           \
	}                                                 \
	void a_sol3_detail_function_decl_please_no_collide()
#define SOL_DERIVED_CLASSES(T, ...)                    \
	namespace sol {                                   \
		template <>                                  \
		struct derive<T> : std::true_type {          \
			typedef ::sol::types<__VA_ARGS__> type; \
		};                                           \
	}                                                 \
	void a_sol3_detail_function_decl_please_no_collide()

#endif // SOL_FORWARD_HPP
// end of sol/forward.hpp

#endif // SOL_SINGLE_INCLUDE_FORWARD_HPP
