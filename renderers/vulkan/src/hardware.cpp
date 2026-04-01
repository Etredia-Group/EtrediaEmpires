module vulkan_render;
import utils;

template <typename T> using iterator = std::ranges::borrowed_iterator_t<std::vector<T>&>;

vulkan_render::Context::Context(const char *appName)
    : vulkan(MakeInstance(appName, {}, {})) {}
vulkan_render::Context::Context(const vk::InstanceCreateInfo &instanceInfo)
    : vulkan(vk::raii::Instance(context, instanceInfo)) {}

vulkan_render::Context::Context(
    const char *appName,
    const std::vector<const char*> &extensions,
    const std::vector<const char*> &layers
)
    : vulkan(MakeInstance(appName, extensions, layers)) {}

void vulkan_render::Context::SetupPhysicalDevice(
    const Surface &surface,
    const PhysicalDevicePriorities &priorities,
    vk::SurfaceFormatKHR& selectedFormat,
    vk::PresentModeKHR& selectedPresentMode
) {
    // Find physical devices
    const std::vector<vk::raii::PhysicalDevice> devices = vulkan.enumeratePhysicalDevices();
    if (devices.size() == 0) {
        LOG(CRITICAL, "No devices with Vulkan support found!");
        return;
    }
    LOG(INFO, "Found %zu Vulkan compatible physical devices.", devices.size());

    if (devices.empty())
    {
        LOG(CRITICAL, "No compatible physical devices found.");
    }

    // Filter and select device
    std::uint32_t bestPriority = 0;
    for (const vk::raii::PhysicalDevice &physicalDeviceVariant : devices)
    {
        std::uint32_t priority = 1;

        // Check device features and properties
        physicalDeviceCapabilities.features = physicalDeviceVariant.getFeatures2<
            PhysicalDeviceCapabilities::Features1,
            PhysicalDeviceCapabilities::Features12,
            PhysicalDeviceCapabilities::Features13
        >();

        //TODO: provide features list to check and enable
        const PhysicalDeviceCapabilities::Features1 &features1 = physicalDeviceCapabilities.GetFeatures1();
        const PhysicalDeviceCapabilities::Features13 &features13 = physicalDeviceCapabilities.GetFeatures13();
        if (
            features1.features.wideLines != vk::True ||
            features1.features.independentBlend != vk::True ||
            features1.features.multiDrawIndirect != vk::True ||

            features13.dynamicRendering != vk::True ||
            features13.synchronization2 != vk::True ||
            features13.maintenance4 != vk::True
        ) {
            continue;
        }

        PhysicalDeviceCapabilities::Features1 &enabledFeatures1 = physicalDeviceCapabilities.GetEnabledFeatures1();
        PhysicalDeviceCapabilities::Features13 &enabledFeatures13 = physicalDeviceCapabilities.GetEnabledFeatures13();
        enabledFeatures1.features.wideLines = vk::True;
        enabledFeatures1.features.independentBlend = vk::True;
        enabledFeatures1.features.multiDrawIndirect = vk::True;
        enabledFeatures13.dynamicRendering = vk::True;
        enabledFeatures13.synchronization2 = vk::True;
        enabledFeatures13.maintenance4 = vk::True;

        physicalDeviceCapabilities.properties = physicalDeviceVariant.getProperties();
        if (physicalDeviceCapabilities.properties.apiVersion < vk::makeApiVersion(0, 1, 3, 0))
            continue;

        // Check queue families
        std::vector<vk::QueueFamilyProperties> queueFamiliesProperties = physicalDeviceVariant.getQueueFamilyProperties();

        std::uint32_t i = 0;
        PhysicalDevice::QueueFamilies queueFamiliesBuffer;
        for (const vk::QueueFamilyProperties &familyProps : queueFamiliesProperties)
        {
            if (familyProps.queueFlags & vk::QueueFlagBits::eGraphics)
                queueFamiliesBuffer.graphics = i;

            if (
                queueFamiliesBuffer.graphics != queueFamiliesBuffer.present ||
                queueFamiliesBuffer.graphics == PhysicalDevice::QueueFamilies::NO_VALUE
            )
            {
                if (physicalDeviceVariant.getSurfaceSupportKHR(i, surface.handle))
                    queueFamiliesBuffer.present = i;
            }

            i++;
        }

        if (
            queueFamiliesBuffer.graphics == PhysicalDevice::QueueFamilies::NO_VALUE ||
            queueFamiliesBuffer.present == PhysicalDevice::QueueFamilies::NO_VALUE
        )
        continue;
        queueFamiliesBuffer.transfer = queueFamiliesBuffer.graphics;

        // Check surface info
        SurfaceCapabilities surfaceCapabilitiesBuffer {};

        surfaceCapabilitiesBuffer.availableFormats = physicalDeviceVariant.getSurfaceFormatsKHR(surface.handle);
        surfaceCapabilitiesBuffer.availablePresentModes =  physicalDeviceVariant.getSurfacePresentModesKHR(surface.handle);

        if (
            surfaceCapabilitiesBuffer.availableFormats.empty() ||
            surfaceCapabilitiesBuffer.availablePresentModes.empty()
        )
            continue;

        // Rate and select physical device
        if (queueFamiliesBuffer.graphics == queueFamiliesBuffer.present)
            priority += priorities.queueFamilies.PRESENT;

        switch (physicalDeviceCapabilities.properties.deviceType) {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                priority += priorities.gpu.DISCRETE;
                break;
            case vk::PhysicalDeviceType::eIntegratedGpu:
                priority += priorities.gpu.INTEGRATED;
                break;
            case vk::PhysicalDeviceType::eVirtualGpu:
                priority += priorities.gpu.VIRTUAL;
                break;
            case vk::PhysicalDeviceType::eCpu:
                priority += priorities.gpu.CPU;
                break;
            case vk::PhysicalDeviceType::eOther:
                priority += priorities.gpu.OTHER;
                break;
        }

        vk::SurfaceFormatKHR formatBuffer {
            .format = vk::Format::eB8G8R8A8Unorm,
            .colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear
        };
        for (const vk::SurfaceFormatKHR &format : surfaceCapabilitiesBuffer.availableFormats)
        {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                formatBuffer = format;
                priority += priorities.surfaceFormats.B8G8R8_SRGB;
                break;
            }
        }

        vk::PresentModeKHR presentModeBuffer = vk::PresentModeKHR::eFifo;
        for (const vk::PresentModeKHR &presentMode : surfaceCapabilitiesBuffer.availablePresentModes)
        {
            if (presentMode == vk::PresentModeKHR::eFifoRelaxed)
            {
                presentModeBuffer = presentMode;
                priority += priorities.presentModes.FIFO_RELAXED;
                break;
            }
        }

        if (priority > bestPriority)
        {
            bestPriority = priority;
            physicalDevice.handle = physicalDeviceVariant;
            physicalDevice.queueFamilies = queueFamiliesBuffer;
            surfaceCapabilities = surfaceCapabilitiesBuffer;
            selectedFormat = formatBuffer;
            selectedPresentMode = presentModeBuffer;
        }
    }
    
    surfaceCapabilities.capabilities = physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle);

    LOG(INFO, "%hs is selected for render with %d priority score", physicalDeviceCapabilities.properties.deviceName, bestPriority);

    physicalDevice.memory.properties = physicalDevice.handle.getMemoryProperties();
}

