#ifndef APP_VERSION
#include "compile_config.hpp"
#endif

#include "Renderer.hpp"
#include "Logger.h"
#include <allocator.hpp>
#include <algorithm>
#include <set>
#include <stdexcept>

bool operator < (const RenderObject& lhs, const RenderObject& rhs)
{
	if (&lhs.material != &rhs.material)
		return &lhs.material < &rhs.material;
	else return lhs.engineId < rhs.engineId;
}

Renderer::Renderer(GLFWwindow *window, const AssetMaterial &baseMaterial, const size_t &vertexBufferSize) :
	isRunning(false),
	isDrawing(false),
	baseMaterial(baseMaterial),
	VERTEX_BUFFER_SIZE(vertexBufferSize)
{
    this->window = window;
	int width = -1, height = -1;
	glfwGetFramebufferSize(window, &width, &height);
	screenWidth.store(width);
	screenHeight.store(height);
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
	uint32_t extensionsGLFWCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsGLFWCount);
	for (size_t i = 0; i < extensionsGLFWCount; i++)
		vulkanExtensions.push_back(glfwExtensions[i]);

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
	
	vkResult = glfwCreateWindowSurface(vulkan, window, nullptr, &surface);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to create GLFW Vulkan surface. Code: %d", vkResult);
	else LOG(INFO, "GLFW Vulkan surface created");
}

