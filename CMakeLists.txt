cmake_minimum_required(VERSION 3.30)
project(woodwork LANGUAGES CXX C)

include(FetchContent)

# Declare versions
set(CMAKE_CXX_STANDARD 23)
set(VULKAN_VER_MAJOR 1)
set(VULKAN_VER_MINOR 3)
set(VULKAN_VER_PATCH 278)
set(VULKAN_VERSION ${VULKAN_VER_MAJOR}.${VULKAN_VER_MINOR}.${VULKAN_VER_PATCH})
set(GLAD_VERSION 2.0.5)
set(GLFW3_VERSION 3.4)

#-------------------------------------------------------------------------
# Fetch GLAD
# ------------------------------------------------------------------------
message(STATUS "Include GLAD")
FetchContent_Declare(
        libglad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v${GLAD_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_GetProperties(libglad)
FetchContent_MakeAvailable(libglad)
set(GLAD_SOURCES_DIR ${libglad_SOURCE_DIR})
add_subdirectory(${libglad_SOURCE_DIR}/cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake)
glad_add_library(glad_vulkan SHARED REPRODUCIBLE LOADER API gl:core=4.6 vulkan=${VULKAN_VER_MAJOR}.${VULKAN_VER_MINOR})

#-------------------------------------------------------------------------
# Fetch GLFW3
# ------------------------------------------------------------------------
message(STATUS "Include GLFW3")
FetchContent_Declare(
        libglfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG ${GLFW3_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

set(GLFW_INSTALL OFF)
set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_COCOA OFF)
set(GLFW_BUILD_WAYLAND OFF)
FetchContent_MakeAvailable(libglfw)

#-------------------------------------------------------------------------
# Fetch Vulkan C Headers
# ------------------------------------------------------------------------
message(STATUS "Include Vulkan C Headers")
FetchContent_Declare(
        vulkanheaders
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG v${VULKAN_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
set(PROJECT_IS_TOP_LEVEL OFF)
FetchContent_MakeAvailable(vulkanheaders) # vulkanheaders has a proper CMakeLists

#-------------------------------------------------------------------------
# Fetch Vulkan C++ Headers
#-------------------------------------------------------------------------
message(STATUS "Include Vulkan C++ Headers")
FetchContent_Declare(
        vulkanhpp
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Hpp.git
        GIT_TAG v${VULKAN_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
# vulkanhpp does not have a proper CMakeLists
FetchContent_GetProperties(vulkanhpp)
FetchContent_MakeAvailable(vulkanhpp)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

# Define a library for Vulkan
add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE ${vulkanhpp_SOURCE_DIR} ${VULKAN_INCLUDE_DIRS})
target_link_libraries(vulkan INTERFACE Vulkan::Headers glfw glad_vulkan ${Vulkan_LIBRARIES} Vulkan::shaderc_combined)

file(GLOB_RECURSE HEADERS ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)


add_executable(${CMAKE_PROJECT_NAME} ${HEADERS} ${SOURCES})
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE include)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE vulkan)
