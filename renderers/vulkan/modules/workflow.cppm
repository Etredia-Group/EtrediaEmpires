/**
 * Workflow partition module is responsible for configuration of rendering stages
 * including pipelines, render passes and so on.
 */

export module vulkan_render:workflow;
import :hardware;
import :resources;
import :data;
import std;
import vulkan_hpp;

export namespace vulkan_render {
    using DescriptorBindings = std::vector<vk::DescriptorSetLayoutBinding>;
    using DescriptorCounts = std::unordered_map<vk::DescriptorType /*type*/, std::uint8_t /*count*/>;

    /**
     * Content of SPIR-V shaders binary data for a single pipeline.
     */
    struct ShaderProgram {
        std::span<std::byte> vertex;
        std::span<std::byte> fragment;

        /**
         * Name of vertex shader entry point.
         */
        const char* vertexMainFun = "main";
        /**
         * Name of fragment shader entry point.
         */
        const char* fragmentMainFun = "main";
    };

    struct ShaderAttributeFormat {
        static constexpr vk::Format FLOAT = vk::Format::eR32Sfloat;
        static constexpr vk::Format VEC2 = vk::Format::eR32G32Sfloat;
        static constexpr vk::Format VEC3 = vk::Format::eR32G32B32Sfloat;
        static constexpr vk::Format VEC4 = vk::Format::eR32G32B32A32Sfloat;
        static constexpr vk::Format UVEC4 = vk::Format::eR8G8B8A8Unorm;

        ShaderAttributeFormat() = delete;
    };

    /**
     * Shaders input attributes binding.
     */
    struct ShaderBindings {
        vk::VertexInputBindingDescription vertexBinding;
        std::uint32_t vertexAttributesCount;
        std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    };

    /**
     * Set of descriptions needed for Vulkan pipeline initialization.
     */
    struct PipelineConfig {
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        vk::PipelineViewportStateCreateInfo viewportStateInfo;
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
        vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
        vk::DescriptorSetLayoutCreateInfo descriptorsInfo;

        // SRC - new color, DST - old color
        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates;
        std::vector<vk::DynamicState> dynamicStates;
        std::vector<vk::DescriptorSetLayoutBinding> descriptors;
        std::uint32_t maxExtraDescriptorSets = 0;

        vk::Viewport defaultViewport;
        vk::Rect2D defaultScissors;
        vk::PushConstantRange pushConstantRange;
    };

    struct DescriptorInfo {
        std::variant<
            vk::DescriptorBufferInfo,
            vk::DescriptorImageInfo
        > info;
        std::uint32_t bindingPosition;
        vk::DescriptorType type;
    };
}

export namespace vulkan_render {
    class RenderPass {
    public:
        const Context &context;
        const Swapchain &swapchain;
        const Surface &surface;

        vk::raii::RenderPass handle;
        std::vector<vk::raii::Framebuffer> framebuffers;

        RenderPass(
            const Context &context,
            const Swapchain &swapchain,
            const Surface &surface,
            const Texture<TextureMode::DEPTH> &depth
        );
    private:
        vk::raii::RenderPass MakeInstance() const;
        void MakeFramebuffers(const Texture<TextureMode::DEPTH> &depth);
    };

    /**
     * Vulkan pipeline constructed on context, shaders and descriptors.
     */
    class Pipeline {
    public:
        const Context &context;
        const Surface &surface;
        const Swapchain &swapchain;
        const RenderPass &renderPass;
        const std::uint32_t maxFramesInFlight;
        DescriptorCounts descriptorCounts;

        vk::raii::DescriptorSetLayout descriptorsLayout;
        vk::raii::PipelineLayout layout;
        vk::raii::Pipeline handle;

        vk::Viewport viewport;
        vk::Rect2D scissors;

        vk::raii::DescriptorPool descriptorPool;
        vk::raii::DescriptorSets descriptorSets;

        /**
         * Initialize Pipeline on shaders and their input.
         * @param context Vulkan context with ready device.
         * @param surface
         * @param swapchain swapchain frame images used to allocate descriptors.
         * @param config pipeline configuration.
         * @param shaderProgram SPIR-V shaders binaries.
         * @param maxFramesInFlight max number of frames in flight used to allocate descriptors .
         */
        Pipeline(
            const Context &context,
            const Surface &surface,
            const Swapchain &swapchain,
            const PipelineConfig &config,
            const RenderPass &renderPass,
            const ShaderProgram &shaderProgram,
            std::uint32_t maxFramesInFlight
        );

        void UpdateDescriptors(const std::vector<DescriptorInfo> &descriptors, vk::DescriptorSet set) const;
        void UpdateDescriptors(const std::vector<DescriptorInfo> &descriptors, std::uint32_t frame);
        vk::raii::DescriptorSets AllocateDescriptorSet() const;

        /**
         * Construct default pipeline configuration for given shaders input layout.
         * @param surface surface origin.
         * @param bindings shaders input attributes binding definition.
         * @param descriptors descriptors binding definition (e.g. uniforms).
         * @param pushConstants definition for small performant data set.
         * @return default pipeline configuration.
         */
        static PipelineConfig MakeConfig(
            const Surface &surface,
            const ShaderBindings &bindings,
            const DescriptorBindings &descriptors,
            const vk::PushConstantRange& pushConstants
        );

    private:
        vk::raii::DescriptorSetLayout MakeDescriptorsLayout(const PipelineConfig &config) const;
        vk::raii::PipelineLayout MakeLayout(const PipelineConfig &config) const;
        vk::raii::Pipeline MakeInstance(const PipelineConfig &config, const ShaderProgram &shaderProgram) const;
        vk::raii::DescriptorPool MakeDescriptorPool(
            const DescriptorBindings &descriptors,
            std::uint32_t maxExtraDescriptorSet
        );
        vk::raii::DescriptorSets MakeDescriptorSets() const;
    };

}