#pragma once

#include <spdlog/logger.h>

namespace cb
{

/**
 * spdlog logger with some cool features
 */
class Logger final : public spdlog::logger
{
public:
	explicit Logger(std::string in_name, spdlog::sink_ptr in_sink) : logger(in_name, in_sink) {}
	
	void sink_it_(const spdlog::details::log_msg& msg) override;
};
	
}