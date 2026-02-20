module;

#define NOMINMAX

#include <Magnum/Platform/GLContext.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Shaders/VertexColorGL.h>
#include <Corrade/Utility/Debug.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

module MagnumRender;
import utils;

using namespace Math::Literals;

ee_magnum::MagnumRenderer::MagnumRenderer(GLFWwindow *window) : IRenderer(window) {
    ValidateSupport();

    LOG(INFO, "Renderer created");
}

void ee_magnum::MagnumRenderer::Init() {
    LOG(INFO, "GLSL version: %d", GL::Context::current().version());

    LoadShaders();
    LoadProvinces();

    _isReady = true;
    LOG(INFO, "Renderer initialized");
}

void ee_magnum::MagnumRenderer::Clear() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);
}

void ee_magnum::MagnumRenderer::DrawMap() {
    ApplyCamera();
    for (std::uint32_t i = 0; i < provinceMeshes.size(); i++) {
        provinceMaterial->setId(i);
        provinceMaterial->draw(provinceMeshes[i]);
    }
}

void ee_magnum::MagnumRenderer::DrawUI() {}

void ee_magnum::MagnumRenderer::ValidateSupport() {}

void ee_magnum::MagnumRenderer::ApplyCamera() {
    Matrix4 viewProjection{Magnum::NoInit};
    std::memcpy(viewProjection.data(), glm::value_ptr(this->viewProjection), sizeof(float) * 16);

    provinceMaterial->setViewProjection(viewProjection);
}

void ee_magnum::MagnumRenderer::LoadShaders() {
    provinceMaterial = Allocate<ProvinceMaterial>(std::string(
                                                      reinterpret_cast<const char*>(shaders.vertProvinces->content),
                                                      shaders.vertProvinces->size
                                                  ) + '\0', std::string(
                                                      reinterpret_cast<const char*>(shaders.fragProvinces->content),
                                                      shaders.fragProvinces->size
                                                  )) + '\0';

    LOG(INFO, "Shaders loaded");
}

void ee_magnum::MagnumRenderer::LoadProvinces() {
    std::vector<ee_engine::ProvinceSSBO> provincesData;

    std::size_t regionIndex = 0;
    for (auto& [region, provs] : mapMesh->regions) {
        for (ee_engine::MapMesh::Province& province : provs) {
            GL::Mesh regionMesh;

            Containers::ArrayView provinceVertices(province.vertices.data(), province.vertices.size());
            Containers::ArrayView provinceIndices(province.indices.data(), province.indices.size());

            regionMesh.setPrimitive(GL::MeshPrimitive::Triangles)
                      .setCount(static_cast<Magnum::Int>(province.indices.size()))
                      .addVertexBuffer(GL::Buffer{provinceVertices}, 0, GL::Attribute<0, Magnum::Vector2>{})
                      .setIndexBuffer(GL::Buffer{provinceIndices}, 0, GL::MeshIndexType::UnsignedShort);

            provinceMeshes.push_back(std::move(regionMesh));

            ee_engine::ProvinceSSBO data{
                .regionId = region,
                .color = 0xFFFFFF
            };

            provincesData.push_back(data);
        }

        regionsMapping[region] = {.startIndex = regionIndex, .provincesCount = provs.size()};
        regionIndex += provs.size() * sizeof(ee_engine::ProvinceSSBO);
    }

    LOG(INFO, "Registered %d province meshes in renderer", provinceMeshes.size());

    provinceSSBO.setTargetHint(GL::Buffer::TargetHint::ShaderStorage);
    Containers::ArrayView provincesDataView(provincesData.data(), provincesData.size());
    provinceSSBO.setData(provincesDataView, GL::BufferUsage::DynamicDraw);
    provinceSSBO.bind(GL::Buffer::Target::ShaderStorage, ProvinceMaterial::PROVINCES_DATA_BINDING);
}

void ee_magnum::MagnumRenderer::UpdateProvinces(const std::size_t index,
                                                const std::vector<ee_engine::ProvinceSSBO>& data) {
    Containers::ArrayView provincesData(data.data(), data.size());
    provinceSSBO.setSubData(index, provincesData);
}

void ee_magnum::MagnumRenderer::Shutdown() {
    provinceMeshes.clear();
    provinceSSBO = GL::Buffer{NoCreate};
    if (provinceMaterial != nullptr)
        Deallocate(provinceMaterial);
    provinceMaterial = nullptr;
}

ee_magnum::MagnumRenderer::~MagnumRenderer() {}
