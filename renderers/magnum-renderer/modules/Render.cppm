module;

#define NOMINMAX

#include <Magnum/Platform/GLContext.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Buffer.h>
#include <GLFW/glfw3.h>

export module MagnumRender;
    import :Materials;
import views;
import Render;
import std;

using namespace Magnum;

export namespace ee_magnum
{
    class MagnumRenderer final : public ee_engine::IRenderer
    {
    public:
        MagnumRenderer(GLFWwindow *window);
        ~MagnumRenderer();

        void Init() override;
        void Shutdown() override;

    protected:
        void Clear() override;
        void DrawMap() override;
        void DrawUI() override;

        void ApplyCamera() override;

        virtual void UpdateProvinces(const std::size_t index, const std::vector<ee_engine::ProvinceSSBO> &data) override;

    private:
        Platform::GLContext context;

        std::vector<GL::Mesh> provinceMeshes;
        GL::Buffer provinceSSBO;
        ProvinceMaterial *provinceMaterial;

        void ValidateSupport();
        void LoadShaders();
        void LoadProvinces();

        void ApplyMapMVP();
    };
}