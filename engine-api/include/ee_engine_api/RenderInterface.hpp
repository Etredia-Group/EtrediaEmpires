#pragma once
#include <GLFW/glfw3.h>

class IRenderer
{
public:
    virtual ~IRenderer();

    void Draw();
    virtual void ResizeWindow(int width, int height);

protected:
    int screenWidth, screenHeight;

    GLFWwindow *window;

    explicit IRenderer(GLFWwindow *window);

    virtual void Clear() = 0;
    virtual void DrawMap() = 0;
    virtual void DrawUI() = 0;
};