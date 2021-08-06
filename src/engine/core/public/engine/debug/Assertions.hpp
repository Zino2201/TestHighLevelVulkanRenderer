#pragma once

#include "engine/PlatformMacros.hpp"
#include "engine/logger/Logger.hpp"

/** Portable __debugbreak */
#if CB_COMPILER(MSVC)
#define CB_DEBUGBREAK() __debugbreak();
#elif CB_COMPILER(GCC) || CB_COMPILER(CLANG)
#define CB_DEBUGBREAK() __asm volatile ("int $0x3");
#else
#define CB_DEBUGBREAK()
#endif /** CB_COMPILER(MSVC) */

/** Assertions */

/** ASSERT/ASSETF */
#if CB_BUILD(DEBUG)
#define CB_ASSERT(condition) if(!(condition)) { logger::fatal("Assertion failed: {} (File: {}, Line: {})", #condition, __FILE__, __LINE__); CB_DEBUGBREAK(); }
#define CB_ASSERTF(condition, msg, ...) if(!(condition)) { logger::fatal("Assertion failed: {} (File: {}, Line: {})", fmt::format(msg, __VA_ARGS__), __FILE__, __LINE__); CB_DEBUGBREAK(); }
#else 
#define CB_ASSERT(condition) if(!(condition)) { logger::fatal("Assertion failed: {}", #condition); }
#define CB_ASSERTF(condition, msg, ...) if(!(condition)) { logger::fatal("Assertion failed: {} ({})", fmt::format(msg, __VA_ARGS__), #condition); }
#endif

/** CHECK/CHECKF */
#if CB_BUILD(DEBUG)
#define CB_CHECK(condition) if(!(condition)) { logger::error("Check failed: {} (File: {}, Line: {})", #condition, __FILE__, __LINE__); CB_DEBUGBREAK(); }
#define CB_CHECKF(condition, msg, ...) if(!(condition)) { logger::error("{} (File: {}, Line: {})", fmt::format(msg, __VA_ARGS__), __FILE__, __LINE__); CB_DEBUGBREAK(); }
#else
#define CB_CHECK(condition)
#define CB_CHECKF(condition, msg, ...)
#endif