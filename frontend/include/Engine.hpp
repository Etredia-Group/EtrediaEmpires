#pragma once
#include "AssetManager.hpp"
#include "Renderer.hpp"

class Engine
{
public:
    static AssetManager *assetManager;
    static Renderer *renderer;

private:
    Engine();
};
