cmake_minimum_required(VERSION 3.29)
project(woodwork LANGUAGES CXX C)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_STANDARD 23)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")
include(FetchContent)

#set(GLAD_VERSION 2.0.5)
set(GLFW3_VERSION 3.4)

##-------------------------------------------------------------------------
## Fetch GLAD
## ------------------------------------------------------------------------
#message(STATUS "Include GLAD")
#FetchContent_Declare(
#        libglad
#        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
#        GIT_TAG v${GLAD_VERSION}
#        GIT_SHALLOW TRUE
#        GIT_PROGRESS TRUE
#)
#
#FetchContent_GetProperties(libglad)
#FetchContent_MakeAvailable(libglad)
#set(GLAD_SOURCES_DIR ${libglad_SOURCE_DIR})
#add_subdirectory(${libglad_SOURCE_DIR}/cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake)
#glad_add_library(glad_vulkan SHARED REPRODUCIBLE LOADER API gl:core=4.6 vulkan=${VULKAN_VER_MAJOR}.${VULKAN_VER_MINOR})

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


find_package (Vulkan REQUIRED)

# set up Vulkan C++ module
add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_compile_definitions(VulkanCppModule PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)
target_include_directories(VulkanCppModule
        PRIVATE
        "${Vulkan_INCLUDE_DIR}"
)
target_link_libraries(VulkanCppModule
        PUBLIC
        Vulkan::Vulkan
)

set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)


target_sources(VulkanCppModule
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS
        "${Vulkan_INCLUDE_DIR}"
        FILES
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

target_sources(VulkanCppModule
        PRIVATE
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

file(GLOB_RECURSE HEADERS ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)


add_executable(${CMAKE_PROJECT_NAME} ${HEADERS} ${SOURCES})
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE include)
target_link_libraries(${CMAKE_PROJECT_NAME} VulkanCppModule glfw)


