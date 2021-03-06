#pragma once

#include "engine/logger/Sink.hpp"

namespace cb::logger
{

class StdoutSink final : public Sink
{
public:
	void log(const Message& in_message) override;
	void set_pattern(const std::string& in_pattern) override { pattern = in_pattern; }
private:
	std::string pattern;
};

}