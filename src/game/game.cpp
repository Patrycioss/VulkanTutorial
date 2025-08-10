#include "game.hpp"

#include <concepts>

#include "engine/window_glfw.hpp"


Game::Game() {
	constexpr Vec2i windowSize{600,500};
	window = new WindowGLFW(windowSize, "Vulkan Setup!");
}

Game::~Game() {
	delete window;
}

void Game::start() {
	window->init([this]() {
		
	});
}

void Game::initVulkan() {
	constexpr vk::ApplicationInfo appInfo{
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14,
	};

	std::vector<const char*> extensions{};

	auto& glfwExtensions = window->getVkExtensions();
	
	extensions.insert(std::end(extensions), std::begin(glfwExtensions), std::end(glfwExtensions));
	const uint32_t extensionCount{static_cast<uint32_t>(glfwExtensions.size())};

	auto extensionProperties = context.enumerateInstanceExtensionProperties();
	for (uint32_t i = 0; i < extensionCount; ++i)
	{
		if (std::ranges::none_of(extensionProperties,
								 [extension = extensions[i]](auto const& extensionProperty)
								 { return strcmp(extensionProperty.extensionName, extension) == 0; }))
		{
			throw std::runtime_error("Required extension not supported: " + std::string(glfwExtensions[i]));
		}
	}

	const vk::InstanceCreateInfo instanceCreateInfo{
		.pApplicationInfo =  &appInfo,
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = glfwExtensions.data(),
	};

	instance = vk::raii::Instance(context, instanceCreateInfo);
	
}
