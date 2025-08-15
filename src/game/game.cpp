#include "game.hpp"

#include <iostream>

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
	setupDebugMessenger();
	pickPhysicalDevice();
}

void Game::createInstance() {
	constexpr vk::ApplicationInfo appInfo{
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14,
	};

	requiredLayers.push_back("VK_LAYER_KHRONOS_validation");

	for (const auto& layer : requiredLayers) {
		bool layerCompatible = false;
		for (const auto& layerProperty : context.enumerateInstanceLayerProperties()) {
			if (strcmp(layerProperty.layerName, layer) == 0) {
				layerCompatible = true;
				break;
			}
		}
		if (!layerCompatible) {
			throw std::runtime_error("Layer not supported: " + std::string(layer));
		}
	}

	auto& glfwExtensions = window->getVkExtensions();
	extensions.push_back(vk::EXTDebugUtilsExtensionName);

	extensions.insert(std::end(extensions), std::begin(glfwExtensions), std::end(glfwExtensions));
	const uint32_t extensionCount{static_cast<uint32_t>(extensions.size())};

	auto extensionProperties = context.enumerateInstanceExtensionProperties();
	for (uint32_t i = 0; i < extensionCount; ++i) {
		bool extensionSupported = false;
		for (auto& [extensionName, specVersion] : extensionProperties) {
			if (strcmp(extensionName, extensions[i]) == 0) {
				extensionSupported = true;
				break;
			}
		}
		if (!extensionSupported) {
			throw std::runtime_error("Extension not supported: " + std::string(extensions[i]));
		}
	}

	const vk::InstanceCreateInfo createInfo{
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};
	instance = vk::raii::Instance(context, createInfo);
}

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

void Game::pickPhysicalDevice() {
	auto devices = instance.enumeratePhysicalDevices();
	if (devices.empty()) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	for (const auto& device : devices) {
		bool supportsVulkan1_4 = device.getProperties().apiVersion >= VK_API_VERSION_1_4;

		bool supportsGraphics = false;
		auto queueFamilies = device.getQueueFamilyProperties();
		for (auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			}
			supportsGraphics = true;
			break;
		}

		auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
		bool supportsAllRequiredExtensions =
			std::ranges::all_of(deviceExtensions,
			                    [&availableDeviceExtensions](auto const& requiredDeviceExtension) {
				                    return std::ranges::any_of(availableDeviceExtensions,
				                                               [requiredDeviceExtension](auto const& availableDeviceExtension) {
					                                               return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) ==
						                                               0;
				                                               });
			                    });

		auto features = device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
		                                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
		bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
			features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

		if (supportsVulkan1_4 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == nullptr) {
		printf("failed to find a suitable gpu");
	}
}

void Game::createLogicalDevice() {
	// find the index of the first queue family that supports graphics
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

	// get the first index into queueFamilyProperties which supports graphics
	auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties,
	                                                        [](auto const& qfp) {
		                                                        return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(
			                                                        0);
	                                                        });
	assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");

	auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

	// query for Vulkan 1.3 features
	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
		featureChain = {
			{}, // vk::PhysicalDeviceFeatures2
			{.dynamicRendering = true}, // vk::PhysicalDeviceVulkan13Features
			{.extendedDynamicState = true} // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
		};

	// create a Device
	float queuePriority = 0.0f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
	vk::DeviceCreateInfo deviceCreateInfo{
		.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &deviceQueueCreateInfo,
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data()
	};

	device = vk::raii::Device(physicalDevice, deviceCreateInfo);
	graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
}
