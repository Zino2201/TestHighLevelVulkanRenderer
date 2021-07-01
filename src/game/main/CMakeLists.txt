add_executable(main Main.cpp)
set_target_properties(main PROPERTIES OUTPUT_NAME CityBuilder)
target_link_libraries(main PRIVATE gfx glfw vulkangfx)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CB_BINS_DIR}")