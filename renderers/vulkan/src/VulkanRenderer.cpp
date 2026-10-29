module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/packing.hpp>
#include <RmlUi/Core/Core.h>

module VulkanRender;
import utils;

namespace {
    /// Region id of the storage buffer element describing the ocean plane.
    constexpr std::uint32_t OCEAN_REGION_ID = std::numeric_limits<std::uint32_t>::max();
    /// Half-size of the ocean plane in world units, large enough to fill the view at any zoom.
    constexpr float OCEAN_PLANE_EXTENT = 1.0e6f;
    /// Water margin around the map in rasters (texels), so coastal effects are not cut at the map edge.
    constexpr std::int32_t RASTER_PADDING = 32;
    /// Distances stored in the coast distance field are clamped to this value (raster texels).
    constexpr float MAX_COAST_DISTANCE = 4096.0f;
    /// Upper bound of the province raster size; bigger maps are rasterized with several map pixels per texel.
    constexpr std::uint64_t MAX_RASTER_TEXELS = 48ull * 1024 * 1024;
    /// Upper bound of the coast distance field size; it is smooth, so it may be coarser than the province raster.
    constexpr std::uint64_t MAX_COAST_FIELD_TEXELS = 12ull * 1024 * 1024;
    constexpr std::size_t REGIONS_YIELD_STEP = 256;

    struct Bounds {
        glm::vec2 min {std::numeric_limits<float>::max()};
        glm::vec2 max {std::numeric_limits<float>::lowest()};

        void Add(const glm::vec2 &point) {
            min = glm::min(min, point);
            max = glm::max(max, point);
        }

        bool IsEmpty() const { return min.x > max.x || min.y > max.y; }
    };

    /**
     * Map rasterized with texelSize x texelSize map pixels per texel.
     * Texel (0, 0) center is located at world position origin, texel (x, y) at origin + (x, y) * texelSize.
     */
    struct ProvinceRaster {
        glm::ivec2 origin {0, 0};
        glm::ivec2 size {1, 1};
        std::int32_t texelSize = 1;
        /// Province index + 1, 0 for water.
        std::vector<std::uint32_t> ids;

        ProvinceRaster(const Bounds &bounds, const std::int32_t mapPixelsPerTexel) : texelSize(mapPixelsPerTexel) {
            if (!bounds.IsEmpty()) {
                const glm::ivec2 low(
                    static_cast<std::int32_t>(std::floor(bounds.min.x)),
                    static_cast<std::int32_t>(std::floor(bounds.min.y))
                );
                const glm::ivec2 high(
                    static_cast<std::int32_t>(std::ceil(bounds.max.x)),
                    static_cast<std::int32_t>(std::ceil(bounds.max.y))
                );
                origin = low - RASTER_PADDING * texelSize;
                size = (high - low) / texelSize + 1 + 2 * RASTER_PADDING;
            }
            ids.assign(static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y), 0);
        }

        /**
         * Smallest integer texel size that keeps the raster within texture limits and memory budget.
         */
        static std::int32_t ChooseTexelSize(
            const Bounds &bounds,
            const std::uint32_t maxDimension,
            const std::uint64_t maxTexels
        ) {
            if (bounds.IsEmpty()) return 1;

            const glm::vec2 extent = bounds.max - bounds.min + 1.0f;
            for (std::int32_t candidate = 1; ; ++candidate) {
                const float step = static_cast<float>(candidate);
                const std::uint64_t width =
                    static_cast<std::uint64_t>(std::ceil(extent.x / step)) + 2 * RASTER_PADDING + 1;
                const std::uint64_t height =
                    static_cast<std::uint64_t>(std::ceil(extent.y / step)) + 2 * RASTER_PADDING + 1;
                if (width <= maxDimension && height <= maxDimension && width * height <= maxTexels)
                    return candidate;
            }
        }

