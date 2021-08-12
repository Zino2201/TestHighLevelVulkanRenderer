#pragma once

#include <memory>
#include <chrono>
#include <thread>
#include <string_view>
#include "engine/debug/Assertions.hpp"
#include <fmt/format.h>
#include "engine/UnusedParameters.hpp"

namespace cb::logger
{

class Sink;

enum class SeverityFlagBits : uint8_t
{
	Verbose,
	Info,
	Warn,
	Error,
	Fatal
};

struct Category
{
	std::string_view name;

	Category() = default;
	constexpr Category(const std::string_view& in_name) : name(in_name) {}
};

struct Message
{
	std::chrono::system_clock::time_point time;
	std::thread::id thread;
	SeverityFlagBits severity;
	Category category;
	std::string message;
};

void log(SeverityFlagBits in_severity, const Category& in_category, const std::string& in_message);
void add_sink(std::unique_ptr<Sink>&& in_sink);
void set_pattern(const std::string& in_pattern);

namespace detail
{

std::string severity_to_string(SeverityFlagBits in_severity);

inline std::string format_message(const std::string& in_pattern, const Message& in_message)
{
	/** TODO: Optimize this out */
	std::time_t time = std::chrono::system_clock::to_time_t(in_message.time);
	std::tm* localtime = ::localtime(&time);

	std::stringstream ss;
	ss << std::put_time(localtime, "%H:%M:%S");

	return fmt::format(in_pattern, fmt::arg("time", ss.str()),
		fmt::arg("severity", severity_to_string(in_message.severity)),
		fmt::arg("category", in_message.category.name),
		fmt::arg("message", in_message.message));
}

}

#define CB_DEFINE_LOG_CATEGORY(Name) constexpr cb::logger::Category log_##Name(#Name);

CB_DEFINE_LOG_CATEGORY(unknown);

template<typename... Args>
void logf(SeverityFlagBits in_severity, const Category& in_category,
	const std::string_view& in_format, Args&&... in_args)
{
	log(in_severity, in_category, fmt::format(in_format, std::forward<Args>(in_args)...));
}

template<typename... Args>
void verbose(const Category& in_category, const std::string_view& in_format, Args&&... in_args)
{

#if CB_BUILD(DEBUG)
	logf(SeverityFlagBits::Verbose, in_category, in_format, std::forward<Args>(in_args)...);
#else
	UnusedParameters { in_category, in_format, in_args... };
#endif
}

template<typename... Args>
void verbose(const std::string_view& in_format, Args&&... in_args)
{
	verbose(log_unknown, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void info(const Category& in_category, const std::string_view& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Info, in_category, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void info(const std::string_view& in_format, Args&&... in_args)
{
	info(log_unknown, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void warn(const Category& in_category, const std::string_view& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Warn, in_category, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void warn(const std::string_view& in_format, Args&&... in_args)
{
	warn(log_unknown, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void error(const Category& in_category, const std::string_view& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Error, in_category, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void error(const std::string_view& in_format, Args&&... in_args)
{
	error(log_unknown, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void fatal(const Category& in_category, const std::string_view& in_format, Args&&... in_args)
{
	logf(SeverityFlagBits::Fatal, in_category, in_format, std::forward<Args>(in_args)...);
}

template<typename... Args>
void fatal(const std::string_view& in_format, Args&&... in_args)
{
	fatal(log_unknown, in_format, std::forward<Args>(in_args)...);
}

}