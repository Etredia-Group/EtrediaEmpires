module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

export module Render;
import utils;
import views;
import AssetManager;
import rml;
import std;

export namespace ee_engine {
    struct ShaderRefs {
        AssetHandle vertProvinces;
        AssetHandle fragProvinces;

        AssetHandle vertRml;
        AssetHandle fragRml;
    };

    struct CameraTransform {
        glm::vec2 position{0.0f, 0.0f};
        float zoom = 1.0f;
    };

    struct MeshTransform {
        glm::vec2 position{0.0f, 0.0f};
        glm::vec2 scale{1.0f, 1.0f};
        float rotation;
    };

    struct RendererCommand : ee_utils::WorkerCommand {
        enum Type : std::uint8_t {
            NONE,

            UPDATE_REGION,

            COMPLETED
        };

        struct RegionPayload {
            std::uint32_t id;
            ProvinceSSBO data;
        };

        union {
            RegionPayload region;
        };
    };

    struct ProvinceHit {
        std::uint32_t provinceIndex;
        std::uint32_t regionId;
    };

    class IRenderer : public ee_utils::Worker<RendererCommand, RendererCommand::COMPLETED> {
    public:
        virtual ~IRenderer();

        constexpr bool isReady() const noexcept { return _isReady; }

        void RegisterAssetManager(AssetManager &assetManager);
        void RegisterMaterials(const ShaderRefs &shaders);
        void RegisterMapMesh(MapMesh *mapMesh);

        void SetUIContext(Rml::Context *rmlContext);
        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        void Draw();

        virtual void ResizeWindow(int width, int height);
        void SetCamera(const CameraTransform &camera);
        void RequestProvincePick(float screenX, float screenY);
        void ClearProvincePick();

        virtual constexpr Rml::RenderInterface* getRenderInterface() noexcept = 0;
        virtual constexpr ProvinceSSBO &getRegionData(std::uint32_t id) const noexcept = 0;

        constexpr CameraTransform getCamera() const noexcept { return camera; };
        constexpr std::optional<ProvinceHit> getHoveredProvince() const noexcept { return hoveredProvince; }
        glm::vec2 ScreenToWorld(float screenX, float screenY) const;
        glm::vec2 ScreenToWorld(const glm::vec2 &screenPos) const;

    protected:
        bool _isReady = false;
        bool isDrawing = false;
        int screenWidth, screenHeight;
        float screenAspect;
        CameraTransform camera;
        glm::mat4 viewProjection;
        glm::mat4 viewProjectionInversed;

        GLFWwindow *window;
        AssetManager *assetManager;
        MapMesh *mapMesh;
        ShaderRefs shaders;

        Rml::Context *rmlContext;

        struct RegionRanges {
            std::size_t offset;
            std::size_t provincesCount;
        };

        std::unordered_map<std::uint32_t /*id*/, RegionRanges> regionsMapping;
        std::optional<ProvinceHit> hoveredProvince;

        explicit IRenderer(GLFWwindow *window);

        virtual bool DrawBegin() = 0;
        virtual void DrawMap() = 0;
        virtual void DrawUI() = 0;
        virtual void DrawEnd() = 0;

        virtual void ApplyCamera() = 0;

        virtual void UpdateProvinces(std::size_t offset, const std::vector<ProvinceSSBO> &data) = 0;
        virtual void RequestProvincePickAt(std::optional<glm::vec2> worldPos) = 0;

    private:
        void UpdateRegionCmd(RendererCommand &command);
    };
}