void Renderer::SetupDevice()
{
	// Find physical devices
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

	// Filter and select device
	gpu = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties deviceProperties {};
	VkPhysicalDeviceFeatures deviceFeatures {};
	VkPhysicalDeviceFeatures enabledFeatures {};
	uint32_t best_priority = 0;
	for (const VkPhysicalDevice &physicalDevice : devices)
	{
		uint32_t priority = 1;

		// Check device features and properties
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		if (deviceFeatures.wideLines) enabledFeatures.wideLines = VK_TRUE; else continue;
		if (deviceFeatures.independentBlend) enabledFeatures.independentBlend = VK_TRUE; else continue;

		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		// Check queue families
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
					LOG(WARNING, "Failed to check surface present support. Code: %d. Device: %hs", vkResult, deviceProperties.deviceName);
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
		queueFamiliesBuffer.transfer = queueFamiliesBuffer.graphics;

		// Check surface info
		SurfaceInfo surfaceInfoBuffer {};

		uint32_t surfaceFormatsCount = 0;
		vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice,
			surface,
			&surfaceFormatsCount,
			nullptr
		);
		if (vkResult != VK_SUCCESS)
			LOG(WARNING, "Failed to get surface formats number. Code: %d. Device: %hs", vkResult, deviceProperties.deviceName);
		surfaceInfoBuffer.availableFormats.resize(surfaceFormatsCount);
		vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice,
			surface,
			&surfaceFormatsCount,
			surfaceInfoBuffer.availableFormats.data()
		);
		if (vkResult != VK_SUCCESS)
			LOG(WARNING, "Failed to get surface formats. Code: %d. Device: %hs", vkResult, deviceProperties.deviceName);

		uint32_t presentModesCount = 0;
		vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(
			physicalDevice,
			surface,
			&presentModesCount,
			nullptr
		);
		if (vkResult != VK_SUCCESS)
			LOG(WARNING, "Failed to get surface present modes number. Code: %d. Device: %hs", vkResult, deviceProperties.deviceName);
		surfaceInfoBuffer.availablePresentModes.resize(presentModesCount);
		vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(
			physicalDevice,
			surface,
			&surfaceFormatsCount,
			surfaceInfoBuffer.availablePresentModes.data()
		);
		if (vkResult != VK_SUCCESS)
			LOG(WARNING, "Failed to get surface present modes. Code: %d. Device: %hs", vkResult, deviceProperties.deviceName);

		if (
			surfaceInfoBuffer.availableFormats.empty() ||
			surfaceInfoBuffer.availablePresentModes.empty()
		)
			continue;

		// Rate and select physical device
		if (queueFamiliesBuffer.graphics == queueFamiliesBuffer.present)
			priority += GPU_GRAPHICS_WITH_PRESENT_PRIORITY;
		
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			priority += GPU_DISCRETE_PRIORITY;

		surfaceInfoBuffer.format.format = VK_FORMAT_B8G8R8A8_UNORM;
		for (const VkSurfaceFormatKHR &format : surfaceInfoBuffer.availableFormats)
		{
			surfaceInfoBuffer.format = format;
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				priority += GPU_B8G8R8_SRGB_PRIORITY;
				break;
			}
		}

		surfaceInfoBuffer.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (const VkPresentModeKHR &presentMode : surfaceInfoBuffer.availablePresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
			{
				surfaceInfoBuffer.presentMode = presentMode;
				break;
			}
		}

		if (priority > best_priority)
		{
			best_priority = priority;
			gpu = physicalDevice;
			queueFamilies = queueFamiliesBuffer;
			surfaceInfo = surfaceInfoBuffer;
		}
	}

	if (gpu == VK_NULL_HANDLE)
	{
		LOG(CRITICAL, "Failed to select suitable physical device");
		return;
	}

	// Define queues
	//TODO: transfer is separate queue for async loading
	std::vector<VkDeviceQueueCreateInfo> queuesCreateConfigs;
	std::set<uint32_t> uniqueQueueFamilies {
		queueFamilies.transfer,
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
			.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY
		};

		queuesCreateConfigs.push_back(queueCreateConfig);
	}

	// Create logical device
	std::vector<const char*> deviceExtensions {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo deviceInfo {
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
	vkGetDeviceQueue(device, queueFamilies.transfer, 0, &queues.transfer);
	vkGetDeviceQueue(device, queueFamilies.graphics, 0, &queues.graphics);
	vkGetDeviceQueue(device, queueFamilies.present, 0, &queues.present);
}

void Renderer::SetupSwapchain()
{
	VkResult vkResult;
	vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surfaceInfo.capabilities);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to get surface capabilities. Code: %d", vkResult);

	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	
	// Clamp extent
	surfaceInfo.extent.width = (std::max)(
		surfaceInfo.capabilities.minImageExtent.width,
		(std::min)(surfaceInfo.capabilities.maxImageExtent.width, static_cast<uint32_t>(width))
	);
	surfaceInfo.extent.height = (std::max)(
		surfaceInfo.capabilities.minImageExtent.height,
		(std::min)(surfaceInfo.capabilities.maxImageExtent.height, static_cast<uint32_t>(height))
	);

	uint32_t imagesCount = surfaceInfo.capabilities.minImageCount + 1;
	// Clamp min images count
	if (surfaceInfo.capabilities.maxImageCount > 0 && imagesCount > surfaceInfo.capabilities.maxImageCount)
		imagesCount = surfaceInfo.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR swapchainCreateConfig {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.surface = surface,
		.minImageCount = imagesCount,
		.imageFormat = surfaceInfo.format.format,
		.imageColorSpace = surfaceInfo.format.colorSpace,
		.imageExtent = surfaceInfo.extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = surfaceInfo.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = surfaceInfo.presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	if (queueFamilies.graphics != queueFamilies.present)
	{
		uint32_t queueFamiliesShare[] {queueFamilies.graphics, queueFamilies.present};
		swapchainCreateConfig.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateConfig.queueFamilyIndexCount = 2;
		swapchainCreateConfig.pQueueFamilyIndices = queueFamiliesShare;
	}
	else
		swapchainCreateConfig.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkResult = vkCreateSwapchainKHR(device, &swapchainCreateConfig, nullptr, &swapchain);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to create swapchain. Code: %d", vkResult);
	else LOG(INFO, "Swapchain is created");

	vkResult = vkGetSwapchainImagesKHR(device, swapchain, &imagesCount, nullptr);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to get frames number. Code: %d", vkResult);
	frames.resize(imagesCount);
	vkResult = vkGetSwapchainImagesKHR(device, swapchain, &imagesCount, frames.data());
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to get frames descriptors. Code: %d", vkResult);
	else LOG(INFO, "Frames are ready");

	frameViews.resize(imagesCount);
	for (size_t i = 0; i < frames.size(); i++)
	{
		VkImageViewCreateInfo frameViewConfig {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.image = frames[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = surfaceInfo.format.format,
			.components {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		vkResult = vkCreateImageView(device, &frameViewConfig, nullptr, &frameViews[i]);
		if (vkResult != VK_SUCCESS)
			LOG(CRITICAL, "Failed to create image view for frame. Code: %d", vkResult);
	}
	LOG(INFO, "Frame views are ready.");
}

void Renderer::RecreateSwapchain()
{
	if (screenWidth.load() == 0 || screenHeight.load() == 0)
	{
		isDrawing.store(false);
		return;
	}
	else isDrawing.store(true);

	vkDeviceWaitIdle(device);
	ReleaseSwapchain();

	SetupSwapchain();
	SetupRenderPasses();
	SetupPipelines();
	SetupRenderBuffers();
}

void Renderer::ReleaseSwapchain()
{
	for (VkFramebuffer &frameBuffer : baseFrameBuffers)
	{
		vkDestroyFramebuffer(device, frameBuffer, nullptr);
	}
	vkFreeCommandBuffers(
		device,
		graphicsCommandPool,
		static_cast<uint32_t>(graphicsCommandBuffers.size()),
		graphicsCommandBuffers.data()
	);
	vkFreeCommandBuffers(
		device,
		transferCommandPool,
		static_cast<uint32_t>(transferCommandBuffers.size()),
		transferCommandBuffers.data()
	);

	vkDestroyRenderPass(device, baseRenderPass, nullptr);
	Deallocate(pipelineFactory);

	for (VkImageView &frameView : frameViews)
	{
		vkDestroyImageView(device, frameView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::SetupRenderPasses()
{
	VkAttachmentDescription colorAttachmentConfig {
		.format = surfaceInfo.format.format,
		.samples = VK_SAMPLE_COUNT_1_BIT, // TODO: Multisampling
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentRef {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef
	};

	VkSubpassDependency dependency
    {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};

	VkRenderPassCreateInfo baseRenderPassConfig {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .attachmentCount = 1,
        .pAttachments = &colorAttachmentConfig,
        .subpassCount = 1,
        .pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	
    VkResult vkResult = vkCreateRenderPass(device, &baseRenderPassConfig, nullptr, &baseRenderPass);
    if (vkResult != VK_SUCCESS)
        LOG(CRITICAL, "Failed to create base render pass. Code: %d", vkResult);
}

void Renderer::SetupPipelines()
{
	std::array vertexAttributes = GPU_Vertex::GetAttributes();

	pipelineBinding = {
		.baseMaterial = &baseMaterial,
		.baseRenderPass = &baseRenderPass,
		.device = &device,
		.surfaceExtent = &surfaceInfo.extent,
		
		.vertexBindingConfig = &drawConfig.vertexBindingConfig,
		.vertexAttributesCount = static_cast<uint32_t>(vertexAttributes.size()),
		.vertexAttributesConfig = vertexAttributes.data()
	};

	pipelineFactory = Allocate<PipelineFactory>(pipelineBinding);
}

void Renderer::SetupObjectsBuffers()
{
	VkResult vkResult;

	VkBufferCreateInfo worldVertexBufferConfig {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = sizeof(GPU_Vertex) * VERTEX_BUFFER_SIZE,
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	worldVerticesBinded.size = VERTEX_BUFFER_SIZE;
	vkResult = vkCreateBuffer(device, &worldVertexBufferConfig, nullptr, &worldVertices);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to create buffer for world vertices. Code: %d", vkResult);

	VkBufferCreateInfo stagingWorldVertexBufferConfig {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = sizeof(GPU_Vertex) * VERTEX_BUFFER_SIZE,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	vkResult = vkCreateBuffer(device, &stagingWorldVertexBufferConfig, nullptr, &stagingWorldVertices);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to create buffer for staging world vertices. Code: %d", vkResult);

	VkBufferCreateInfo uiVertexBufferConfig {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = sizeof(GPU_Vertex) * VERTEX_BUFFER_SIZE,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	uiVerticesBinded.size = VERTEX_BUFFER_SIZE;
	vkResult = vkCreateBuffer(device, &uiVertexBufferConfig, nullptr, &uiVertices);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to create buffer for UI vertices. Code: %d", vkResult);

	VkMemoryRequirements worldMemoryRequirements;
	vkGetBufferMemoryRequirements(device, worldVertices, &worldMemoryRequirements);
	VkMemoryRequirements stagingWorldMemoryRequirements;
	vkGetBufferMemoryRequirements(device, stagingWorldVertices, &stagingWorldMemoryRequirements);
	VkMemoryRequirements uiMemoryRequirements;
	vkGetBufferMemoryRequirements(device, uiVertices, &uiMemoryRequirements);

	VkPhysicalDeviceMemoryProperties memoryProps;
	vkGetPhysicalDeviceMemoryProperties(gpu, &memoryProps);
	uint32_t optimizedMemTypeIndex = UINT32_MAX, cpuAccessMemTypeIndex = UINT32_MAX;
	VkMemoryPropertyFlags optimizedMemoryProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkMemoryPropertyFlags cpuAccessMemoryProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	for (uint32_t i = 0; i < memoryProps.memoryTypeCount; i++)
	{
		if (
			optimizedMemTypeIndex == UINT32_MAX &&
			worldMemoryRequirements.memoryTypeBits & (1 << i) &&
			(memoryProps.memoryTypes[i].propertyFlags & optimizedMemoryProps) == optimizedMemoryProps
		)
			optimizedMemTypeIndex = i;
		if (
			cpuAccessMemTypeIndex == UINT32_MAX &&
			stagingWorldMemoryRequirements.memoryTypeBits & (1 << i) &&
			uiMemoryRequirements.memoryTypeBits & (1 << i) &&
			(memoryProps.memoryTypes[i].propertyFlags & cpuAccessMemoryProps) == cpuAccessMemoryProps
		)
			cpuAccessMemTypeIndex = i;
	}
	if (optimizedMemTypeIndex == UINT32_MAX)
		LOG(CRITICAL, "Failed to find GPU memory available for local data. Code: %d", vkResult);
	if (cpuAccessMemTypeIndex == UINT32_MAX)
		LOG(CRITICAL, "Failed to find GPU memory available for transfering data. Code: %d", vkResult);

	VkMemoryAllocateInfo allocateWorldVertices {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = worldMemoryRequirements.size,
		.memoryTypeIndex = optimizedMemTypeIndex
	};
	VkMemoryAllocateInfo allocateStagingWorldVertices {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = stagingWorldMemoryRequirements.size,
		.memoryTypeIndex = cpuAccessMemTypeIndex
	};
	VkMemoryAllocateInfo allocateUiVertices {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = uiMemoryRequirements.size,
		.memoryTypeIndex = cpuAccessMemTypeIndex
	};

	vkResult = vkAllocateMemory(device, &allocateWorldVertices, nullptr, &worldVerticesMemory);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to allocate GPU memory for world vertices. Code: %d", vkResult);
	vkResult = vkAllocateMemory(
		device, &allocateStagingWorldVertices, nullptr, &stagingWorldVerticesMemory);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to allocate GPU memory for staging world vertices. Code: %d", vkResult);
	vkResult = vkAllocateMemory(device, &allocateUiVertices, nullptr, &uiVerticesMemory);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to allocate GPU memory for UI vertices. Code: %d", vkResult);

	vkResult = vkBindBufferMemory(device, worldVertices, worldVerticesMemory, 0);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to bind GPU memory for world vertices. Code: %d", vkResult);
	vkResult = vkBindBufferMemory(device, stagingWorldVertices, stagingWorldVerticesMemory, 0);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to bind GPU memory for staging world vertices. Code: %d", vkResult);
	vkResult = vkBindBufferMemory(device, uiVertices, uiVerticesMemory, 0);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to bind GPU memory for UI vertices. Code: %d", vkResult);

	vkResult = vkMapMemory(
		device,
		stagingWorldVerticesMemory,
		0,
		stagingWorldVertexBufferConfig.size,
		0,
		reinterpret_cast<void**>(&(worldVerticesBinded.data))
	);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to map staging world vertices buffer to CPU");
	vkResult = vkMapMemory(
		device,
		uiVerticesMemory,
		0,
		uiVertexBufferConfig.size,
		0,
		reinterpret_cast<void**>(&uiVerticesBinded.data)
	);
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to map UI buffer vertices to CPU");

	memset(worldVerticesBinded.data, 0, stagingWorldVertexBufferConfig.size);
	memset(uiVerticesBinded.data, 0, uiVertexBufferConfig.size);
}

void Renderer::SetupRenderBuffers()
{
	VkResult vkResult;

	baseFrameBuffers.resize(frameViews.size());
	size_t i = 0;
	for (const VkImageView &frameView : frameViews)
	{
		VkImageView attachments[] = {frameView};

		VkFramebufferCreateInfo frameBufferConfig {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.renderPass = baseRenderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = surfaceInfo.extent.width,
			.height = surfaceInfo.extent.height,
			.layers = 1
		};

		vkResult = vkCreateFramebuffer(device, &frameBufferConfig, nullptr, &baseFrameBuffers[i]);
		if (vkResult != VK_SUCCESS)
			LOG(CRITICAL, "Failed to create framebuffer for base render pass. Code: %d", vkResult);

		i++;
	}

	if (graphicsCommandPool == nullptr)
	{
		VkCommandPoolCreateInfo commandPoolConfig {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilies.graphics,
		};

		vkResult = vkCreateCommandPool(device, &commandPoolConfig, nullptr, &graphicsCommandPool);
		if (vkResult != VK_SUCCESS)
			LOG(CRITICAL, "Failed to create graphics command pool. Code: %d", vkResult);
	}

	graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocateConfig {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = graphicsCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, //TODO: reuse some commands using SECONDARY
		.commandBufferCount = static_cast<uint32_t>(graphicsCommandBuffers.size())
	};
	vkResult = vkAllocateCommandBuffers(device, &allocateConfig, graphicsCommandBuffers.data());
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to allocate graphics command buffers. Code: %d", vkResult);

	if (transferCommandPool == nullptr)
	{
		VkCommandPoolCreateInfo commandPoolConfig {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			.queueFamilyIndex = queueFamilies.transfer,
		};

		vkResult = vkCreateCommandPool(device, &commandPoolConfig, nullptr, &transferCommandPool);
		if (vkResult != VK_SUCCESS)
			LOG(CRITICAL, "Failed to create transfer command pool. Code: %d", vkResult);
	}

	transferCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	allocateConfig = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = transferCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, //TODO: reuse some commands using SECONDARY
		.commandBufferCount = static_cast<uint32_t>(transferCommandBuffers.size())
	};
	vkResult = vkAllocateCommandBuffers(device, &allocateConfig, transferCommandBuffers.data());
	if (vkResult != VK_SUCCESS)
		LOG(CRITICAL, "Failed to allocate transfer command buffers. Code: %d", vkResult);
}

void Renderer::Draw()
{
	if (!isDrawing.load()) return;

	VkResult vkResult;
	uint32_t imageIndex = 0;

	vkResult = vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to wait for fence. Code: %d", vkResult);

	vkResult = vkAcquireNextImageKHR(
		device, swapchain,
		UINT64_MAX,
		imageAvailable[currentFrame],
		VK_NULL_HANDLE,
		&imageIndex
	);
	switch (vkResult)
	{
		case VK_SUCCESS:
			break;

		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			LOG(WARNING, "Swapchain needs to be recreated");
			RecreateSwapchain();
			break;
		
		default:
			LOG(ERROR, "Failed to get next image index. Code: %d", vkResult);
			break;
	}

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	vkResult = vkResetCommandBuffer(graphicsCommandBuffers[currentFrame], 0);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to reset command buffer. Code: %d", vkResult);
	vkResult = vkBeginCommandBuffer(graphicsCommandBuffers[currentFrame], &drawConfig.commandBeginConfig);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to begin command buffer. Code: %d", vkResult);

	drawConfig.renderPassBeginConfig.renderPass = baseRenderPass;
	drawConfig.renderPassBeginConfig.framebuffer = baseFrameBuffers[imageIndex];
	drawConfig.renderPassBeginConfig.renderArea = {
		.offset = {.x = 0, .y = 0},
		.extent = surfaceInfo.extent
	};

	vkCmdBeginRenderPass(
		graphicsCommandBuffers[currentFrame],
		&drawConfig.renderPassBeginConfig,
		VK_SUBPASS_CONTENTS_INLINE
	);

	Pipeline* lastPipeline = nullptr;
	Pipeline* pipeline = nullptr;
	for (const RenderObject &object : objects)
	{
		pipeline = &pipelineFactory->Get(object.material);
		if (pipeline != lastPipeline)
		{
			vkCmdBindPipeline(
				graphicsCommandBuffers[currentFrame],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline->instance
			);
			vkCmdSetViewport(graphicsCommandBuffers[currentFrame], 0, 1, &pipeline->viewport);
			vkCmdSetScissor(graphicsCommandBuffers[currentFrame], 0, 1, &pipeline->scissors);
			lastPipeline = pipeline;
		}

		std::vector<VkBuffer> vertexBuffers;
		vertexBuffers.reserve(1);
		if (object.isUI)
			vertexBuffers.push_back(uiVertices);
		else
			vertexBuffers.push_back(worldVertices);
		VkDeviceSize offsets[] = {object.mesh.verticesBufferPosition};
		vkCmdBindVertexBuffers(
			graphicsCommandBuffers[currentFrame],
			0,
			1,
			vertexBuffers.data(),
			offsets
		);

		vkCmdDraw(
			graphicsCommandBuffers[currentFrame],
			static_cast<uint32_t>(object.mesh.vertices.size()),
			static_cast<uint32_t>(object.transforms.size()),
			0, // TODO: batching
			0 // TODO: batching
		);
	}

	vkCmdEndRenderPass(graphicsCommandBuffers[currentFrame]);
	vkResult = vkEndCommandBuffer(graphicsCommandBuffers[currentFrame]);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to end command buffer. Code: %d", vkResult);

	VkSemaphore waitSemaphores[] = {imageAvailable[currentFrame]};
	VkSemaphore signalSemaphores[] = {renderFinished[currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	//drawConfig.submitConfig.pWaitSemaphores = &imageAvailable[currentFrame];
	drawConfig.submitConfig.pWaitSemaphores = waitSemaphores;
	//drawConfig.submitConfig.pSignalSemaphores = &renderFinished[currentFrame];
	drawConfig.submitConfig.pSignalSemaphores =signalSemaphores;
	drawConfig.submitConfig.pCommandBuffers = &graphicsCommandBuffers[currentFrame];
	drawConfig.submitConfig.pWaitDstStageMask = waitStages;

	vkResult = vkResetFences(device, 1, &inFlightFences[currentFrame]);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to reset for fence. Code: %d", vkResult);

	vkResult = vkQueueSubmit(queues.graphics, 1, &drawConfig.submitConfig, inFlightFences[currentFrame]);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to submit render commands. Code: %d", vkResult);

	//drawConfig.presentConfig.pWaitSemaphores = &renderFinished[currentFrame];
	drawConfig.presentConfig.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = {swapchain};
	//drawConfig.presentConfig.pSwapchains = &swapchain;
	drawConfig.presentConfig.pSwapchains = swapChains;
	drawConfig.presentConfig.pImageIndices = &imageIndex;
	vkResult = vkQueuePresentKHR(queues.present, &drawConfig.presentConfig);
	switch (vkResult)
	{
		case VK_SUCCESS:
			break;

		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			LOG(WARNING, "Swapchain needs to be recreated");
			RecreateSwapchain();
			break;
		
		default:
			LOG(ERROR, "Failed to present swapchain. Code: %d", vkResult);
			break;
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::CopyBuffer(VkBuffer src, size_t offset, VkBuffer dst, VkDeviceSize size)
{
	VkResult vkResult;

	VkCommandBufferAllocateInfo bufferAllocateConfig {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = transferCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

    VkCommandBuffer commandBuffer;
    vkResult = vkAllocateCommandBuffers(device, &bufferAllocateConfig, &commandBuffer);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to allocate temporary command buffer for copy operation. Code: %d", vkResult);

	VkCommandBufferBeginInfo commandBeginConfig {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	vkResult = vkBeginCommandBuffer(commandBuffer, &commandBeginConfig);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to begin command for copy operation. Code: %d", vkResult);

	VkBufferCopy copyRegion {
		.srcOffset = offset,
		.dstOffset = offset,
		.size = size
	};
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	vkResult = vkEndCommandBuffer(commandBuffer);
	if (vkResult != VK_SUCCESS)
		LOG(ERROR, "Failed to end command for copy operation. Code: %d", vkResult);

	VkSubmitInfo submitConfig {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};
	vkQueueSubmit(queues.transfer, 1, &submitConfig, VK_NULL_HANDLE); //TODO: use fence for optimizatoin
	vkQueueWaitIdle(queues.transfer);

	vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);
}

void Renderer::SetupSync()
{
	VkResult vkResult;

	VkSemaphoreCreateInfo semaphoreConfig {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr
	};
	VkFenceCreateInfo fenceConfig {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	imageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinished.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(frameViews.size(), VK_NULL_HANDLE);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkResult = vkCreateSemaphore(device, &semaphoreConfig, nullptr, &imageAvailable[i]);
		if (vkResult != VK_SUCCESS)
			LOG(CRITICAL, "Failed to create semaphore. Code: %d", vkResult);
		vkResult = vkCreateSemaphore(device, &semaphoreConfig, nullptr, &renderFinished[i]);
		if (vkResult != VK_SUCCESS)
			LOG(CRITICAL, "Failed to create semaphore. Code: %d", vkResult);
		vkResult = vkCreateFence(device, &fenceConfig, nullptr, &inFlightFences[i]);
		if (vkResult != VK_SUCCESS)
			LOG(CRITICAL, "Failed to create fence. Code: %d", vkResult);
	}
}

void Renderer::Start()
{
	if (isRunning)
	{
		LOG(CRITICAL, "Renderer can be started only once");
		throw std::logic_error("Renderer can be started only once");
	}

    isRunning.store(true);
	isDrawing.store(true);
    renderThread = std::jthread([this](std::stop_token stop) {
        this->stoken = &stop;
        Threading();
    });
}

void Renderer::Threading()
{
    Logger::trace->Register_Thread(L"Renderer", 0);
    LOG(INFO, "Renderer thread started");

	Init();
    Loop();
    Release();
}

void Renderer::Init()
{
	InitVulkan();
	SetupDevice();
	SetupSwapchain();
	SetupRenderPasses();
	SetupPipelines();
	SetupObjectsBuffers();
	SetupRenderBuffers();
	CopyBuffer(stagingWorldVertices, 0, worldVertices, worldVerticesBinded.size * worldVerticesBinded.ELEMENT_SIZE);
	SetupSync();
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

		Draw();
    }
	vkDeviceWaitIdle(device);
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

		case RenderCommand::REGISTER_OBJECTS:
		{
			LOG(INFO, "REGISTER_OBJECTS command");

			for (size_t i = 0; i < command.objectsSize; i++)
			{
				RenderObject &object = command.objects[i];
				object.engineId = static_cast<uint32_t>(objects.size());
				if (objects.contains(object)) continue;

				// TODO: combine vertices data and write with one Write out of loop
				size_t verticesPos = worldVerticesBinded.Write(
					object.mesh.vertices.data(),
					object.mesh.vertices.size()
				) * worldVerticesBinded.ELEMENT_SIZE;
				object.mesh.verticesBufferPosition = verticesPos;
				CopyBuffer(
					stagingWorldVertices,
					verticesPos,
					worldVertices,
					object.mesh.vertices.size() * worldVerticesBinded.ELEMENT_SIZE
				);
				objects.insert(object);
			}

			break;
		}
		
		case RenderCommand::RESIZE_WINDOW:
		{
			LOG(INFO, "RESIZE_WINDOW command");

			RecreateSwapchain();

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

void Renderer::Request(const RenderCommand::Type &type)
{
	commands.push(RenderCommand {.type = type});
}

void Renderer::RegisterObjects(RenderObject* objects, size_t size)
{
	RenderCommand command {
		.type = RenderCommand::REGISTER_OBJECTS,
		.objects = objects,
		.objectsSize = size
	};
	Request(command);
}

void Renderer::ResizeWindow(int width, int height)
{
	screenWidth.store(width);
	screenHeight.store(height);
	Request(RenderCommand::RESIZE_WINDOW);
}

void Renderer::Release()
{
	vkDeviceWaitIdle(device);
	
	ReleaseSwapchain();

	vkUnmapMemory(device, stagingWorldVerticesMemory);
	vkUnmapMemory(device, uiVerticesMemory);

	vkDestroyBuffer(device, worldVertices, nullptr);
	vkDestroyBuffer(device, stagingWorldVertices, nullptr);
	vkDestroyBuffer(device, uiVertices, nullptr);

	vkFreeMemory(device, worldVerticesMemory, nullptr);
	vkFreeMemory(device, stagingWorldVerticesMemory, nullptr);
	vkFreeMemory(device, uiVerticesMemory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, renderFinished[i], nullptr);
		vkDestroySemaphore(device, imageAvailable[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
	vkDestroyCommandPool(device, transferCommandPool, nullptr);

	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(vulkan, surface, nullptr);
	vkDestroyInstance(vulkan, nullptr);

    LOG(INFO, "Renderer is released");
    isRunning.store(false);
}

bool Renderer::GetThreadStatus() { return isRunning.load(); }
bool Renderer::GetDrawStatus() { return isDrawing.load(); }