void vulkan_render::Context::SetupLogicalDevice(const std::vector<const char*> &extensions) {
    // Define queues
    std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfos;
    const std::set uniqueQueueFamilies {
        physicalDevice.queueFamilies.transfer,
        physicalDevice.queueFamilies.graphics,
        physicalDevice.queueFamilies.present
    };

    for (const std::uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo {
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &LogicalDevice::DEFAULT_QUEUE_PRIORITY
    };

        queuesCreateInfos.push_back(queueCreateInfo);
    }

    // Create logical device
    const vk::DeviceCreateInfo deviceInfo {
        .pNext = &physicalDeviceCapabilities.enabledFeatures.get(),
        .queueCreateInfoCount = static_cast<std::uint32_t>(queuesCreateInfos.size()),
        .pQueueCreateInfos = queuesCreateInfos.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    device.handle = physicalDevice.handle.createDevice(deviceInfo);
    LOG(INFO, "Logical device is created.");

    // Save queues descriptor
    device.transfer = device.handle.getQueue(physicalDevice.queueFamilies.transfer, 0);
    device.graphics = device.handle.getQueue(physicalDevice.queueFamilies.graphics, 0);
    device.present = device.handle.getQueue(physicalDevice.queueFamilies.present, 0);
    LOG(INFO, "Queues registered.");
}

std::optional<std::uint32_t> vulkan_render::Context::FindMemoryType(const vk::MemoryRequirements &requirements, const vk::MemoryPropertyFlags &flags) {
    std::optional<std::uint32_t> memoryType;

    for (std::uint32_t i = 0; i < physicalDevice.memory.properties.memoryTypeCount; i++)
    {
        if (
            requirements.memoryTypeBits & (1 << i) &&
            (physicalDevice.memory.properties.memoryTypes[i].propertyFlags & flags) == flags
        ) {
            memoryType = i;
            break;
        }
    }
    if (!memoryType.has_value()) {
        LOG(CRITICAL, "Failed to find GPU memory with requirements %d and flags %d", requirements.memoryTypeBits, flags);
    }

    return memoryType;
}

std::optional<vk::raii::DeviceMemory> vulkan_render::Context::AllocateMemory(
    const vk::MemoryRequirements2 &memoryRequirements,
    const vk::MemoryPropertyFlags &memoryFlags
) const {
    std::optional<std::uint32_t> memoryTypeId;
    const vk::PhysicalDeviceMemoryProperties &memoryProps = physicalDevice.memory.properties;
    for (std::uint32_t i = 0; i < memoryProps.memoryTypeCount; i++) {
        if (
            memoryRequirements.memoryRequirements.memoryTypeBits & (1 << i) &&
            (memoryProps.memoryTypes[i].propertyFlags & memoryFlags) == memoryFlags
        ) {
            memoryTypeId = i;
            break;
        }
    }
    if (!memoryTypeId.has_value()) {
        LOG(CRITICAL, "Failed to find suitable GPU memory. Flags: %d", memoryFlags);
        return std::nullopt;
    }

    return device.handle.allocateMemory({
        .allocationSize = memoryRequirements.memoryRequirements.size,
        .memoryTypeIndex = memoryTypeId.value()
    });
}

std::optional<vk::raii::DeviceMemory> vulkan_render::Context::AllocateBufferMemory(
    const vk::raii::Buffer &buffer,
    const vk::MemoryPropertyFlags &memoryFlags
) const {
    return AllocateMemory(device.handle.getBufferMemoryRequirements2({.buffer = buffer}), memoryFlags);
}

std::optional<vk::raii::DeviceMemory> vulkan_render::Context::AllocateImageMemory(
    const vk::raii::Image &image,
    const vk::MemoryPropertyFlags &memoryFlags
) const {
    return AllocateMemory(device.handle.getImageMemoryRequirements2({.image = image}), memoryFlags);
}

vk::raii::Instance vulkan_render::Context::MakeInstance(
    const char *appName,
    const std::vector<const char*> &extensions,
    const std::vector<const char*> &layers
) const {
    const vk::ApplicationInfo appInfo {
        .pApplicationName = appName,
        .applicationVersion = vk::makeApiVersion(0, 1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = vk::makeApiVersion(0, 1, 0, 0),
        .apiVersion = vk::makeApiVersion(0, 1, 3, 0)
    };

    const vk::InstanceCreateInfo createInfo {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    return vk::raii::Instance(context, createInfo);
}
