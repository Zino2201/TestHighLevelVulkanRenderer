cb_add_module(core
	public/engine/Core.hpp
	public/engine/Flags.hpp
	public/engine/PlatformMacros.hpp
	public/engine/Hash.hpp
	public/engine/MulticastDelegate.hpp
	public/engine/Result.hpp
	public/engine/debug/Assertions.hpp
	public/engine/logger/Logger.hpp
	public/engine/logger/Sink.hpp
	public/engine/containers/SparseArray.hpp
	public/engine/logger/sinks/StdoutSink.hpp
	public/engine/module/Module.hpp
	public/engine/module/ModuleManager.hpp
	public/engine/util/SimplePool.hpp
	private/engine/logger/Logger.cpp
	private/engine/logger/sinks/StdoutSink.cpp
	private/engine/module/ModuleManager.cpp
	private/engine/Core.cpp)
target_include_directories(core PUBLIC public ${CB_THIRD_PARTY_DIR}/boost PRIVATE private)
target_compile_options(core PUBLIC /GR- /W4)
target_compile_features(core PUBLIC cxx_std_20)
target_compile_definitions(core PUBLIC FMT_EXCEPTIONS=0 _HAS_EXCEPTIONS=0 CB_MODULE_PREFIX="${CB_MODULE_PREFIX}" 
	GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE)
if(CB_MONOLITHIC)
	target_compile_definitions(core PUBLIC CB_MONOLITHIC=1)
else()
	target_compile_definitions(core PUBLIC CB_MONOLITHIC=0)
endif()
target_compile_definitions(core PUBLIC "$<$<CONFIG:Debug>:CB_BUILD_PRIVATE_DEFINITION_DEBUG=1>$<$<CONFIG:RelWithDebInfo>:CB_BUILD_PRIVATE_DEFINITION_RELWITHDEBINFO=1>$<$<CONFIG:Release>:CB_BUILD_PRIVATE_DEFINITION_RELEASE=1>")
target_link_libraries(core PUBLIC robin_hood::robin_hood glm::glm fmt)