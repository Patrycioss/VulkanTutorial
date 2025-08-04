#pragma once

#include <engine/data/vec2i.hpp>

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

    virtual void init() = 0;

    virtual int getWidth() = 0;
    virtual int getHeight() = 0;
    virtual Vec2i getSize() = 0;
    
private: // Private Methods
};
