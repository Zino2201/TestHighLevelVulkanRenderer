#pragma once

namespace cb::gfx
{

enum class QueueType
{
	Gfx,
	Compute,
	Transfer,
	Present
};

struct CommandPoolCreateInfo
{
	QueueType queue_type;

	CommandPoolCreateInfo(QueueType in_queue_type) : queue_type(in_queue_type) {}
};

}