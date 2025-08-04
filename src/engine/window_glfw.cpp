#include <vector>
#include <engine/window_glfw.hpp>
#include <vulkan/vulkan.hpp>

WindowGLFW::WindowGLFW(const int width, const int height, std::string title): size(width, height), title(std::move(title)) {
}

WindowGLFW::~WindowGLFW() {
	glfwDestroyWindow(window);
}

void WindowGLFW::init() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto window = glfwCreateWindow(size.x, size.y, "Vulkan 101", nullptr, nullptr);

	auto glfwExtensionCount = 0u;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> glfwExtensionsVector(glfwExtensions, glfwExtensions + glfwExtensionCount);
	glfwExtensionsVector.push_back("VK_EXT_debug_utils");
	glfwExtensionsVector.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
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
