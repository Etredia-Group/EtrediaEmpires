#pragma once
#include "AssetManager.hpp"
#include "RenderInterface.hpp"

class Engine
{
public:
    static AssetManager *assetManager;
    static IRenderer *renderer;

private:
    Engine();
};
