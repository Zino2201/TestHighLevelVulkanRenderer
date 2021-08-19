# For DXC I completly removed the build and directly download the prebuilt version because
# It was working but every CMake invocation required a full rebuild)
# And I don't want to manage a seperate build system to build 3rd party libs

CPMAddPackage(
	NAME DirectXShaderCompiler 
	URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.6.2106/dxc_2021_07_01.zip)

file(COPY ${DirectXShaderCompiler_SOURCE_DIR}/bin/x64/dxcompiler.dll DESTINATION ${CB_BIN_DIR})
set(DXC_ROOT_DIR ${DirectXShaderCompiler_SOURCE_DIR} CACHE STRING "" FORCE)
set(DXC_INCLUDE_DIR ${DXC_ROOT_DIR}/inc CACHE STRING "" FORCE)
set(DXC_LIB_DIR ${DXC_ROOT_DIR}/lib/x64 CACHE STRING "" FORCE)
#file(DOWNLOAD https://raw.githubusercontent.com/microsoft/DirectXShaderCompiler/master/include/dxc/Support/dxcapi.use.h 
#	${DXC_INCLUDE_DIR}/dxcapi.use.h)

# Here's old source code
#set(ENABLE_SPIRV_CODEGEN ON CACHE BOOL "" FORCE)
#set(CLANG_ENABLE_ARCMT OFF CACHE BOOL "" FORCE)
#set(CLANG_ENABLE_STATIC_ANALYZER OFF CACHE BOOL "" FORCE)
#set(CLANG_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)
#set(LLVM_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)
#set(HLSL_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)
#set(HLSL_BUILD_DXILCONV OFF CACHE BOOL "" FORCE)
#set(HLSL_SUPPORT_QUERY_GIT_COMMIT_INFO OFF CACHE BOOL "" FORCE)
##set(HLSL_ENABLE_DEBUG_ITERATORS ON CACHE BOOL "" FORCE)
#set(LLVM_TARGETS_TO_BUILD "None" CACHE STRING "" FORCE)
#set(LLVM_INCLUDE_DOCS OFF CACHE BOOL "" FORCE)
#set(LLVM_INCLUDE_EXAMPLES OFF CACHE BOOL "" FORCE)
#set(LIBCLANG_BUILD_STATIC ON CACHE BOOL "" FORCE)
#set(LLVM_OPTIMIZED_TABLEGEN OFF CACHE BOOL "" FORCE)
#set(LLVM_REQUIRES_EH ON CACHE BOOL "" FORCE)
#set(LLVM_APPEND_VC_REV ON CACHE BOOL "" FORCE)
#set(LLVM_ENABLE_RTTI ON CACHE BOOL "" FORCE)
#set(LLVM_ENABLE_EH ON CACHE BOOL "" FORCE)
#set(LLVM_ENABLE_TERMINFO OFF CACHE BOOL "" FORCE)
#set(LLVM_DEFAULT_TARGET_TRIPLE "dxil-ms-dx" CACHE STRING "" FORCE)
#set(CLANG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#set(LLVM_REQUIRES_RTTI ON CACHE BOOL "" FORCE)
#set(CLANG_CL OFF CACHE BOOL "" FORCE)
#set(DXC_BUILD_ARCH "Win32" CACHE STRING "" FORCE)
#set(SPIRV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
#set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
#set(VS_PATH $ENV{VSINSTALLDIR} CACHE STRING "" FORCE)

# DXC's FindDiaSDK.cmake doesn't supports Ninja (because of some ifs CMAKE_GENERATOR) so find required libs earliers
#find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS "${VS_PATH}DIA SDK/lib/amd64")
#CPMAddPackage("gh:microsoft/DirectXShaderCompiler@1.6.2106")

# Strange compile bug..
#target_compile_definitions(dxcompiler PRIVATE SUPPORT_QUERY_GIT_COMMIT_INFO=1)