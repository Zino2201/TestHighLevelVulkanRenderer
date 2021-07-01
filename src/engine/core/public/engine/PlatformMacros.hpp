#pragma once

/**
 * Minmal macros
 * https://www.fluentcpp.com/2019/05/28/better-macros-better-flags/
 */

/** So for some reason MSVC defined is broken.. */
#if _MSC_VER >= 1
#define CB_DEFINED(X) X >= 1
#else
#define CB_DEFINED(X) defined(X)
#endif

/** Platforms */
#define CB_PLATFORM_PRIVATE_DEFINITION_WIN32() CB_DEFINED(_WIN32) || CB_DEFINED(__INTELLISENSE__)
#define CB_PLATFORM_PRIVATE_DEFINITION_WIN64() CB_DEFINED(_WIN64) || CB_DEFINED(__INTELLISENSE__)
#define CB_PLATFORM_PRIVATE_DEFINITION_WINDOWS() CB_PLATFORM_PRIVATE_DEFINITION_WIN32() || CB_PLATFORM_PRIVATE_DEFINITION_WIN64() || defined(__INTELLISENSE__)
#define CB_PLATFORM_PRIVATE_DEFINITION_ANDROID() CB_DEFINED(__ANDROID__)
#define CB_PLATFORM_PRIVATE_DEFINITION_LINUX() CB_DEFINED(__linux__) && !CB_PLATFORM_PRIVATE_DEFINITION_ANDROID()
#define CB_PLATFORM_PRIVATE_DEFINITION_OSX() CB_DEFINED(__APPLE__) || CB_DEFINED(__MACH__)
#define CB_PLATFORM_PRIVATE_DEFINITION_FREEBSD() CB_DEFINED(__FREEBSD__)

/** Return 1 if compiling for this platform */
#define CB_PLATFORM(X) CB_PLATFORM_PRIVATE_DEFINITION_##X()

/** Compilers */
#define CB_COMPILER_PRIVATE_DEFINITION_MSVC() CB_DEFINED(_MSC_VER) || CB_DEFINED(__INTELLISENSE__)
#define CB_COMPILER_PRIVATE_DEFINITION_GCC() CB_DEFINED(__GNUC__) && !CB_DEFINED(__llvm__) && !CB_DEFINED(__INTEL_COMPILER)
#define CB_COMPILER_PRIVATE_DEFINITION_CLANG() CB_DEFINED(__clang__) && !CB_DEFINED(__INTELLISENSE__)
#define CB_COMPILER_PRIVATE_DEFINITION_CLANG_CL() CB_COMPILER_PRIVATE_DEFINITION_CLANG() && CB_PLATFORM(WINDOWS)

/** Return 1 if compiling with the specified compiler */
#define CB_COMPILER(X) CB_COMPILER_PRIVATE_DEFINITION_##X()

/** Build types */
#define CB_BUILD(X) CB_DEFINED(CB_BUILD_PRIVATE_DEFINITION_##X)

/** Compile-time features */

/** Return 1 if feature is enabled */
#define CB_FEATURE(X) CB_FEATURE_PRIVATE_DEFINITION_##X()

/** Dll symbol export/import */
#if CB_COMPILER(MSVC) || CB_COMPILER(CLANG_CL)
#define CB_DLLEXPORT __declspec(dllexport)
#define CB_DLLIMPORT __declspec(dllimport)
#else
#define CB_DLLEXPORT
#define CB_DLLIMPORT
#endif /** CB_COMPILER(MSVC) || CB_COMPILER(CLANG_CL) */

/** Compiler specific macros */
#if CB_COMPILER(MSVC)
#define CB_FORCEINLINE __forceinline
#define CB_RESTRICT __restrict
#define CB_USED
#elif CB_COMPILER(GCC) || CB_COMPILER(CLANG)
#define CB_FORCEINLINE __attribute__((always_inline)) inline
#define CB_RESTRICT __restrict
#define CB_USED __attribute__((used))
#else
#define CB_FORCEINLINE inline
#define CB_RESTRICT
#define CB_USED
#endif /** CB_COMPILER(MSVC) */