        /// Integer world position relative to the raster origin.
        glm::ivec2 ToLocal(const glm::vec2 &world) const {
            return glm::ivec2(
                static_cast<std::int32_t>(std::lround(world.x)),
                static_cast<std::int32_t>(std::lround(world.y))
            ) - origin;
        }
    };

    std::int64_t EdgeFunction(const glm::ivec2 &a, const glm::ivec2 &b, const glm::ivec2 &p) {
        return static_cast<std::int64_t>(b.x - a.x) * (p.y - a.y) - static_cast<std::int64_t>(b.y - a.y) * (p.x - a.x);
    }

    std::int32_t FloorDiv(const std::int32_t value, const std::int32_t divisor) {
        return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
    }

    /**
     * Marks texels whose centers lie inside (or on the edge of) province triangles.
     * Mesh vertices are pixel centers of the source bitmap, so with texel size 1 the result closely reproduces the source map.
     */
    void RasterizeProvince(
        ProvinceRaster &raster,
        const std::vector<glm::vec2> &vertices,
        const std::vector<std::uint16_t> &indices,
        const std::uint32_t id
    ) {
        std::uint32_t *ids = raster.ids.data();
        const std::int32_t width = raster.size.x;
        const std::int32_t step = raster.texelSize;

        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size())
                continue;

            const glm::ivec2 a = raster.ToLocal(vertices[indices[i]]);
            glm::ivec2 b = raster.ToLocal(vertices[indices[i + 1]]);
            glm::ivec2 c = raster.ToLocal(vertices[indices[i + 2]]);

            const std::int64_t area = EdgeFunction(a, b, c);
            if (area == 0) continue;
            if (area < 0) std::swap(b, c);

            const std::int32_t minX = std::max(FloorDiv(std::min({a.x, b.x, c.x}) + step - 1, step), 0);
            const std::int32_t minY = std::max(FloorDiv(std::min({a.y, b.y, c.y}) + step - 1, step), 0);
            const std::int32_t maxX = std::min(FloorDiv(std::max({a.x, b.x, c.x}), step), raster.size.x - 1);
            const std::int32_t maxY = std::min(FloorDiv(std::max({a.y, b.y, c.y}), step), raster.size.y - 1);

            for (std::int32_t y = minY; y <= maxY; ++y) {
                for (std::int32_t x = minX; x <= maxX; ++x) {
                    const glm::ivec2 p(x * step, y * step);
                    if (EdgeFunction(b, c, p) >= 0 && EdgeFunction(c, a, p) >= 0 && EdgeFunction(a, b, p) >= 0)
                        ids[static_cast<std::size_t>(y) * width + x] = id;
                }
            }
        }
    }

    /**
     * Province contours are simplified, so thin slivers between neighbouring provinces stay unassigned.
     * Assigns such texels to the dominating neighbour when they are mostly surrounded by land.
     */
    std::generator<float> CloseRasterGaps(ProvinceRaster &raster, const std::uint32_t iterations) {
        const std::size_t width = static_cast<std::size_t>(raster.size.x);
        const std::size_t height = static_cast<std::size_t>(raster.size.y);
        std::uint32_t *ids = raster.ids.data();
        const std::size_t totalPixels = raster.ids.size();
        const std::size_t pixelsYieldStep = totalPixels / 5;

        std::vector<std::size_t> candidates;
        std::vector<std::pair<std::size_t, std::uint32_t>> changes;
        bool firstPass = true;

        const auto evaluate = [&](const std::size_t i) {
            if (ids[i] != 0) return;
            const std::size_t x = i % width;
            const std::size_t y = i / width;
            if (x == 0 || y == 0 || x + 1 >= width || y + 1 >= height) return;

            // 4-neighbours first, then diagonals
            const std::array neighbours {
                ids[i - 1], ids[i + 1], ids[i - width], ids[i + width],
                ids[i - width - 1], ids[i - width + 1], ids[i + width - 1], ids[i + width + 1]
            };
            std::int32_t sides = 0;
            std::int32_t assigned = 0;
            for (std::size_t n = 0; n < neighbours.size(); ++n) {
                if (neighbours[n] == 0) continue;
                ++assigned;
                if (n < 4) ++sides;
            }
            if (assigned == 0) return;

            const bool enclosed = (neighbours[0] != 0 && neighbours[1] != 0) ||
                (neighbours[2] != 0 && neighbours[3] != 0);
            if (sides < 3 && assigned < 5 && !enclosed) return;

            std::uint32_t best = 0;
            std::int32_t bestVotes = 0;
            for (const std::uint32_t candidate : neighbours) {
                if (candidate == 0) continue;
                const std::int32_t votes = static_cast<std::int32_t>(std::ranges::count(neighbours, candidate));
                if (votes > bestVotes) {
                    best = candidate;
                    bestVotes = votes;
                }
            }
            changes.emplace_back(i, best);
        };

        for (std::uint32_t iteration = 0; iteration < iterations; ++iteration) {
            changes.clear();
            if (firstPass) {
                for (std::size_t i = 0; i < totalPixels; ++i) {
                    evaluate(i);

                    if ((i & pixelsYieldStep) == 0) {
                        co_yield
                            (static_cast<float>(iteration) + static_cast<float>(i) / static_cast<float>(totalPixels)) /
                            static_cast<float>(iterations);
                    }
                }
                firstPass = false;
            }
            else {
                for (const std::size_t i : candidates) {
                    evaluate(i);

                    if ((i & pixelsYieldStep) == 0) {
                        co_yield
                            (static_cast<float>(iteration) + static_cast<float>(i) / static_cast<float>(totalPixels)) /
                            static_cast<float>(iterations);
                    }
                }
            }
            if (changes.empty()) break;

            // Apply after the scan, so every texel of an iteration sees the same state
            candidates.clear();
            for (const auto &[i, id] : changes) {
                ids[i] = id;
                for (
                    const std::size_t n : {
                        i - 1,
                        i + 1,
                        i - width,
                        i + width,
                        i - width - 1,
                        i - width + 1,
                        i + width - 1,
                        i + width + 1
                    }
                ) {
                    candidates.push_back(n);
                }
            }

            co_yield static_cast<float>(iteration + 1) / static_cast<float>(iterations);
        }

        co_yield 1.0f;
    }

    /**
     * Province meshes have no holes, so small water pockets fully enclosed by land are leftovers
     * of contour simplification. Fills them with the most common surrounding province.
     */
    std::generator<float> FillEnclosedWater(ProvinceRaster &raster, const std::size_t maxArea) {
        enum : std::uint8_t { UNVISITED, CURRENT, OPEN_WATER };

        const std::size_t width = static_cast<std::size_t>(raster.size.x);
        const std::size_t height = static_cast<std::size_t>(raster.size.y);
        const std::size_t totalPixels = raster.ids.size();
        const std::size_t pixelsYieldStep = totalPixels / 5;
        std::uint32_t *ids = raster.ids.data();
        std::vector<std::uint8_t> state(totalPixels, UNVISITED);
        std::vector<std::size_t> component;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> shore;

        for (std::size_t start = 0; start < totalPixels; ++start) {
            if ((start & pixelsYieldStep) == 0) {
                co_yield static_cast<float>(start) / static_cast<float>(totalPixels);
            }

            if (ids[start] != 0 || state[start] != UNVISITED) continue;

            // Bounded flood fill: stops as soon as the pocket proves to be open water
            component.assign(1, start);
            state[start] = CURRENT;
            shore.clear();
            bool open = false;

            for (std::size_t head = 0; head < component.size() && !open; ++head) {
                const std::size_t i = component[head];
                const std::size_t x = i % width;
                const std::size_t y = i / width;
                if (x == 0 || y == 0 || x + 1 >= width || y + 1 >= height) {
                    open = true;
                    break;
                }

                for (const std::size_t n : {i - 1, i + 1, i - width, i + width}) {
                    if (ids[n] != 0) {
                        const auto entry = std::ranges::find(
                            shore,
                            ids[n],
                            &std::pair<std::uint32_t, std::uint32_t>::first
                        );
                        if (entry != shore.end()) ++entry->second;
                        else shore.emplace_back(ids[n], 1);
                    }
                    else if (state[n] == OPEN_WATER) {
                        open = true;
                        break;
                    }
                    else if (state[n] == UNVISITED) {
                        state[n] = CURRENT;
                        component.push_back(n);
                        if (component.size() > maxArea) {
                            open = true;
                            break;
                        }
                    }
                }
            }

            if (open || shore.empty()) {
                for (const std::size_t i : component) state[i] = OPEN_WATER;
                continue;
            }

            const auto dominant = std::ranges::max_element(
                shore,
                {},
                &std::pair<std::uint32_t, std::uint32_t>::second
            );
            for (const std::size_t i : component) ids[i] = dominant->first;
        }

        co_yield 1.0f;
    }

    /**
     * Felzenszwalb & Huttenlocher squared Euclidean distance transform of a 1D sampled function.
     */
    void DistanceTransform1D(
        const float *f, float *d, std::int32_t *v, float *z, const std::int32_t n
    ) {
        std::int32_t k = 0;
        v[0] = 0;
        z[0] = -std::numeric_limits<float>::max();
        z[1] = std::numeric_limits<float>::max();

        const auto intersection = [f, v](const std::int32_t q, const std::int32_t index) {
            const std::int32_t p = v[index];
            return (
                f[q] +
                static_cast<float>(q) * static_cast<float>(q) -
                (f[p] + static_cast<float>(p) * static_cast<float>(p))
            ) / (2.0f * static_cast<float>(q - p));
        };

        for (std::int32_t q = 1; q < n; ++q) {
            float s = intersection(q, k);
            // z[0] is the lowest float, so k never drops below 0
            while (s <= z[k]) {
                --k;
                s = intersection(q, k);
            }
            ++k;
            v[k] = q;
            z[k] = s;
            z[k + 1] = std::numeric_limits<float>::max();
        }

        k = 0;
        for (std::int32_t q = 0; q < n; ++q) {
            while (z[k + 1] < static_cast<float>(q)) ++k;
            const float delta = static_cast<float>(q - v[k]);
            d[q] = delta * delta + f[v[k]];
        }
    }

    /**
     * In-place 2D squared Euclidean distance transform. Feature cells must be 0, others a large value.
     */
    void DistanceTransform2D(std::vector<float> &grid, const std::int32_t width, const std::int32_t height) {
        const std::int32_t length = std::max(width, height);
        std::vector<float> f(length), d(length), z(length + 1);
        std::vector<std::int32_t> v(length);

        for (std::int32_t x = 0; x < width; ++x) {
            for (std::int32_t y = 0; y < height; ++y) f[y] = grid[static_cast<std::size_t>(y) * width + x];
            DistanceTransform1D(f.data(), d.data(), v.data(), z.data(), height);
            for (std::int32_t y = 0; y < height; ++y) grid[static_cast<std::size_t>(y) * width + x] = d[y];
        }

        for (std::int32_t y = 0; y < height; ++y) {
            float *row = grid.data() + static_cast<std::size_t>(y) * width;
            std::copy_n(row, width, f.data());
            DistanceTransform1D(f.data(), row, v.data(), z.data(), width);
        }
    }

    /**
     * Signed distance to the coastline, measured in province raster texels: positive in water, negative on land.
     * The field is sampled every factor texels of the province raster to bound memory on large maps.
     * The coastline is placed on the edges of the land texels.
     */
    std::vector<float> MakeCoastDistanceField(
        const ProvinceRaster &raster,
        const std::int32_t factor,
        glm::ivec2 &fieldSize
    ) {
        fieldSize = (raster.size - 1) / factor + 1;
        const std::size_t count = static_cast<std::size_t>(fieldSize.x) * static_cast<std::size_t>(fieldSize.y);

        std::vector<std::uint8_t> land(count);
        std::vector<float> toLand(count), toWater(count);
        for (std::int32_t y = 0; y < fieldSize.y; ++y) {
            for (std::int32_t x = 0; x < fieldSize.x; ++x) {
                constexpr float FAR = 1.0e20f;
                const std::size_t i = static_cast<std::size_t>(y) * fieldSize.x + x;
                land[i] = raster.ids[static_cast<std::size_t>(y * factor) * raster.size.x + x * factor] != 0;
                toLand[i] = land[i] ? 0.0f : FAR;
                toWater[i] = land[i] ? FAR : 0.0f;
            }
        }

        DistanceTransform2D(toLand, fieldSize.x, fieldSize.y);
        DistanceTransform2D(toWater, fieldSize.x, fieldSize.y);

        std::vector<float> field(count);
        for (std::size_t i = 0; i < count; ++i) {
            const float distance = land[i]
                ? -(std::sqrt(toWater[i]) - 0.5f)
                : std::sqrt(toLand[i]) - 0.5f;
            field[i] = std::clamp(distance * static_cast<float>(factor), -MAX_COAST_DISTANCE, MAX_COAST_DISTANCE);
        }
        return field;
    }

    /**
     * Outward miter normals of the province outline for every vertex (zero for inner vertices).
     * The outline consists of the triangle edges that are not shared by two triangles.
     */
    std::vector<glm::vec2> ComputeOutlineNormals(
        const std::vector<glm::vec2> &vertices,
        const std::vector<std::uint16_t> &indices
    ) {
        struct Edge {
            std::uint32_t count = 0;
            std::uint16_t a = 0;
            std::uint16_t b = 0;
            std::uint16_t opposite = 0;
        };

        std::unordered_map<std::uint32_t, Edge> edges;
        edges.reserve(indices.size());

        const auto addEdge = [&edges](const std::uint16_t a, const std::uint16_t b, const std::uint16_t opposite) {
            const std::uint32_t key = a < b
                ? (static_cast<std::uint32_t>(a) << 16) | b
                : (static_cast<std::uint32_t>(b) << 16) | a;
            Edge &edge = edges[key];
            ++edge.count;
            edge.a = a;
            edge.b = b;
            edge.opposite = opposite;
        };

        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            const std::uint16_t a = indices[i], b = indices[i + 1], c = indices[i + 2];
            if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size()) continue;
            addEdge(a, b, c);
            addEdge(b, c, a);
            addEdge(c, a, b);
        }

        struct OutlineEdge {
            std::uint16_t a, b;
            glm::vec2 normal;
        };
        std::vector<OutlineEdge> outline;
        std::vector<glm::vec2> sums(vertices.size(), glm::vec2(0.0f));

        for (const Edge &edge : edges | std::views::values) {
            if (edge.count != 1) continue;

            const glm::vec2 direction = vertices[edge.b] - vertices[edge.a];
            if (glm::dot(direction, direction) < 1e-8f) continue;

            glm::vec2 normal = glm::normalize(glm::vec2(direction.y, -direction.x));
            if (glm::dot(normal, vertices[edge.opposite] - vertices[edge.a]) > 0.0f)
                normal = -normal;

            sums[edge.a] += normal;
            sums[edge.b] += normal;
            outline.push_back({edge.a, edge.b, normal});
        }

        std::vector<glm::vec2> normals(vertices.size(), glm::vec2(0.0f));
        std::vector<float> minCos(vertices.size(), 1.0f);
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            if (glm::dot(sums[i], sums[i]) > 1e-8f)
                normals[i] = glm::normalize(sums[i]);
        }
        for (const auto &[a, b, normal] : outline) {
            minCos[a] = std::min(minCos[a], glm::dot(normals[a], normal));
            minCos[b] = std::min(minCos[b], glm::dot(normals[b], normal));
        }
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            // Miter length, limited on sharp spikes
            normals[i] /= std::max(minCos[i], 0.35f);
        }

        return normals;
    }

    bool IsSrgbFormat(const vk::Format format) {
        switch (format) {
            case vk::Format::eB8G8R8A8Srgb:
            case vk::Format::eR8G8B8A8Srgb:
            case vk::Format::eA8B8G8R8SrgbPack32:
                return true;
            default:
                return false;
        }
    }
}

