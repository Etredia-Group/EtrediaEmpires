#if !APP_VERSION
#include "compile_config.hpp"
#endif

#include "Renderer.hpp"
#include "Logger.h"
#include <allocator.hpp>
#include <exception>
#include <set>

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

	// Create surface
	if ( !SDL_Vulkan_CreateSurface(window, vulkan, &surface) )
		LOG(CRITICAL, "Failed to create SDL Vulkan surface. %hs", SDL_GetError());
	else LOG(INFO, "SDL Vulkan surface created");
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
				queueFamiliesBuffer.graphics = i;

			if (
				queueFamiliesBuffer.graphics != queueFamiliesBuffer.present ||
				queueFamiliesBuffer.graphics == QUEUE_FAMILY_NO_VALUE
			)
			{
				VkBool32 presentSupport = VK_FALSE;
				vkResult = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
				if (vkResult != VK_SUCCESS)
					LOG(WARNING, "Failed to check surface present support. Code: %d", vkResult);
				if (presentSupport)
					queueFamiliesBuffer.present = i;
			}

			i++;
		}

		if (
			queueFamiliesBuffer.graphics == QUEUE_FAMILY_NO_VALUE ||
			queueFamiliesBuffer.present == QUEUE_FAMILY_NO_VALUE
		)
			continue;

		if (queueFamiliesBuffer.graphics == queueFamiliesBuffer.present)
			priority += GPU_GRAPHICS_WITH_PRESENT_PRIORITY;
		
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

	// Define queues
	std::vector<VkDeviceQueueCreateInfo> queuesCreateConfigs;
	std::set<uint32_t> uniqueQueueFamilies = {
		queueFamilies.graphics,
		queueFamilies.present
	};

	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateConfig {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &defaultQueuePriority
		};

		queuesCreateConfigs.push_back(queueCreateConfig);
	}

	// Create logical device
	std::vector<const char*> deviceExtensions = {
		"VK_KHR_swapchain"
	};

	VkDeviceCreateInfo deviceInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.queueCreateInfoCount = static_cast<uint32_t>(queuesCreateConfigs.size()),
		.pQueueCreateInfos = queuesCreateConfigs.data(),
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
	else LOG(INFO, "%hs is selected for render with %d priority score", deviceProperties.deviceName, best_priority);

	// Save queues descriptor
	vkGetDeviceQueue(device, queueFamilies.graphics, 0, &queues.graphics);
	vkGetDeviceQueue(device, queueFamilies.present, 0, &queues.present);
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
	vkDestroySurfaceKHR(vulkan, surface, nullptr);
	vkDestroyInstance(vulkan, nullptr);

    LOG(INFO, "Renderer is released");
    isRunning.store(false);
}