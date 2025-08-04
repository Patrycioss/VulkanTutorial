#include "example.hpp"

#include "engine/window_glfw.hpp"

int main() {
	std::unique_ptr<Window> window = std::make_unique<WindowGLFW>(600,500,"Hi Planet");
	window->init();
	
}