#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "engine/window.hpp"

class Game {
public: // Properties

private: // Member Variables
	Window* window;

	vk::raii::Context context;
	vk::raii::Instance instance = nullptr;

public: // Public Functions
	Game();
	~Game();

	Game(const Game&) = delete;
	Game(Game&&) = delete;
	Game& operator=(const Game&) = delete;

	void start();

private: // Private Methods
	void initVulkan();
};