ee_vulkan::VulkanRenderer::VulkanRenderer(GLFWwindow *window) : IRenderer(window) {
    std::uint32_t glfwExtensionsCount;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

    context.emplace(
        "EtrediaEmpires",
        std::vector(glfwExtensions, glfwExtensions + glfwExtensionsCount),
        std::vector{"VK_LAYER_KHRONOS_validation"}
    );

    VkSurfaceKHR vulkanSurface;
    glfwCreateWindowSurface(*context->vulkan, window, nullptr, &vulkanSurface);
    surface.handle = vk::raii::SurfaceKHR(context->vulkan, vulkanSurface);
    surface.extent = {
        .width = static_cast<std::uint32_t>(screenWidth),
        .height = static_cast<std::uint32_t>(screenHeight)
    };

    context->SetupPhysicalDevice(surface, {}, surface.format, surface.presentMode);
    context->SetupLogicalDevice({vk::KHRSwapchainExtensionName});

    swapchain.emplace(
        vulkan_render::ResourcesFactory::CreateSwapchain(*context, surface)
    );
    renderCommands.emplace(*context, MAX_FRAMES_IN_FLIGHT);
    syncManager.emplace(*context, MAX_FRAMES_IN_FLIGHT, TIMEOUT);

    LOG(INFO, "Renderer created");
}

