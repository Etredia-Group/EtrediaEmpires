/**
 * Hardware partition module is responsible for any Vulkan operations dependent on
 * hardware capabilities including devices and extensions initialization, memory allocations and so on.
 */

export module vulkan_render:hardware;
import :resources;
import std;
import vulkan_hpp;

export namespace vulkan_render {

    struct PhysicalDeviceCapabilities {
        vk::PhysicalDeviceProperties properties;

        using Features1 = vk::PhysicalDeviceFeatures2;
        using Features12 = vk::PhysicalDeviceVulkan12Features;
        using Features13 = vk::PhysicalDeviceVulkan13Features;
        using Features = vk::StructureChain<Features1, Features12, Features13>;

        Features features;
        Features enabledFeatures;

        Features1& GetFeatures1() { return features.get<Features1>(); }
        Features12& GetFeatures12() { return features.get<Features12>(); }
        Features13& GetFeatures13() { return features.get<Features13>(); }
        Features1& GetEnabledFeatures1() { return enabledFeatures.get<Features1>(); }
        Features12& GetEnabledFeatures12() { return enabledFeatures.get<Features12>(); }
        Features13& GetEnabledFeatures13() { return enabledFeatures.get<Features13>(); }
    };

    /**
     * Priority scoring rules to select one physical device among available.
     */
    struct PhysicalDevicePriorities {
        struct GPU {
            std::uint32_t DISCRETE = 1000;
            std::uint32_t INTEGRATED = 1;
            std::uint32_t VIRTUAL = 1;
            std::uint32_t CPU = 0;
            std::uint32_t OTHER = 0;
        } gpu;

        struct QueueFamilies {
            std::uint32_t PRESENT = 500;
        } queueFamilies;

        struct SurfaceFormats {
            std::uint32_t B8G8R8_SRGB = 10;
        } surfaceFormats;

        struct PresentModes {
            std::uint32_t FIFO_RELAXED = 10;
        } presentModes;
    };

    struct SurfaceCapabilities {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> availableFormats;
        std::vector<vk::PresentModeKHR> availablePresentModes;
    };

}

export namespace vulkan_render {
    /**
     * Vulkan objects associated with physical device.
     */
    struct PhysicalDevice {
        /**
         * Ids of queue families by their usage.
         */
        struct QueueFamilies {
            static constexpr std::uint32_t NO_VALUE = std::numeric_limits<std::uint32_t>::max();
            std::uint32_t transfer = NO_VALUE;
            std::uint32_t graphics = NO_VALUE;
            std::uint32_t present = NO_VALUE;
        } queueFamilies;

        struct Memory {
            vk::PhysicalDeviceMemoryProperties properties;
        } memory;

        vk::raii::PhysicalDevice handle = nullptr;
    };

    /**
     * Vulkan objects associated with logical device.
     */
    struct LogicalDevice {
        vk::raii::Device handle = nullptr;

        static constexpr float DEFAULT_QUEUE_PRIORITY = 1.0f;
        vk::raii::Queue transfer = nullptr;
        vk::raii::Queue graphics = nullptr;
        vk::raii::Queue present = nullptr;
    };

}

export namespace vulkan_render {

    /**
     * Vulkan device and hardware context.
     * @warning Physical and logical devices need to be defined either directly
     * or using vulkan_render::Context::SetupPhysicalDevice and vulkan_render::Context::SetupLogicalDevice
     * before context usage.
     */
    class Context {
    public:
        vk::raii::Context context;
        vk::raii::Instance vulkan;

        LogicalDevice device {};
        PhysicalDevice physicalDevice {};

        /// Loaded on physical device selection in vulkan_render::Context::SetupPhysicalDevice.
        PhysicalDeviceCapabilities physicalDeviceCapabilities;
        /// Loaded on physical device selection in vulkan_render::Context::SetupPhysicalDevice.
        SurfaceCapabilities surfaceCapabilities;

        /**
         * Initialize VkInstance.
         * @param appName application name to register in Vulkan.
         */
        explicit Context(const char* appName);
        /**
         * Initialize VkInstance.
         * @param instanceInfo custom Vulkan configuration.
         */
        explicit Context(const vk::InstanceCreateInfo &instanceInfo);
        /**
         * Initialize VkInstance.
         * @param appName application name to register in Vulkan.
         * @param extensions list of vulkan enabled extensions labels.
         * @param layers list of vulkan enabled layers labels.
         */
        Context(const char* appName, const std::vector<const char*> &extensions, const std::vector<const char*> &layers);

        /**
         * Select available physical device by priority.
         *
         * @note Updates vulkan_render::Context::physicalDevice and vulkan_render::Context::physicalDeviceCapabilities
         * if at least one device is available, otherwise no effect.
         *
         * @param surface instance to be capable with.
         * @param priorities configuration of physical device selection rules.
         * @param[out] selectedFormat surface format of selected physical device. Undefined if no device was selected.
         * @param[out] selectedPresentMode present mode of selected physical device. Undefined if no device was selected.
         */
        void SetupPhysicalDevice(
            const Surface &surface,
            const PhysicalDevicePriorities &priorities,
            vk::SurfaceFormatKHR &selectedFormat,
            vk::PresentModeKHR &selectedPresentMode
        );
        /**
         * Initialize logical device with its available queues.
         * @param extensions list of enabled device extensions.
         */
        void SetupLogicalDevice(const std::vector<const char*> &extensions);

        /**
         * Find memory type available on device for specified requirements.
         * @return id of memory type or std::nullopt if no suitable memory found.
         */
        std::optional<std::uint32_t> FindMemoryType(
            const vk::MemoryRequirements &requirements,
            const vk::MemoryPropertyFlags &flags
        );

        std::optional<vk::raii::DeviceMemory> AllocateMemory(
            const vk::MemoryRequirements2 &memoryRequirements,
            const vk::MemoryPropertyFlags &memoryFlags
        ) const;

        std::optional<vk::raii::DeviceMemory> AllocateBufferMemory(
            const vk::raii::Buffer &buffer,
            const vk::MemoryPropertyFlags &memoryFlags
        ) const;

        std::optional<vk::raii::DeviceMemory> AllocateImageMemory(
            const vk::raii::Image &image,
            const vk::MemoryPropertyFlags &memoryFlags
        ) const;

    private:
        vk::raii::Instance MakeInstance(
            const char *appName,
            const std::vector<const char*> &extensions,
            const std::vector<const char*> &layers
        ) const;
    };

}
