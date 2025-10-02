#include "game.hpp"

#include <fstream>
#include <iostream>

#include "engine/window_glfw.hpp"

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *
) {
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
    }
    return vk::False;
}

Game::Game() {
    constexpr Vec2i windowSize{600, 500};
    window = std::make_unique<WindowGLFW>(windowSize, "Vulkan Setup!");
    initVulkan();
}

Game::~Game() = default;

void Game::start() {
    window->init([this]() {
        queue.waitIdle();

       auto [result, imageIndex] = swapChain.acquireNextImage( UINT64_MAX, *presentCompleteSemaphore, nullptr );
       recordCommandBuffer(imageIndex);

       device.resetFences(  *drawFence );
       vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
       const vk::SubmitInfo submitInfo{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*presentCompleteSemaphore,
                           .pWaitDstStageMask = &waitDestinationStageMask, .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer,
                           .signalSemaphoreCount = 1, .pSignalSemaphores = &*renderFinishedSemaphore };
       queue.submit(submitInfo, *drawFence);
       while ( vk::Result::eTimeout == device.waitForFences( *drawFence, vk::True, UINT64_MAX ) )
           ;

       const vk::PresentInfoKHR presentInfoKHR{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*renderFinishedSemaphore,
                                               .swapchainCount = 1, .pSwapchains = &*swapChain, .pImageIndices = &imageIndex };
       result = queue.presentKHR( presentInfoKHR );
       switch ( result )
       {
           case vk::Result::eSuccess: break;
           case vk::Result::eSuboptimalKHR: std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n"; break;
           default: break;  // an unexpected result is returned!
       }
    });
}

void Game::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
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

    for (const auto &layer: requiredLayers) {
        bool layerCompatible = false;
        for (const auto &layerProperty: context.enumerateInstanceLayerProperties()) {
            if (strcmp(layerProperty.layerName, layer) == 0) {
                layerCompatible = true;
                break;
            }
        }
        if (!layerCompatible) {
            throw std::runtime_error("Layer not supported: " + std::string(layer));
        }
    }

    auto &glfwExtensions = window->getVkExtensions();
    extensions.push_back(vk::EXTDebugUtilsExtensionName);

    extensions.insert(std::end(extensions), std::begin(glfwExtensions), std::end(glfwExtensions));
    const uint32_t extensionCount{static_cast<uint32_t>(extensions.size())};

    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < extensionCount; ++i) {
        bool extensionSupported = false;
        for (auto &[extensionName, specVersion]: extensionProperties) {
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

void Game::createSurface() {
    surface = window->createVKSurface(instance);
}

void Game::pickPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    for (const auto &deviceToCheck: devices) {
        bool supportsVulkan1_4 = deviceToCheck.getProperties().apiVersion >= VK_API_VERSION_1_4;

        bool supportsGraphics = false;
        auto queueFamilies = deviceToCheck.getQueueFamilyProperties();
        for (auto &queueFamily: queueFamilies) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                supportsGraphics = true;
            }
            break;
        }

        auto availableDeviceExtensions = deviceToCheck.enumerateDeviceExtensionProperties();

        bool supportsAllRequiredExtensions = true;

        for (const auto &requiredExtension: deviceExtensions) {
            bool found = false;

            for (const auto &availableExtension: availableDeviceExtensions) {
                if (strcmp(availableExtension.extensionName, requiredExtension) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                supportsAllRequiredExtensions = false;
                break;
            }
        }

        auto features = deviceToCheck.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                        features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().
                                        extendedDynamicState;

        if (supportsVulkan1_4 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures) {
            physicalDevice = deviceToCheck;
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

    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); ++qfpIndex) {
        if (queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
            queueIndex = qfpIndex;
            break;
        }
    }

    if (queueIndex == ~0) {
        throw std::runtime_error("No graphics queue family found!");
    }

    // query for Vulkan 1.3 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
                vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
            featureChain = {
                {}, // vk::PhysicalDeviceFeatures2
                {.dynamicRendering = true}, // vk::PhysicalDeviceVulkan13Features
                {.extendedDynamicState = true} // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
            };

    // create a Device
    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority
    };
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    queue = vk::raii::Queue(device, queueIndex, 0);
}

void Game::createSwapChain() {
    const auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface));
    const vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface = *surface,
        .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(*surface)),
        .clipped = true
    };

    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();
}

void Game::createImageViews() {
    assert(swapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D, .format = swapChainSurfaceFormat.format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };
    for (const auto image: swapChainImages) {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
}

void Game::createGraphicsPipeline() {
    vk::raii::ShaderModule shaderModule = createShaderModule(readFile("assets/shaders/shader.spv"));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise, .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f, .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    std::vector dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0, .pushConstantRangeCount = 0};

    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format
    };
    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2, .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo, .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState, .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling, .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState, .layout = pipelineLayout, .renderPass = nullptr
    };

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

void Game::createCommandPool() {
    const vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueIndex
    };
    commandPool = vk::raii::CommandPool(device, poolInfo);
}

void Game::createCommandBuffer() {
    const vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
}

void Game::recordCommandBuffer(const uint32_t imageIndex) {
    commandBuffer.begin({});
    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {}, // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite, // dstAccessMask
        vk::PipelineStageFlagBits2::eTopOfPipe, // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput // dstStage
    );
    constexpr vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };
    const vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width),
                                              static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRendering();
    // After rendering, transition the swapchain image to PRESENT_SRC
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite, // srcAccessMask
        {}, // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe // dstStage
    );
    commandBuffer.end();
}

void Game::transition_image_layout(
    const uint32_t currentFrame,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const vk::AccessFlags2 src_access_mask,
    const vk::AccessFlags2 dst_access_mask,
    const vk::PipelineStageFlags2 src_stage_mask,
    const vk::PipelineStageFlags2 dst_stage_mask
) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChainImages[currentFrame],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    const vk::DependencyInfo dependency_info = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    commandBuffer.pipelineBarrier2(dependency_info);
}

void Game::createSyncObjects() {
    presentCompleteSemaphore =vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());
    renderFinishedSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());
    drawFence = vk::raii::Fence(device, {.flags = vk::FenceCreateFlagBits::eSignaled});
}


vk::raii::ShaderModule Game::createShaderModule(const std::vector<char> &code) const {
    const vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t *>(code.data())
    };
    vk::raii::ShaderModule shaderModule{device, createInfo};

    return shaderModule;
}


uint32_t Game::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

vk::SurfaceFormatKHR Game::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats) {
    assert(!availableFormats.empty());
    const auto formatIt = std::ranges::find_if(
        availableFormats,
        [](const auto &format) {
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Game::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    assert(
        std::ranges::any_of(availablePresentModes, [](auto presentMode){ return presentMode == vk::PresentModeKHR::eFifo
            ; }));
    return std::ranges::any_of(availablePresentModes,
                               [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; })
               ? vk::PresentModeKHR::eMailbox
               : vk::PresentModeKHR::eFifo;
}

vk::Extent2D Game::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
        return capabilities.currentExtent;
    }

    auto [width, height] = window->getFrameBufferSize();

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

std::vector<char> Game::readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();
    return buffer;
}