ee_vulkan::VulkanRenderer::~VulkanRenderer() {}

void ee_vulkan::VulkanRenderer::Init() {
    depth.emplace(*context, surface);
    renderPass.emplace(*context, *swapchain, surface, *depth);

    LoadProvincesMaterial();
    LoadRmlMaterial();

    ee_rml::RenderContext rmlContext {
        .context = *context,
        .surface = surface,
        .commands = *renderCommands,
        .pipeline = *rmlPipeline,

        .assetManager = *assetManager,
        .framesInFlight = MAX_FRAMES_IN_FLIGHT
    };
    rmlRenderInterface.emplace(rmlContext);

    mapUniform.emplace(*context, MAX_FRAMES_IN_FLIGHT);

    CreatePickReadback(true);

    _isReady = true;
    isDrawing = true;
}

void ee_vulkan::VulkanRenderer::Shutdown() {
    context->device.handle.waitIdle();
}

bool ee_vulkan::VulkanRenderer::DrawBegin() {
    if (isSwapchainDirty) {
        RecreateSwapchain();
        if (!isDrawing) return false;
    }

    syncManager->WaitInFlightFence(currentFrame);
    rmlRenderInterface->CollectGarbage();

    ResolvePick(currentFrame);

    std::uint32_t imageIndex = 0;
    switch (vk::Result result = syncManager->AcquireNextImage(*swapchain, currentFrame, imageIndex)) {
        case vk::Result::eSuccess:
        case vk::Result::eSuboptimalKHR:
            break;

        case vk::Result::eErrorOutOfDateKHR:
            LOG(WARNING, "Swapchain needs to be recreated");
            RecreateSwapchain();
            return false;

        default:
            LOG(ERROR, "Failed to get next image index. Code: %d", result);
            return false;
    }
    syncManager->WaitImageInFlight(currentFrame, imageIndex);

    UpdateData(currentFrame);
    RecordPick(currentFrame);

    std::array clears = {DrawConfig::CLEAR_COLOR, DrawConfig::CLEAR_DEPTH};
    frameContext.emplace(
        vulkan_render::FrameContext(*renderPass, surface, *renderCommands, imageIndex, currentFrame, clears)
    );
    rmlRenderInterface->SetFrameContext(&*frameContext);

    return true;
}

void ee_vulkan::VulkanRenderer::DrawMap() {
    frameContext->renderCommand->handle.bindPipeline(vk::PipelineBindPoint::eGraphics, provincesPipeline->handle);
    frameContext->renderCommand->handle.setViewport(0, {provincesPipeline->viewport});
    frameContext->renderCommand->handle.setScissor(0, {provincesPipeline->scissors});

    frameContext->renderCommand->handle.bindVertexBuffers(
        0,
        {provinces.vertices->handle},
        {0}
    );
    frameContext->renderCommand->handle.bindIndexBuffer(
        provinces.indices->handle,
        0,
        vk::IndexType::eUint32
    );

    frameContext->renderCommand->handle.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        provincesPipeline->layout,
        0,
        {provincesPipeline->descriptorSets[currentFrame]},
        {}
    );

    frameContext->renderCommand->handle.drawIndexedIndirect(
        provinces.indirectCommands->handle,
        0,
        provinces.indirectCommands->content.size(),
        provinces.indirectCommands->elementSize
    );
}

void ee_vulkan::VulkanRenderer::DrawUI() {
    frameContext->renderCommand->handle.bindPipeline(vk::PipelineBindPoint::eGraphics, rmlPipeline->handle);
    rmlContext->Render();
}

