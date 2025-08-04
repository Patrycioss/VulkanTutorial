#pragma once

#include <string>

#include <GLFW/glfw3.h>

#include "window.hpp"

class WindowGLFW : public Window
{
public: // Properties
    
private: // Member Variables
    Vec2i size;
    std::string title;
    GLFWwindow* window;
    
public: // Public Functions
    WindowGLFW(int width, int height, std::string title);
    ~WindowGLFW();
    
    WindowGLFW(const WindowGLFW&) = delete;
    WindowGLFW(WindowGLFW&&) = delete;
    WindowGLFW& operator=(const WindowGLFW&) = delete;
    
    void init() override;
    int getWidth() override;
    int getHeight() override;
    Vec2i getSize() override;

private: // Private Methods
};
