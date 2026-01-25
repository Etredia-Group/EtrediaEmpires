#pragma once
#include <ee_engine_api/InputInterface.hpp>

class Actions
{
public:
    static ee_engine::InputHandlers mappings;

private:
    static void Zoom(float dx, float dy);
    static void MapScroll(float dx, float dy);

    Actions();
};