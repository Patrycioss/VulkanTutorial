#include <functional>
#include <vector>
#include <engine/window_glfw.hpp>
#include <vulkan/vulkan.hpp>


WindowGLFW::WindowGLFW(const Vec2i size, std::string title): size(size), title(std::move(title)) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(size.x, size.y, "Vulkan 101", nullptr, nullptr);
	// Todo: Set glfw input callbacks

	auto glfwExtensionCount = 0u;
	const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	extensions = {glfwExtensions, glfwExtensions + glfwExtensionCount};
}

WindowGLFW::~WindowGLFW() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void WindowGLFW::init(const std::function<void()>& mainLoop) {
	
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		mainLoop();
	}
}

int WindowGLFW::getWidth() {
	return size.x;
}

int WindowGLFW::getHeight() {
	return size.y;
}

Vec2i WindowGLFW::getSize() {
	return size;
}

VkResult WindowGLFW::createVKSurface(const VkInstance& instance, VkSurfaceKHR* surface) const {
	return glfwCreateWindowSurface(instance, window, nullptr, surface);
}

const std::vector<const char*>& WindowGLFW::getVkExtensions() {
	return extensions;
}
