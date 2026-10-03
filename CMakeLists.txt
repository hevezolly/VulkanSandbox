cmake_minimum_required(VERSION 3.20)
project(VulkanSandbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

# Disable examples when building as dependency
set(VULKAN_ENGINE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# Optionally control UI
set(VULKAN_ENGINE_IMGUI_UI ON CACHE BOOL "" FORCE)

add_subdirectory(VulkanEngine)
target_compile_definitions(VulkanEngine PRIVATE THROW_ON_UNDESIRED_SWAPCHAIN)

add_executable(VulkanSandbox src/main.cpp)

# This single line gives you everything:
# - all include dirs (engine + transitive deps)
# - all linked libs (volk, glfw, glslang, glm...)
# - all compile definitions
target_link_libraries(VulkanSandbox PRIVATE VulkanEngine VulkanEngineUI)