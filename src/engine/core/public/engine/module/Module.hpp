#pragma once

#include <string_view>

namespace cb
{

/**
 * Base class for modules
 */
class Module
{
public:
	Module() : os_handle(nullptr), name(nullptr) {}
	virtual ~Module() = default;

	void prepare_module(const std::string_view& in_name,
		void* in_os_handle)
	{
		name = in_name;
		os_handle = in_os_handle;
	}

	[[nodiscard]] const std::string& get_name() const { return name; }
private:
	void* os_handle;
	std::string name;
};

#define CB_DEFINE_MODULE(ModuleClass, ModuleName) \
	extern "C" void cb_detail_instantiate_module_##ModuleName() \
	{ \
		ModuleClass* module = new ModuleClass; \
		return new ModuleClass; \
	} 
	
}