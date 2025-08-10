#pragma once

#include <functional>
#include <engine/data/vec2i.hpp>
#include <vulkan/vulkan.hpp>

class Window 
{
public: // Properties
    
private: // Member Variables
    
public: // Public Functions
    Window() = default;
    ~Window() = default;
    
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;

    virtual void init(const std::function<void()>&) = 0;

    virtual int getWidth() = 0;
    virtual int getHeight() = 0;
    virtual Vec2i getSize() = 0;

    virtual VkResult createVKSurface(const VkInstance& instance, VkSurfaceKHR* surface) const = 0;
    virtual const std::vector<const char*>& getVkExtensions() = 0;
    
private: // Private Methods
};
