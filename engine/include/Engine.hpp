#pragma once

#include <GLFW/glfw3.h>
#include <ee_engine_api/RenderInterface.hpp>
#include <ee_engine_api/InputInterface.hpp>
#include "AssetManager.hpp"

class Engine
{
public:
    static GLFWwindow *window;
    static ee_engine::IInput *input;
    static AssetManager *assetManager;
    static ee_engine::IRenderer *renderer;

    static void Shutdown();

private:
    Engine();
};
