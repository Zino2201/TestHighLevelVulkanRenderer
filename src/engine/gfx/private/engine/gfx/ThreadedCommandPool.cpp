#include "engine/gfx/BackendDevice.hpp"
#include "engine/gfx/Device.hpp"

namespace cb::gfx::detail
{

void ThreadedCommandPool::reset()
{
	for(auto& [thread, pool] : pools)
		pool.reset();
}

ThreadedCommandPool::Pool::Pool(const QueueType in_type)
{
	auto ret = get_device()->get_backend_device()->create_command_pool(CommandPoolCreateInfo(in_type));
	handle = ret.get_value();
}

ThreadedCommandPool::Pool::~Pool()
{
	get_device()->get_backend_device()->destroy_command_pool(handle);
}

void ThreadedCommandPool::Pool::reset()
{
	get_device()->get_backend_device()->reset_command_pool(handle);
}

CommandListHandle ThreadedCommandPool::Pool::allocate_cmd_list()
{
	auto result = get_device()->get_backend_device()->allocate_command_lists(
		handle,
		1);
	auto cmd_list = command_lists.allocate(*get_device(), result.get_value());
	return Device::cast_resource_ptr<CommandListHandle, CommandList>(cmd_list);
}


}