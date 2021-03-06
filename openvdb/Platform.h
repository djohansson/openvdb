///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2019 DreamWorks Animation LLC
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DreamWorks Animation nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////
///
/// @file Platform.h

#ifndef OPENVDB_PLATFORM_HAS_BEEN_INCLUDED
#define OPENVDB_PLATFORM_HAS_BEEN_INCLUDED

#include "PlatformConfig.h"

#define PRAGMA(x) _Pragma(#x)

/// @name Utilities
/// @{
/// @cond OPENVDB_VERSION_INTERNAL
#define OPENVDB_PREPROC_STRINGIFY_(x) #x
/// @endcond
/// @brief Return @a x as a string literal.  If @a x is a macro,
/// return its value as a string literal.
/// @hideinitializer
#define OPENVDB_PREPROC_STRINGIFY(x) OPENVDB_PREPROC_STRINGIFY_(x)

/// @cond OPENVDB_VERSION_INTERNAL
#define OPENVDB_PREPROC_CONCAT_(x, y) x ## y
/// @endcond
/// @brief Form a new token by concatenating two existing tokens.
/// If either token is a macro, concatenate its value.
/// @hideinitializer
#define OPENVDB_PREPROC_CONCAT(x, y) OPENVDB_PREPROC_CONCAT_(x, y)
/// @}


/// Use OPENVDB_DEPRECATED to mark functions as deprecated.
/// It should be placed right before the signature of the function,
/// e.g., "OPENVDB_DEPRECATED void functionName();".
#ifdef OPENVDB_DEPRECATED
#undef OPENVDB_DEPRECATED
#endif
#ifdef _MSC_VER
    #define OPENVDB_DEPRECATED  __declspec(deprecated)
#else
    #define OPENVDB_DEPRECATED  __attribute__ ((deprecated))
#endif

/// Macro for determining if GCC version is >= than X.Y
#if defined(__GNUC__)
    #define OPENVDB_CHECK_GCC(MAJOR, MINOR) \
        (__GNUC__ > MAJOR || (__GNUC__ == MAJOR && __GNUC_MINOR__ >= MINOR))
#else
    #define OPENVDB_CHECK_GCC(MAJOR, MINOR) 0
#endif

/// Macro for determining if there are sufficient C++0x/C++11 features
#ifdef __INTEL_COMPILER
    #ifdef __INTEL_CXX11_MODE__
        #define OPENVDB_HAS_CXX11 1
    #endif
#elif defined(__clang__)
    #ifndef _LIBCPP_VERSION
        #include <ciso646>
    #endif
    #ifdef _LIBCPP_VERSION
        #define OPENVDB_HAS_CXX11 1
    #endif
#elif defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus > 199711L)
    #define OPENVDB_HAS_CXX11 1
#elif defined(_MSC_VER)
    #if (_MSC_VER >= 1700)
        #define OPENVDB_HAS_CXX11 1
    #endif
#endif
#if defined(__GNUC__) && !OPENVDB_CHECK_GCC(4, 4)
    // ICC uses GCC's standard library headers, so even if the ICC version
    // is recent enough for C++11, the GCC version might not be.
    #undef OPENVDB_HAS_CXX11
#endif

/// For compilers that need templated function specializations to have
/// storage qualifiers, we need to declare the specializations as static inline.
/// Otherwise, we'll get linker errors about multiply defined symbols.
#if defined(__GNUC__) && OPENVDB_CHECK_GCC(4, 4)
    #define OPENVDB_STATIC_SPECIALIZATION
#else
    #define OPENVDB_STATIC_SPECIALIZATION static
#endif


/// Bracket code with OPENVDB_NO_UNREACHABLE_CODE_WARNING_BEGIN/_END,
/// as in the following example, to inhibit ICC remarks about unreachable code:
/// @code
/// template<typename NodeType>
/// void processNode(NodeType& node)
/// {
///     OPENVDB_NO_UNREACHABLE_CODE_WARNING_BEGIN
///     if (NodeType::LEVEL == 0) return; // ignore leaf nodes
///     int i = 0;
///     ...
///     OPENVDB_NO_UNREACHABLE_CODE_WARNING_END
/// }
/// @endcode
/// In the above, <tt>NodeType::LEVEL == 0</tt> is a compile-time constant expression,
/// so for some template instantiations, the line below it is unreachable.
#if defined(__INTEL_COMPILER)
    // Disable ICC remarks 111 ("statement is unreachable"), 128 ("loop is not reachable"),
    // 185 ("dynamic initialization in unreachable code"), and 280 ("selector expression
    // is constant").
    #define OPENVDB_NO_UNREACHABLE_CODE_WARNING_BEGIN \
        _Pragma("warning (push)") \
        _Pragma("warning (disable:111)") \
        _Pragma("warning (disable:128)") \
        _Pragma("warning (disable:185)") \
        _Pragma("warning (disable:280)")
    #define OPENVDB_NO_UNREACHABLE_CODE_WARNING_END \
        _Pragma("warning (pop)")
