#pragma once
#include <GLFW/glfw3.h>
#include "Map.hpp"
#include "AssetData.hpp"

struct ShaderRefs
{
    AssetShader *vertProvinces;
    AssetShader *fragProvinces;
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

class IRenderer
{
public:
    virtual ~IRenderer();

    void RegisterMaterials(const ShaderRefs &shaders);
    void RegisterMapMesh(MapMesh *mapMesh);
    
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    void Draw();
    
    virtual void ResizeWindow(int width, int height);
    void SetCamera(const CameraTransform &camera);
    void SetMapTransform(const MeshTransform &mapTransform);

protected:
    int screenWidth, screenHeight;
    float screenAspect;
    CameraTransform camera;
    MeshTransform mapTransform;
    glm::mat4 mapModel;
    glm::mat4 viewProjection;

    GLFWwindow *window;
    MapMesh *mapMesh;
    ShaderRefs shaders;

    explicit IRenderer(GLFWwindow *window);

    virtual void Clear() = 0;
    virtual void DrawMap() = 0;
    virtual void DrawUI() = 0;

    virtual void ApplyCamera() = 0;
    virtual void ApplyMapTransform() = 0;
};