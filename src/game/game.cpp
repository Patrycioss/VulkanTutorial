#include "game.hpp"

#ifdef WW_DEBUG
#include <iostream>
#endif

#include "engine/window_glfw.hpp"

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
	const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
	const vk::DebugUtilsMessageTypeFlagsEXT type,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*
) {
	if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
		std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
	}
	return vk::False;
}

Game::Game() {
	constexpr Vec2i windowSize{600, 500};
	window = new WindowGLFW(windowSize, "Vulkan Setup!");
	initVulkan();
}

Game::~Game() {
	delete window;
}

void Game::start() {
	window->init([this]() {
	});
}

void Game::initVulkan() {
	createInstance();
#ifdef WW_DEBUG
	setupDebugMessenger();
#endif
}

void Game::createInstance() {
	constexpr vk::ApplicationInfo appInfo{
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14,
	};

	std::vector<const char*> requiredLayers;
#ifdef WW_DEBUG
	requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

	for (const auto& layer : requiredLayers) {
		bool layerCompatible = false;
		for (const auto& layerProperty : context.enumerateInstanceLayerProperties()) {
			if (strcmp(layerProperty.layerName, layer) == 0) {
				layerCompatible = true;
				break;
			}
		}
		if (!layerCompatible) {
#ifdef WW_DEBUG
			throw std::runtime_error("Layer not supported: " + std::string(layer));
#else
			exit(1);
#endif
		}
	}

	std::vector<const char*> extensions{};

#ifdef WW_DEBUG
	extensions.push_back(vk::EXTDebugUtilsExtensionName);
#endif

	auto& glfwExtensions = window->getVkExtensions();

	extensions.insert(std::end(extensions), std::begin(glfwExtensions), std::end(glfwExtensions));
	const uint32_t extensionCount{static_cast<uint32_t>(extensions.size())};

	auto extensionProperties = context.enumerateInstanceExtensionProperties();
	for (uint32_t i = 0; i < extensionCount; ++i) {
		bool extensionSupported = false;
		for (size_t j = 0; j < extensionProperties.size(); ++j) {
			if (strcmp(extensionProperties[j].extensionName, extensions[i]) == 0) {
				extensionSupported = true;
				break;
			}
		}
		if (!extensionSupported) {
#ifdef WW_DEBUG
			throw std::runtime_error("Extension not supported: " + std::string(extensions[i]));
#else
			exit(1);
#endif
		}
	}

	const vk::InstanceCreateInfo instanceCreateInfo{
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = extensions.data(),
	};

	instance = vk::raii::Instance(context, instanceCreateInfo);
}

#ifdef WW_DEBUG
void Game::setupDebugMessenger() {
	constexpr vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
	constexpr vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
	constexpr vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
		.messageSeverity = severityFlags,
		.messageType = messageTypeFlags,
		.pfnUserCallback = &debugCallback
	};
	debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}
#endif
