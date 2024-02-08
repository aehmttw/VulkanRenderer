cmake_minimum_required(VERSION 3.23)
project(vulkan)

set(CMAKE_CXX_STANDARD 23)
include_directories( "~/VulkanSDK/1.3.268.1/macOS/include", "/opt/homebrew/include" )

add_executable(vulkan main.cpp triangle.cpp vecmath.hpp vecmathtest.cpp json.hpp jsontest.cpp)
