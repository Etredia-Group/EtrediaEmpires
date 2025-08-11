#if !APP_VERSION
#include "compile_config.hpp"
#endif

#include "Renderer.hpp"
#include "Logger.h"
#include <allocator.hpp>
#include <exception>

Renderer::Renderer(SDL_Window *window) : isRunning(false)
{
    this->window = window;
}

Renderer::~Renderer()
{
	Release();
}

void Renderer::InitVulkan()
{
	std::vector<const char*> vulkanExtensions {
		//
	};
	std::vector<const char*> vulkanLayers {
		VALIDATION_LAYER
	};

    // Get Vulkan extensions list
	uint32_t extensionsSDLCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions(window, &extensionsSDLCount, nullptr) )
		LOG(CRITICAL, "Fail to get SDL Vulkan extensions number! %hs", SDL_GetError());

	std::vector<const char*> vulkanExtensionsSDL(extensionsSDLCount);
	if ( !SDL_Vulkan_GetInstanceExtensions(window, &extensionsSDLCount, vulkanExtensionsSDL.data()) )
	{
		LOG(CRITICAL, "Fail to register SDL Vulkan extensions! %hs", SDL_GetError());
		vulkanExtensionsSDL.clear();
	}
	else vulkanExtensions.insert(vulkanExtensions.end(), vulkanExtensionsSDL.begin(), vulkanExtensionsSDL.end());

	for (const char *extension : vulkanExtensions)
	{
		LOG(INFO, "%hs Vulkan extension registered", extension);
	}

	for (const char *layer : vulkanLayers)
	{
		LOG(INFO, "%hs Vulkan layer registered", layer);
	}

	// Create VkInstance
	VkApplicationInfo vkAppConfig {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Etredia Empires",
		.applicationVersion = VK_MAKE_VERSION(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	VkInstanceCreateInfo vkCreateConfig {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.pApplicationInfo = &vkAppConfig,
		.enabledLayerCount = static_cast<uint32_t>(vulkanLayers.size()),
		.ppEnabledLayerNames = vulkanLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(vulkanExtensions.size()),
		.ppEnabledExtensionNames = vulkanExtensions.data(),
	};

	VkResult vkResult = vkCreateInstance(&vkCreateConfig, nullptr, &vulkan);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Vulkan is not instantiated! Code: %d", vkResult);
	else LOG(INFO, "Vulkan is instantiated");
}

void Renderer::SetupDevice()
{
	// Select physical device and queue families
	VkResult vkResult;
	uint32_t devicesCount = 0;
	vkResult = vkEnumeratePhysicalDevices(vulkan, &devicesCount, nullptr);
	if (vkResult != VK_SUCCESS)
	{
		LOG(CRITICAL, "Failed to enumerate physical devices. Code: %d", vkResult);
		return;
	}

	if (devicesCount == 0)
	{
		LOG(CRITICAL, "No devices with Vulkan support found!");
		return;
	}

	std::vector<VkPhysicalDevice> devices(devicesCount);
	vkResult = vkEnumeratePhysicalDevices(vulkan, &devicesCount, devices.data());
	if (vkResult != VK_SUCCESS)
	{
		LOG(CRITICAL, "Failed to enumerate %d physical devices. Code: %d", devicesCount, vkResult);
		return;
	}
	else
	{
		LOG(INFO, "Found %d Vulkan compatible physical devices.", devices.size());
	}

	gpu = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties deviceProperties {};
	VkPhysicalDeviceFeatures deviceFeatures {};
	VkPhysicalDeviceFeatures enabledFeatures {};
	uint32_t best_priority = 0;
	for (const VkPhysicalDevice &physicalDevice : devices)
	{
		uint32_t priority = 1;

		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		if (deviceFeatures.wideLines) enabledFeatures.wideLines = VK_TRUE; else continue;
		if (deviceFeatures.independentBlend) enabledFeatures.independentBlend = VK_TRUE; else continue;

		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		uint32_t queueFamiliesCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(
			physicalDevice,
			&queueFamiliesCount,
			nullptr
		);
		std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(
			physicalDevice,
			&queueFamiliesCount,
			queueFamiliesProperties.data()
		);

		uint32_t i = 0;
		QueueFamiliesBundle queueFamiliesBuffer {};
		for (const VkQueueFamilyProperties &familyProps : queueFamiliesProperties)
		{
			if (familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queueFamiliesBuffer.graphics = i;
				queueFamiliesBuffer.presence = i;
			}

			//TODO: if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, value))

			i++;
		}

		if (
			!queueFamiliesBuffer.graphics.has_value() ||
			!queueFamiliesBuffer.presence.has_value()
		)
			continue;

		
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			priority += GPU_DISCRETE_PRIORITY;

		if (priority > best_priority)
		{
			best_priority = priority;
			gpu = physicalDevice;
			queueFamilies = queueFamiliesBuffer;
		}
	}

	if (gpu == VK_NULL_HANDLE)
	{
		LOG(CRITICAL, "Failed to select suitable physical device");
		return;
	}

	// Create queues
	VkDeviceQueueCreateInfo graphicsQueueCreateConfig {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.queueFamilyIndex = queueFamilies.graphics.value(),
		.queueCount = 1,
		.pQueuePriorities = &defaultQueuePriority
	};

	// Create logical device
	std::vector<const char*> deviceExtensions = {
		"VK_KHR_swapchain"
	};

	VkDeviceCreateInfo deviceInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &graphicsQueueCreateConfig,
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &enabledFeatures,
	};

	vkResult = vkCreateDevice(gpu, &deviceInfo, nullptr, &device);
	if (vkResult != VK_SUCCESS)
	{
		LOG(CRITICAL, "Failed to create logical device on %hs. Code: %d", deviceProperties.deviceName, vkResult);
		return;
	}
	else LOG(INFO, "%hs is selected for render", deviceProperties.deviceName);
}

void Renderer::Start()
{
	if (isRunning) throw std::exception("Renderer can be started only once");

    isRunning = true;
    renderThread = std::jthread([this](std::stop_token stop) {
        this->stoken = &stop;
        Threading();
    });
}

void Renderer::Threading()
{
    Logger::trace->Register_Thread(L"Renderer", 0);
    LOG(INFO, "Renderer thread started");
	
	InitVulkan();
	SetupDevice();

    Loop();

    Release();
}

void Renderer::Loop()
{
	RenderCommand commandBuffer;
    while (!stoken->stop_requested())
    {
		while (commands.pop(commandBuffer))
		{
			ProcessCommand(commandBuffer);
		}

		
    }
}

void Renderer::ProcessCommand(const RenderCommand &command)
{
	switch(command.type)
	{
		case RenderCommand::SHUTDOWN:
		{
			LOG(INFO, "SHUTDOWN command");
			Shutdown();
			break;
		}
		
		case RenderCommand::RESIZE_WINDOW:
		{
			LOG(INFO, "RESIZE_WINDOW command");
			break;
		}

		default:
		{
			LOG(WARNING, "Unhandled command. Type id: %d", command.type);
			break;
		}
	}
}

void Renderer::Shutdown()
{
    renderThread.request_stop();
}

void Renderer::Request(const RenderCommand &command)
{
	commands.push(command);
}

void Renderer::Request(RenderCommand::Type type)
{
	commands.push(RenderCommand {.type = type});
}

void Renderer::Release()
{
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(vulkan, nullptr);

    LOG(INFO, "Renderer is released");
    isRunning.store(false);
}