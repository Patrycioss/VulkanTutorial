#pragma once

#include <string>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "window.hpp"

class WindowGLFW : public Window
{
public: // Properties
    
private: // Member Variables
    Vec2i size;
    std::string title;
    GLFWwindow* window{};
    std::vector<const char*> extensions{};
    
public: // Public Functions
    WindowGLFW(Vec2i size, std::string title);
    ~WindowGLFW();
    
    WindowGLFW(const WindowGLFW&) = delete;
    WindowGLFW(WindowGLFW&&) = delete;
    WindowGLFW& operator=(const WindowGLFW&) = delete;
    
    void init(const std::function<void()>& mainLoop) override;
    int getWidth() override;
    int getHeight() override;
    Vec2i getSize() override;

    VkResult createVKSurface(const VkInstance& instance, VkSurfaceKHR* surface) const override;
    const std::vector<const char*>& getVkExtensions() override;

private: // Private Methods
};
