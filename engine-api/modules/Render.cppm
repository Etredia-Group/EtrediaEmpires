module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cstddef>

export module Render;
import worker;
import MapView;
import AssetData;

export namespace ee_engine
{
    struct ShaderRefs
    {
        ee_assets::AssetShader *vertProvinces;
        ee_assets::AssetShader *fragProvinces;
    };

    struct CameraTransform
    {
        glm::vec2 position {0.0f, 0.0f};
        float zoom = 1.0f;
    };

    struct MeshTransform
    {
        glm::vec2 position {0.0f, 0.0f};
        glm::vec2 scale {1.0f, 1.0f};
        float rotation;
    };

    struct RendererCommand : public ee_utils::WorkerCommand
    {
        enum Type : std::uint8_t
        {
            NONE,

            UPDATE_REGION,

            COMPLETED
        };

        struct RegionPayload { std::uint32_t id; ProvinceSSBO data; };

        union {
            RegionPayload region;
        };
    };

    class IRenderer : public ee_utils::Worker<RendererCommand, RendererCommand::COMPLETED>
    {
    public:
        virtual ~IRenderer();

        constexpr bool isReady() const noexcept {return _isReady;}

        void RegisterMaterials(const ShaderRefs &shaders);
        void RegisterMapMesh(MapMesh *mapMesh);

        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        void Draw();

        virtual void ResizeWindow(int width, int height);
        void SetCamera(const CameraTransform &camera);

        constexpr CameraTransform getCamera() const noexcept {return camera;};
        glm::vec2 ScreenToWorld(const float screenx, const float screenY);
        glm::vec2 ScreenToWorld(const glm::vec2 &screenPos);


    protected:
        bool _isReady;
        int screenWidth, screenHeight;
        float screenAspect;
        CameraTransform camera;
        glm::mat4 viewProjection;
        glm::mat4 viewProjectionInversed;

        GLFWwindow *window;
        MapMesh *mapMesh;
        ShaderRefs shaders;

        struct RegionIndecies {std::size_t startIndex; std::size_t provincesCount;};
        std::unordered_map<std::uint32_t /*id*/, RegionIndecies> regionsMapping;

        explicit IRenderer(GLFWwindow *window);

        virtual void Clear() = 0;
        virtual void DrawMap() = 0;
        virtual void DrawUI() = 0;

        virtual void ApplyCamera() = 0;

        virtual void UpdateProvinces(const std::size_t index, const std::vector<ee_engine::ProvinceSSBO> &data) = 0;

    private:
        void UpdateRegionCmd(RendererCommand &command);
    };
}