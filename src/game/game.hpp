#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "engine/window.hpp"

class Game {
public: // Properties

private: // Member Variables
	Window* window;

	vk::raii::Context context;
	vk::raii::Instance instance = nullptr;
	vk::raii::PhysicalDevice physicalDevice = nullptr;
	vk::raii::Device device = nullptr;
	vk::raii::Queue graphicsQueue = nullptr;

	std::vector<const char*> requiredLayers{};
	std::vector<const char*> extensions{};
	
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

	std::vector<const char*> deviceExtensions = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName
	};


public: // Public Functions
	Game();
	~Game();

	Game(const Game&) = delete;
	Game(Game&&) = delete;
	Game& operator=(const Game&) = delete;

	void start();

private: // Private Methods
	void initVulkan();
	void createInstance();
	void setupDebugMessenger();
	void pickPhysicalDevice();
	void createLogicalDevice();
};