void ee_vulkan::VulkanRenderer::DrawEnd() {
    frameContext->Complete();

    context->device.handle.resetFences({syncManager->inFlightFences[currentFrame]});

    std::array<vk::CommandBufferSubmitInfo, 2> cmdSubmitInfo;
    std::uint32_t cmdSubmitInfoCount = 0;
    if (pick.recorded[currentFrame]) {
        cmdSubmitInfo[cmdSubmitInfoCount++] = {.commandBuffer = pick.commands->buffers[currentFrame].handle};
    }
    cmdSubmitInfo[cmdSubmitInfoCount++] = {.commandBuffer = frameContext->renderCommand->handle};

    vk::SemaphoreSubmitInfo waitSemaphoreInfo {
        .semaphore = syncManager->imageAvailable[currentFrame],
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };
    vk::SemaphoreSubmitInfo signalSemaphoreInfo {
        .semaphore = syncManager->renderFinished[currentFrame],
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };
    const vk::SubmitInfo2 submitInfo {
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = cmdSubmitInfoCount,
        .pCommandBufferInfos = cmdSubmitInfo.data(),
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalSemaphoreInfo,
    };
    context->device.graphics.submit2({submitInfo}, syncManager->inFlightFences[currentFrame]);

    rmlRenderInterface->EndFrame();

    vk::SwapchainKHR swapchains[] = {swapchain->handle};
    switch (
        vk::Result result = context->device.present.presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &signalSemaphoreInfo.semaphore,
            .swapchainCount = 1,
            .pSwapchains = swapchains,
            .pImageIndices = &frameContext->imageIndex
        })
    ) {
        case vk::Result::eSuccess:
            break;

        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR:
            LOG(WARNING, "Swapchain needs to be recreated");
            RecreateSwapchain();
            return;

        default:
            LOG(ERROR, "Failed to get next image index. Code: %d", result);
            break;
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void ee_vulkan::VulkanRenderer::ResizeWindow(const int width, const int height) {
    IRenderer::ResizeWindow(width, height);

    isDrawing = screenWidth > 0 && screenHeight > 0;
    if (!isDrawing) return;

    isSwapchainDirty = true;
    rmlContext->SetDimensions({width, height});
}

void ee_vulkan::VulkanRenderer::ApplyCamera() {
    viewProjection[0][1] *= -1.0f;
    viewProjection[1][1] *= -1.0f;
    viewProjection[2][1] *= -1.0f;
    viewProjection[3][1] *= -1.0f;

    viewProjectionInversed = glm::inverse(viewProjection);
}

void ee_vulkan::VulkanRenderer::UpdateProvinces(
    const std::size_t offset,
    const std::vector<ee_engine::ProvinceSSBO> &data
) {
    const std::size_t elementOffset = offset / provinces.storage->elementSize;
    std::ranges::copy(data, provinces.storage->content.data() + elementOffset);
    renderCommands->SubmitStagingBuffer(*provinces.storage);
}

void ee_vulkan::VulkanRenderer::RequestProvincePickAt(const std::optional<glm::vec2> worldPos) {
    if (pick.recorded.empty()) return;

    if (!worldPos) {
        hoveredProvince.reset();
        pick.requested.reset();
        pick.lastTexel.reset();
        std::ranges::fill(pick.recorded, false);
        return;
    }

    const glm::vec2 local =
        (*worldPos - glm::vec2(provinces.rasterOrigin)) / static_cast<float>(provinces.rasterTexelSize);
    const std::int64_t texelX = std::lround(local.x);
    const std::int64_t texelY = std::lround(local.y);
    if (texelX < 0 || texelY < 0 || texelX >= provinces.rasterSize.x || texelY >= provinces.rasterSize.y) {
        hoveredProvince.reset();
        pick.requested.reset();
        pick.lastTexel.reset();
        std::ranges::fill(pick.recorded, false);
        return;
    }

    const glm::ivec2 texel(static_cast<std::int32_t>(texelX), static_cast<std::int32_t>(texelY));
    if (pick.lastTexel == texel) return;

    pick.requested = texel;
    pick.lastTexel = texel;
}

void ee_vulkan::VulkanRenderer::RecreateSwapchain() {
    // We should not render with minimized window
    if (screenWidth == 0 || screenHeight == 0) return;

    context->device.handle.waitIdle();

    context->surfaceCapabilities.capabilities =
        context->physicalDevice.handle.getSurfaceCapabilitiesKHR(*surface.handle);
    surface.extent = {
        .width = static_cast<std::uint32_t>(screenWidth),
        .height = static_cast<std::uint32_t>(screenHeight)
    };

    frameContext.reset();
    renderPass.reset();
    depth.reset();

    swapchain.reset();
    swapchain.emplace(vulkan_render::ResourcesFactory::CreateSwapchain(*context, surface));
    depth.emplace(*context, surface);
    renderPass.emplace(*context, *swapchain, surface, *depth);

    syncManager->ResetSemaphores();
    syncManager->imagesInFlight.assign(MAX_FRAMES_IN_FLIGHT, nullptr);

    for (
        vulkan_render::Pipeline *pipeline : {
            &*provincesPipeline, &*rmlPipeline
        }
    ) {
        pipeline->viewport.width = static_cast<float>(surface.extent.width);
        pipeline->viewport.height = static_cast<float>(surface.extent.height);
        pipeline->scissors.offset = {0, 0};
        pipeline->scissors.extent = surface.extent;
    }

    isSwapchainDirty = false;
}

void ee_vulkan::VulkanRenderer::LoadProvincesMaterial() {
    const vulkan_render::ShaderBindings shaderBindings {
        .vertexBinding = {
            .binding = 0,
            .stride = sizeof(ProvinceVertex),
            .inputRate = vk::VertexInputRate::eVertex
        },
        .vertexAttributesCount = 2,
        .vertexAttributes = {
            {
                .location = 0,
                .binding = 0,
                .format = vulkan_render::ShaderAttributeFormat::VEC2,
                .offset = 0
            },
            {
                .location = 1,
                .binding = 0,
                .format = vulkan_render::ShaderAttributeFormat::VEC2,
                .offset = sizeof(glm::vec2)
            }
        }
    };

    const vk::DescriptorSetLayoutBinding mapData {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::Flags(vk::ShaderStageFlagBits::eVertex) | vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
    };
    const vk::DescriptorSetLayoutBinding provincesSSBO {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::Flags(vk::ShaderStageFlagBits::eVertex) | vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
    };
    const vk::DescriptorSetLayoutBinding provinceIds {
        .binding = 2,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
    };
    const vk::DescriptorSetLayoutBinding coastDistance {
        .binding = 3,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
    };

    const vk::PushConstantRange model {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(glm::mat4)
    };

    vulkan_render::PipelineConfig provincesMaterial = vulkan_render::Pipeline::MakeConfig(
        surface,
        shaderBindings,
        {mapData, provincesSSBO, provinceIds, coastDistance},
        model
    );

    ee_assets::Shader &vertProvinces = assetManager->Get<ee_assets::Shader>(shaders.vertProvinces);
    ee_assets::Shader &fragProvinces = assetManager->Get<ee_assets::Shader>(shaders.fragProvinces);

    vulkan_render::ShaderProgram provincesShader {
        .vertex = {reinterpret_cast<std::byte*>(vertProvinces.content), vertProvinces.size},
        .fragment = {reinterpret_cast<std::byte*>(fragProvinces.content), fragProvinces.size}
    };

    provincesPipeline.emplace(
        *context,
        surface,
        *swapchain,
        provincesMaterial,
        *renderPass,
        provincesShader,
        MAX_FRAMES_IN_FLIGHT
    );
}

void ee_vulkan::VulkanRenderer::LoadRmlMaterial() {
    const vulkan_render::ShaderBindings shaderBindings {
        .vertexBinding = {
            .binding = 0,
            .stride = ee_rml::RmlVertexLayout::stride,
            .inputRate = vk::VertexInputRate::eVertex
        },
        .vertexAttributesCount = 3,
        .vertexAttributes ={
            {
                .location = 0,
                .binding = 0,
                .format = vulkan_render::ShaderAttributeFormat::VEC2,
                .offset = ee_rml::RmlVertexLayout::offsetPos
            },
            {
                .location = 1,
                .binding = 0,
                .format = vulkan_render::ShaderAttributeFormat::UVEC4,
                .offset = ee_rml::RmlVertexLayout::offsetColor
            },
            {
                .location = 2,
                .binding = 0,
                .format = vulkan_render::ShaderAttributeFormat::VEC2,
                .offset = ee_rml::RmlVertexLayout::offsetUV
            }
        }
    };

    const vk::DescriptorSetLayoutBinding texture {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
    };

    const vk::PushConstantRange model {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(ee_rml::RmlTransformGeometry)
    };

    vulkan_render::PipelineConfig rmlMaterial = vulkan_render::Pipeline::MakeConfig(
        surface,
        shaderBindings,
        {texture},
        model
    );
    rmlMaterial.maxExtraDescriptorSets = 100; //TODO: remove magic const

    ee_assets::Shader &vertRml = assetManager->Get<ee_assets::Shader>(shaders.vertRml);
    ee_assets::Shader &fragRml = assetManager->Get<ee_assets::Shader>(shaders.fragRml);

    vulkan_render::ShaderProgram rmlShader {
        .vertex = {reinterpret_cast<std::byte*>(vertRml.content), vertRml.size},
        .fragment = {reinterpret_cast<std::byte*>(fragRml.content), fragRml.size}
    };

    rmlPipeline.emplace(
        *context,
        surface,
        *swapchain,
        rmlMaterial,
        *renderPass,
        rmlShader,
        MAX_FRAMES_IN_FLIGHT
    );
}

std::generator<float> ee_vulkan::VulkanRenderer::LoadProvinces() {
    // Map extent and number of provinces
    Bounds bounds;
    std::uint32_t provincesCount = 0;
    for (
        const std::vector<ee_engine::MapMesh::Province> &provs :
        mapMesh->regions | std::views::values
    ) {
        provincesCount += static_cast<std::uint32_t>(provs.size());
        for (const ee_engine::MapMesh::Province &province : provs) {
            for (const glm::vec2 &vertex : province.vertices) bounds.Add(vertex);
        }
    }

    const std::uint32_t maxDimension = std::min<std::uint32_t>(
        context->physicalDeviceCapabilities.properties.limits.maxImageDimension2D,
        16384
    );
    ProvinceRaster raster(bounds, ProvinceRaster::ChooseTexelSize(bounds, maxDimension, MAX_RASTER_TEXELS));
    provinces.oceanInstance = provincesCount;

    std::vector<ee_engine::ProvinceSSBO> provincesData;
    std::vector<ProvinceVertex> verticesData;
    std::vector<std::uint32_t> indicesData;
    std::vector<vk::DrawIndexedIndirectCommand> indirectCommandsData;
    provincesData.reserve(provincesCount + 1);
    indirectCommandsData.reserve(provincesCount + 1);

    // Ocean plane is the first command of the multi-draw, so provinces are drawn on top of it.
    // Its storage element is the last one to keep regions offsets unchanged.
    verticesData.append_range(std::array<ProvinceVertex, 4>{{
        {.position = {-OCEAN_PLANE_EXTENT, -OCEAN_PLANE_EXTENT}, .normal = {0.0f, 0.0f}},
        {.position = { OCEAN_PLANE_EXTENT, -OCEAN_PLANE_EXTENT}, .normal = {0.0f, 0.0f}},
        {.position = { OCEAN_PLANE_EXTENT,  OCEAN_PLANE_EXTENT}, .normal = {0.0f, 0.0f}},
        {.position = {-OCEAN_PLANE_EXTENT,  OCEAN_PLANE_EXTENT}, .normal = {0.0f, 0.0f}}
    }});
    indicesData.append_range(std::array<std::uint32_t, 6>{0, 1, 2, 0, 2, 3});
    indirectCommandsData.push_back({
        .indexCount = 6,
        .instanceCount = 1,
        .firstIndex = 0,
        .vertexOffset = 0,
        .firstInstance = provinces.oceanInstance
    });

    std::size_t regionOffset = 0;
    std::size_t regionsCount = 0;
    std::uint32_t indicesOffset = static_cast<std::uint32_t>(indicesData.size());
    std::int32_t verticesOffset = static_cast<std::int32_t>(verticesData.size());
    for (auto& [region, provs] : mapMesh->regions) {
        for (auto&[vertices, indices] : provs) {
            const auto instance = static_cast<std::uint32_t>(provincesData.size());

            indirectCommandsData.push_back({
                .indexCount = static_cast<std::uint32_t>(indices.size()),
                .instanceCount = 1,
                .firstIndex = indicesOffset,
                .vertexOffset = verticesOffset,
                .firstInstance = instance
            });

            const std::vector<glm::vec2> normals = ComputeOutlineNormals(vertices, indices);
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                verticesData.push_back({.position = vertices[i], .normal = normals[i]});
            }
            indicesData.append_range(indices);

            RasterizeProvince(raster, vertices, indices, instance + 1);

            ee_engine::ProvinceSSBO data{
                .regionId = region,
                .color = 0xFF0000
            };
            provincesData.push_back(data);

            indicesOffset += indices.size();
            verticesOffset += vertices.size();
        }

        regionsMapping[region] = {.offset = regionOffset, .provincesCount = provs.size()};
        regionOffset += provs.size() * sizeof(ee_engine::ProvinceSSBO);
        regionsCount++;

        if ((regionsCount & REGIONS_YIELD_STEP) == 0) {
            co_yield (static_cast<float>(regionsCount) / mapMesh->regions.size()) * 0.15f;
        }
    }

    provincesData.push_back({
        .regionId = OCEAN_REGION_ID,
        .color = 0
    });

    provinces.vertices.emplace(*context, verticesData.size());
    provinces.indices.emplace(*context, indicesData.size());
    provinces.storage.emplace(*context, provincesData.size());
    provinces.indirectCommands.emplace(*context, indirectCommandsData.size());

    std::ranges::copy(verticesData, provinces.vertices->content.data());
    std::ranges::copy(indicesData, provinces.indices->content.data());
    std::ranges::copy(provincesData, provinces.storage->content.data());
    std::ranges::copy(indirectCommandsData, provinces.indirectCommands->content.data());

    renderCommands->SubmitStagingBuffer(*provinces.vertices);
    renderCommands->SubmitStagingBuffer(*provinces.indices);
    renderCommands->SubmitStagingBuffer(*provinces.storage);
    renderCommands->SubmitStagingBuffer(*provinces.indirectCommands);

    // Pixel-exact province coverage and coastal distance field for the map shaders
    co_yield std::ranges::elements_of(ee_utils::ScaledCoroutine(
        CloseRasterGaps(raster, 3),
        0.15f, 0.66f
    ));
    co_yield std::ranges::elements_of(ee_utils::ScaledCoroutine(
        FillEnclosedWater(raster, 32),
        0.66f, 0.99f
    ));

    std::int32_t fieldFactor = 1;
    while (
        static_cast<std::uint64_t>((raster.size.x - 1) / fieldFactor + 1) *
        static_cast<std::uint64_t>((raster.size.y - 1) / fieldFactor + 1) > MAX_COAST_FIELD_TEXELS
    ) ++fieldFactor;

    glm::ivec2 fieldSize;
    const std::vector<float> coastField = MakeCoastDistanceField(raster, fieldFactor, fieldSize);

    provinces.rasterOrigin = raster.origin;
    provinces.rasterSize = glm::uvec2(raster.size);
    provinces.rasterTexelSize = static_cast<std::uint32_t>(raster.texelSize);
    provinces.coastFieldSize = glm::uvec2(fieldSize);
    provinces.coastFieldFactor = static_cast<std::uint32_t>(fieldFactor);

    LoadMapTextures(raster.ids, coastField, provincesCount);

    LOG(INFO, "Map loaded: %d provinces, raster %dx%d (%d map px per texel), coast field %dx%d.",
        static_cast<int>(provincesCount),
        raster.size.x, raster.size.y, raster.texelSize,
        fieldSize.x, fieldSize.y
    );

    isMapLoaded = true;
    co_yield 1.0f;
}

