/**
 * Data partition module is responsible for configuration user-specific Vulkan objects
 * like buffers, textures, transforms and so on.
 */

module;

#if defined(_MSC_VER)
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

export module vulkan_render:data;
import :hardware;
import :resources;
import std;
import vulkan_hpp;
import utils;

namespace vulkan_render {
    struct NoMember {
        constexpr NoMember() = default;
        constexpr NoMember(std::nullptr_t) {}
    };

    template <typename T, bool condition>
    using optional = std::conditional_t<condition, T, NoMember>;

    template <bool hasStaging>
    using StagingBuffer = optional<vk::raii::Buffer, hasStaging>;

    template <bool hasStaging>
    using StagingDeviceMemory = optional<vk::raii::DeviceMemory, hasStaging>;

    template <typename T, bool isMapped>
    using MappedMemory = optional<std::span<T>, isMapped>;
}

export namespace vulkan_render {

    enum class BufferMode {
        STATIC,
        CPU_ACCESS,
        STAGING_ACCESS
    };

    template <BufferMode mode>
    class BaseBuffer {
    public:
        const Context &context;

        vk::raii::Buffer handle = nullptr;
        vk::raii::DeviceMemory memory = nullptr;

        NO_UNIQUE_ADDRESS StagingBuffer<mode == BufferMode::STAGING_ACCESS> stagingHandle = nullptr;
        NO_UNIQUE_ADDRESS StagingDeviceMemory<mode == BufferMode::STAGING_ACCESS> stagingMemory = nullptr;

        std::size_t memorySize;

        explicit BaseBuffer(const Context &context, const std::size_t memorySize) :
            context(context),
            memorySize(memorySize) {}
    };

    template <typename T, vk::BufferUsageFlagBits usage, BufferMode mode>
    class Buffer : public BaseBuffer<mode> {
    public:
        NO_UNIQUE_ADDRESS MappedMemory<T, mode != BufferMode::STATIC> content;
        static constexpr std::size_t elementSize = sizeof(T);

        Buffer(const Context &context, const std::span<T> data) requires (mode == BufferMode::STATIC)
            : BaseBuffer<mode>(context, data.size() * elementSize) {

            MakeInstance(
                usage,
                vk::Flags(vk::MemoryPropertyFlagBits::eHostVisible) | vk::MemoryPropertyFlagBits::eHostCoherent,
                this->handle,
                this->memory
            );

            void *mappedMemory = this->memory.mapMemory(0, this->memorySize);
            std::ranges::copy(data, static_cast<T*>(mappedMemory));
            this->memory.unmapMemory();
        }

        Buffer(const Context &context, const std::size_t size) requires (mode == BufferMode::CPU_ACCESS)
            : BaseBuffer<mode>(context, size * elementSize) {

            MakeInstance(
                usage,
                vk::Flags(vk::MemoryPropertyFlagBits::eHostVisible) | vk::MemoryPropertyFlagBits::eHostCoherent,
                this->handle,
                this->memory
            );

            void *mappedMemory = this->memory.mapMemory(0, this->memorySize);

            content = std::span<T>(static_cast<T*>(mappedMemory), size);
            std::ranges::fill(content, T{});
        }

        Buffer(const Context &context, const std::size_t size) requires (mode == BufferMode::STAGING_ACCESS)
            : BaseBuffer<mode>(context, size * elementSize) {
            MakeInstance(
                vk::Flags(vk::BufferUsageFlagBits::eTransferDst) | usage,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                this->handle,
                this->memory
            );

            MakeInstance(
                vk::BufferUsageFlagBits::eTransferSrc,
                 vk::Flags(vk::MemoryPropertyFlagBits::eHostVisible) | vk::MemoryPropertyFlagBits::eHostCoherent,
                this->stagingHandle,
                this->stagingMemory
            );

            void *mappedMemory = this->stagingMemory.mapMemory(0, this->memorySize);

            content = std::span<T>(static_cast<T*>(mappedMemory), size);
            std::ranges::fill(content, T{});
        }

