FetchContent_Declare(stb 
	GIT_REPOSITORY https://github.com/nothings/stb.git)
FetchContent_MakeAvailable(stb)
set(STB_SOURCE_DIR ${stb_SOURCE_DIR} CACHE STRING "" FORCE)