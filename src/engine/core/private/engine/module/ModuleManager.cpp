#include "engine/module/ModuleManager.hpp"
#include "engine/module/Module.hpp"
#include <vector>
#if CB_PLATFORM(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace cb
{
	
using InstantiateModuleFunc = Module*(*)();

std::vector<std::unique_ptr<Module>> modules;
	
Result<Module*, LoadModuleResult> load_module_shared(const std::string_view& in_module_name)
{
	const std::string func_name = std::string("cb_detail_instantiate_module_") + in_module_name.data();
	
#if CB_PLATFORM(WINDOWS)
	std::string path = std::string(CB_MODULE_PREFIX) + std::string(in_module_name.data()) + ".dll";
	HMODULE dll = LoadLibraryA(path.c_str());
	if(!dll)
	{
		DWORD err_msg = GetLastError();
		LPSTR buf = nullptr;
		size_t size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM 
				| FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, 
			err_msg, 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			(LPSTR) &buf, 
			0, 
			NULL);

		std::string_view msg(buf, size);
		logger::error("Failed to load module library {}: {}", path, msg);
		LocalFree(buf);

		return make_error(LoadModuleResult::NotFound);
	}

	auto func = reinterpret_cast<InstantiateModuleFunc>(GetProcAddress(dll, func_name.c_str()));
	if(!func)
		return make_error(LoadModuleResult::Invalid);

	Module* module = func();
	if(!module)
		return make_error(LoadModuleResult::Invalid);

	module->prepare_module(in_module_name, dll);

	return make_result(module);
#endif
}
	
Result<Module*, LoadModuleResult> load_module_monolithic(const std::string_view& in_module_name)
{
	(void)(in_module_name);
	return make_error(LoadModuleResult::Invalid);
}
	
Result<Module*, LoadModuleResult> load_module(const std::string_view& in_module_name)
{
	for(const auto& module : modules)
		if(module->get_name() == in_module_name)
			return make_error(LoadModuleResult::AlreadyLoaded);
	
#if CB_MONOLITHIC
#error "unimplemented"
#else
	auto module = load_module_shared(in_module_name);
	if(!module)
		return module.get_error();
#endif

	logger::verbose("Loaded module {}", in_module_name);
	
	modules.emplace_back(module.get_value());
	return module;
}

void unload_modules()
{
	/** Reverse iterate so we unload the latest loaded module */
}
	
}