    private:
        void MakeInstance(
            const vk::BufferUsageFlags &usageFlags,
            const vk::MemoryPropertyFlags &memoryFlags,
            vk::raii::Buffer &buffer,
            vk::raii::DeviceMemory &memory
        ) {
            buffer = this->context.device.handle.createBuffer({
                .size = this->memorySize,
                .usage = usageFlags,
                .sharingMode = vk::SharingMode::eExclusive
            });

            if (
                std::optional<vk::raii::DeviceMemory> allocated = this->context.AllocateBufferMemory(
                    buffer,
                    memoryFlags
                );
                allocated.has_value()
            ) {
                memory = std::move(allocated).value();
                buffer.bindMemory(memory, 0);
            }
        }
    };

    enum class TextureMode {
        RENDER,
        DEPTH
    };

    template <TextureMode mode>
    class Texture {
    public:
        const Context &context;
        const std::uint32_t width;
        const std::uint32_t height;

        NO_UNIQUE_ADDRESS MappedMemory<std::byte, mode == TextureMode::RENDER> data;

        vk::raii::Image handle = nullptr;
        vk::raii::ImageView view = nullptr;
        vk::raii::DeviceMemory memory = nullptr;

        const vk::Format format;

        Texture(
            const Context &context,
            const std::uint32_t width, const std::uint32_t height,
            const vk::Format &format,
            const std::span<std::byte> textureData
        ) requires (mode == TextureMode::RENDER)
        : context(context), width(width), height(height), data(textureData), format(format) {
            Buffer<
                std::byte,
                vk::BufferUsageFlagBits::eTransferSrc,
                BufferMode::STATIC
            > stagingBuffer(context, data);

            MakeInstance(
                vk::Flags(vk::ImageUsageFlagBits::eTransferDst) | vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                handle,
                memory
            );

            /* TODO: Transition image layout
             * Copy buffer to image
             * Transition image layout
             * Create view
             */
        }

        Texture(
            const Context &context,
            const Surface &surface
        ) requires (mode == TextureMode::DEPTH)
        : context(context), width(surface.extent.width), height(surface.extent.height), format(vk::Format::eD32SfloatS8Uint) {
            MakeInstance(
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                handle,
                memory
            );
            MakeView(vk::ImageAspectFlagBits::eDepth);
        }

        Texture(Texture&&) noexcept = default;
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

    private:
        void MakeInstance(
            const vk::ImageUsageFlags &usageFlags,
            const vk::MemoryPropertyFlags &memoryflags,
            vk::raii::Image &image,
            vk::raii::DeviceMemory &memory
        ) const {
            image = context.device.handle.createImage({
                .imageType = vk::ImageType::e2D,
                .format = format,
                .extent = {.width = width, .height = height, .depth = 1},
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = vk::SampleCountFlagBits::e1,
                .tiling = vk::ImageTiling::eOptimal,
                .usage = usageFlags,
                .sharingMode = vk::SharingMode::eExclusive,
                .initialLayout = vk::ImageLayout::eUndefined,
            });

            if (
                std::optional<vk::raii::DeviceMemory> allocated = context.AllocateImageMemory(
                    handle,
                    memoryflags
                );
                allocated.has_value()
            ) {
                memory = std::move(allocated).value();
                handle.bindMemory(memory, 0);
            }
        }

        void MakeView(const vk::ImageAspectFlags &flags) {
            view = context.device.handle.createImageView({
                .image = handle,
                .viewType = vk::ImageViewType::e2D,
                .format = format,
                .subresourceRange = {
                    .aspectMask = flags,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            });
        }
    };

    template <typename T>
    class Uniform {
    public:
        std::vector<Buffer<T, vk::BufferUsageFlagBits::eUniformBuffer, BufferMode::CPU_ACCESS>> buffers;
        explicit Uniform(const Context &context, std::uint32_t maxFramesInFlight) {
            buffers.reserve(maxFramesInFlight);
            for (std::uint32_t i = 0; i < maxFramesInFlight; i++) {
                buffers.push_back(Buffer<T, vk::BufferUsageFlagBits::eUniformBuffer, BufferMode::CPU_ACCESS>(context, 1));
            }
        }
    };

}