void ee_vulkan::VulkanRenderer::LoadMapTextures(
    const std::vector<std::uint32_t> &ids,
    const std::vector<float> &coastField,
    const std::uint32_t provincesCount
) {
    const glm::uvec2 size = provinces.rasterSize;
    const glm::uvec2 fieldSize = provinces.coastFieldSize;

    const auto upload = [this](vulkan_render::DataTexture &texture, const std::span<const std::byte> data) {
        const vulkan_render::Buffer<
            std::byte,
            vk::BufferUsageFlagBits::eTransferSrc,
            vulkan_render::BufferMode::STATIC
        > stagingBuffer(*context, data);

        renderCommands->TranslateTextureLayout(
            texture,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal
        );
        renderCommands->SubmitTexture(stagingBuffer, texture);
        renderCommands->TranslateTextureLayout(
            texture,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eReadOnlyOptimal
        );
        texture.Init(vk::ImageAspectFlagBits::eColor);
    };

    // Province ids, 16 bit when possible
    if (provincesCount < std::numeric_limits<std::uint16_t>::max()) {
        std::vector<std::uint16_t> packed(ids.size());
        std::ranges::transform(
            ids,
            packed.begin(),
            [](const std::uint32_t id) { return static_cast<std::uint16_t>(id); }
        );

        provinces.provinceIds.emplace(
            *context,
            size.x,
            size.y,
            vk::Format::eR16Uint,
            vk::ImageUsageFlagBits::eTransferSrc
        );
        upload(*provinces.provinceIds, std::as_bytes(std::span(packed)));
    }
    else {
        provinces.provinceIds.emplace(
            *context,
            size.x,
            size.y,
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eTransferSrc
        );
        upload(*provinces.provinceIds, std::as_bytes(std::span(ids)));
    }

    // Coast distance field, half floats
    {
        std::vector<std::uint16_t> packed(coastField.size());
        std::ranges::transform(
            coastField,
            packed.begin(),
            [](const float distance) { return glm::packHalf1x16(distance); }
        );

        provinces.coastDistance.emplace(*context, fieldSize.x, fieldSize.y, vk::Format::eR16Sfloat);
        upload(*provinces.coastDistance, std::as_bytes(std::span(packed)));
    }

    provinces.nearestSampler = context->device.handle.createSampler({
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::False,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False
    });
    provinces.linearSampler = context->device.handle.createSampler({
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::False,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eFloatOpaqueBlack,
        .unnormalizedCoordinates = vk::False
    });
}