#elif defined(__clang__)
    #define OPENVDB_NO_UNREACHABLE_CODE_WARNING_BEGIN \
        PRAGMA(clang diagnostic push) \
        PRAGMA(clang diagnostic ignored "-Wunreachable-code")
    #define OPENVDB_NO_UNREACHABLE_CODE_WARNING_END \
        PRAGMA(clang diagnostic pop)
#else
    #define OPENVDB_NO_UNREACHABLE_CODE_WARNING_BEGIN
    #define OPENVDB_NO_UNREACHABLE_CODE_WARNING_END
#endif

#include <cstdint>

/// Helper macros for defining library symbol visibility
#ifdef OPENVDB_EXPORT
#undef OPENVDB_EXPORT
#endif
#ifdef OPENVDB_IMPORT
#undef OPENVDB_IMPORT
#endif
#ifdef __GNUC__
    #define OPENVDB_EXPORT __attribute__((visibility("default")))
    #define OPENVDB_IMPORT __attribute__((visibility("default")))
#endif
#ifdef _WIN32
    #ifdef OPENVDB_DLL
        #define OPENVDB_EXPORT __declspec(dllexport)
        #define OPENVDB_IMPORT __declspec(dllimport)
    #else
        #define OPENVDB_EXPORT
        #define OPENVDB_IMPORT
    #endif
#endif

#ifdef OPENVDB_USE_TBB
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_sort.h>
#define OPENVDB_FOR_EACH(func, range) \
	(tbb::parallel_for((range), (func)))
#define OPENVDB_FOR_EACH_PARTITION(func, range, partition) \
	(tbb::parallel_for((range), (func), (partition)))
#define OPENVDB_REDUCE(func, range) \
	(tbb::parallel_reduce((range), (func)))
#define OPENVDB_REDUCE_SEED(func, range, seed) \
	(tbb::parallel_reduce((range), (seed), (func)))
#define OPENVDB_REDUCE_SEED_JOIN(func, range, seed, join) \
	(tbb::parallel_reduce((range), (seed), (func), (join)))
#define OPENVDB_SORT(begin, end) \
	(tbb::parallel_sort((begin), (end)))
#define OPENVDB_SORT_COMPARE(begin, end, compare) \
	(tbb::parallel_sort((begin), (end), (compare)))
#else
#include <algorithm>
#define OPENVDB_FOR_EACH(func, range) \
	((func)(range))
#define OPENVDB_FOR_EACH_PARTITION(func, range, partition) \
	((func)(range))
#define OPENVDB_REDUCE(func, range) \
	((func)(range))
#define OPENVDB_REDUCE_SEED(func, range, seed) \
	((func)((range), (seed)))
#define OPENVDB_REDUCE_SEED_JOIN(func, range, seed, join) \
	((func)((range), (seed)))
#define OPENVDB_SORT(begin, end) \
	(std::sort((begin), (end)))
#define OPENVDB_SORT_COMPARE(begin, end, compare) \
	(std::sort((begin), (end), (compare)))
#endif

/// All classes and public free standing functions must be explicitly marked
/// as \<lib\>_API to be exported. The \<lib\>_PRIVATE macros are defined when
/// building that particular library.
#ifdef OPENVDB_API
#undef OPENVDB_API
#endif
#ifdef OPENVDB_PRIVATE
    #define OPENVDB_API OPENVDB_EXPORT
#else
    #define OPENVDB_API OPENVDB_IMPORT
#endif
#ifdef OPENVDB_HOUDINI_API
#undef OPENVDB_HOUDINI_API
#endif
#ifdef OPENVDB_HOUDINI_PRIVATE
    #define OPENVDB_HOUDINI_API OPENVDB_EXPORT
#else
    #define OPENVDB_HOUDINI_API OPENVDB_IMPORT
#endif

#endif // OPENVDB_PLATFORM_HAS_BEEN_INCLUDED

// Copyright (c) 2012-2019 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
