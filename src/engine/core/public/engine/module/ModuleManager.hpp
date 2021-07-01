#pragma once

#include "engine/Result.hpp"
#include <string>

namespace cb
{

class Module;

enum class LoadModuleResult
{
	Success = 0,
	AlreadyLoaded = 1,

	NotFound = -1,
	Invalid = -2,
};

/**
 * Try load the specified module
 */
Result<Module*, LoadModuleResult> load_module(const std::string& in_module_name);

/**
 * Unload modules in the order they were loaded from
 */
void unload_modules();

}