void ee_vulkan::VulkanRenderer::UpdateData(const std::uint32_t frame) {
    const double seconds = glfwGetTime();
    const std::uint64_t elapsedMs = static_cast<std::uint64_t>(seconds * 1000.0);

    if (isMapLoaded) {
        MapUniform &mapData = mapUniform->buffers[frame].content[0];

        const float projectionScale = std::abs(viewProjection[0][0]) * static_cast<float>(surface.extent.width);
        mapData.viewProjection = viewProjection;
        mapData.inverseViewProjection = glm::inverse(viewProjection);
        mapData.viewport = glm::vec4(
            static_cast<float>(surface.extent.width),
            static_cast<float>(surface.extent.height),
            projectionScale > 0.0f ? 2.0f / projectionScale : 1.0f,
            IsSrgbFormat(surface.format.format) ? 1.0f : 0.0f
        );
        mapData.raster = glm::vec4(
            static_cast<float>(provinces.rasterOrigin.x),
            static_cast<float>(provinces.rasterOrigin.y),
            static_cast<float>(provinces.rasterSize.x),
            static_cast<float>(provinces.rasterSize.y)
        );
        mapData.coast = glm::vec4(
            static_cast<float>(provinces.rasterTexelSize),
            static_cast<float>(provinces.coastFieldFactor),
            static_cast<float>(provinces.coastFieldSize.x),
            static_cast<float>(provinces.coastFieldSize.y)
        );
        mapData.info = glm::uvec4(
            provinces.oceanInstance,
            static_cast<std::uint32_t>(elapsedMs % 60000),
            0,
            0
        );

        provincesPipeline->UpdateDescriptors({
            {
                .info = vk::DescriptorBufferInfo{
                    .buffer = mapUniform->buffers[frame].handle,
                    .offset = 0,
                    .range = mapUniform->buffers[frame].memorySize
                },
                .bindingPosition = 0,
                .type = vk::DescriptorType::eUniformBuffer
            },
            {
                .info = vk::DescriptorBufferInfo{
                    .buffer = provinces.storage->handle,
                    .offset = 0,
                    .range = provinces.storage->memorySize
                },
                .bindingPosition = 1,
                .type = vk::DescriptorType::eStorageBuffer
            },
            {
                .info = vk::DescriptorImageInfo{
                    .sampler = provinces.nearestSampler,
                    .imageView = provinces.provinceIds->view,
                    .imageLayout = vk::ImageLayout::eReadOnlyOptimal
                },
                .bindingPosition = 2,
                .type = vk::DescriptorType::eCombinedImageSampler
            },
            {
                .info = vk::DescriptorImageInfo{
                    .sampler = provinces.linearSampler,
                    .imageView = provinces.coastDistance->view,
                    .imageLayout = vk::ImageLayout::eReadOnlyOptimal
                },
                .bindingPosition = 3,
                .type = vk::DescriptorType::eCombinedImageSampler
            }
        }, frame);
    }
}

