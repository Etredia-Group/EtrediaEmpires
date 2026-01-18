#pragma once

#include <ee_engine_api/RenderInterface.hpp>

#include "AssetManager.hpp"

class Engine
{
public:
    static AssetManager *assetManager;
    static IRenderer *renderer;

private:
    Engine();
};
