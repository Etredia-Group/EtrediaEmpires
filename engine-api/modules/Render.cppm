module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

export module Render;
import utils;
import views;
import AssetData;
import rml;
import std;

export namespace ee_engine {
    struct ShaderRefs {
        ee_assets::Shader *vertProvinces;
        ee_assets::Shader *fragProvinces;

        ee_assets::Shader *vertRml;
        ee_assets::Shader *fragRml;
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

    class IRenderer : public ee_utils::Worker<RendererCommand, RendererCommand::COMPLETED> {
    public:
        virtual ~IRenderer();

        constexpr bool isReady() const noexcept { return _isReady; }

        void RegisterMaterials(const ShaderRefs& shaders);
        void RegisterMapMesh(MapMesh *mapMesh);

        void SetUIContext(Rml::Context *rmlContext);
        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        void Draw();

        virtual void ResizeWindow(int width, int height);
        void SetCamera(const CameraTransform& camera);

        virtual constexpr Rml::RenderInterface* getRenderInterface() noexcept = 0;

        constexpr CameraTransform getCamera() const noexcept { return camera; };
        glm::vec2 ScreenToWorld(float screenX, float screenY) const;
        glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;

    protected:
        bool _isReady = false;
        bool isDrawing = false;
        int screenWidth, screenHeight;
        float screenAspect;
        CameraTransform camera;
        glm::mat4 viewProjection;
        glm::mat4 viewProjectionInversed;

        GLFWwindow *window;
        MapMesh *mapMesh;
        ShaderRefs shaders;

        Rml::Context *rmlContext;

        struct RegionRanges {
            std::size_t offset;
            std::size_t provincesCount;
        };

        std::unordered_map<std::uint32_t /*id*/, RegionRanges> regionsMapping;

        explicit IRenderer(GLFWwindow *window);

        virtual void DrawBegin() = 0;
        virtual void DrawMap() = 0;
        virtual void DrawUI() = 0;
        virtual void DrawEnd() = 0;

        virtual void ApplyCamera() = 0;

        virtual void UpdateProvinces(std::size_t offset, const std::vector<ProvinceSSBO>& data) = 0;

    private:
        void UpdateRegionCmd(RendererCommand& command);
    };
}
