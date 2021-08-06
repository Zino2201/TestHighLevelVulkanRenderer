#include "engine/logger/Logger.hpp"
#include "engine/PlatformMacros.hpp"
#include "engine/logger/Sink.hpp"
#include <chrono>
#include <thread>
#if CB_PLATFORM(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace cb::logger
{

namespace detail
{
std::string severity_to_string(SeverityFlagBits in_severity)
{
	switch(in_severity)
	{
	default:
	case SeverityFlagBits::Verbose:
		return "verbose";
	case SeverityFlagBits::Info:
		return "info";
	case SeverityFlagBits::Warn:
		return "warn";
	case SeverityFlagBits::Error:
		return "error";
	case SeverityFlagBits::Fatal:
		return "fatal";
	}
}
}

std::vector<std::unique_ptr<Sink>> sinks;
std::string pattern;

void add_sink(std::unique_ptr<Sink>&& in_sink)
{
	in_sink->set_pattern(pattern);
	sinks.emplace_back(std::move(in_sink));
}

void set_pattern(const std::string& in_pattern)
{
	pattern = in_pattern;
	for(const auto& sink : sinks)
		sink->set_pattern(pattern);
}

void log(SeverityFlagBits in_severity, const Category& in_category, const std::string& in_message)
{
	Message message;
	message.time = std::chrono::system_clock::now();
	message.thread = std::this_thread::get_id();
	message.severity = in_severity;
	message.category = in_category;
	message.message = in_message;

	for(const auto& sink : sinks)
	{
		sink->log(message);
		if(in_severity == SeverityFlagBits::Fatal)
		{
			MessageBoxA(nullptr, in_message.c_str(), "Fatal Error", MB_OK | MB_ICONERROR);
			std::terminate();
		}

	}
}
	
}