#include "engine/logger/Logger.hpp"
#include "engine/PlatformMacros.hpp"

#if CB_PLATFORM(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace cb
{

void Logger::sink_it_(const spdlog::details::log_msg& msg)
{
	logger::sink_it_(msg);
	if(msg.level == spdlog::level::critical)
	{
#if CB_PLATFORM(WINDOWS)
		MessageBoxA(nullptr, msg.payload.data(), "Critical Error", MB_OK | MB_ICONERROR);
#endif
	}
}
	
}