void ee_vulkan::VulkanRenderer::CreatePickReadback(const bool idsPacked) {
    pick.idsPacked = idsPacked;
    pick.recorded.assign(MAX_FRAMES_IN_FLIGHT, false);

    pick.buffer.emplace(*context, MAX_FRAMES_IN_FLIGHT);
    pick.commands.emplace(*context, MAX_FRAMES_IN_FLIGHT);
}

void ee_vulkan::VulkanRenderer::RecordPick(const std::uint32_t frame) {
    if (!pick.requested) return;

    const glm::ivec2 texel = *pick.requested;
    pick.requested.reset();

    vulkan_render::Command &pickCommand = pick.commands->buffers[frame];
    pickCommand.Reset();
    pickCommand.Begin();

    //TODO: simplify barriers using vulkan_render
    const vk::ImageMemoryBarrier2 toTransferSource {
        .srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
        .srcAccessMask = {},
        .dstStageMask = vk::PipelineStageFlagBits2::eCopy,
        .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
        .oldLayout = vk::ImageLayout::eReadOnlyOptimal,
        .newLayout = vk::ImageLayout::eTransferSrcOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = provinces.provinceIds->handle,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0, .levelCount = 1,
            .baseArrayLayer = 0, .layerCount = 1
        }
    };
    pickCommand.handle.pipelineBarrier2({
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &toTransferSource
    });

    const vk::BufferImageCopy region {
        .bufferOffset = frame * sizeof(std::uint32_t),
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1
        },
        .imageOffset = {texel.x, texel.y, 0},
        .imageExtent = {1, 1, 1}
    };
    pickCommand.handle.copyImageToBuffer(
        provinces.provinceIds->handle,
        vk::ImageLayout::eTransferSrcOptimal,
        pick.buffer->handle,
        {region}
    );

    const vk::ImageMemoryBarrier2 toShaderRead {
        .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
        .srcAccessMask = {},
        .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
        .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
        .newLayout = vk::ImageLayout::eReadOnlyOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = provinces.provinceIds->handle,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0, .levelCount = 1,
            .baseArrayLayer = 0, .layerCount = 1
        }
    };
    const vk::MemoryBarrier2 toHost {
        .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eHost,
        .dstAccessMask = vk::AccessFlagBits2::eHostRead
    };
    pickCommand.handle.pipelineBarrier2({
        .memoryBarrierCount = 1, .pMemoryBarriers = &toHost,
        .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &toShaderRead
    });

    pickCommand.End();
    pick.recorded[frame] = true;
}

void ee_vulkan::VulkanRenderer::ResolvePick(const std::uint32_t frame) {
    if (!pick.recorded[frame]) return;
    pick.recorded[frame] = false;

    std::uint32_t id = pick.buffer->content[frame];
    if (pick.idsPacked) id &= 0xFFFFu;

    if (id == 0) {
        hoveredProvince.reset();
        return;
    }

    const std::uint32_t provinceIndex = id - 1;
    hoveredProvince.emplace(ee_engine::ProvinceHit{
        .provinceIndex = provinceIndex,
        .regionId = provinces.storage->content[provinceIndex].regionId
    });
}


constexpr ee_engine::ProvinceSSBO& ee_vulkan::VulkanRenderer::getRegionData(const std::uint32_t id) const noexcept {
    const RegionRanges provinces = regionsMapping.at(id);
    return this->provinces.storage->content[provinces.offset / sizeof(ee_engine::ProvinceSSBO)];
}