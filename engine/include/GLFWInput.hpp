#pragma once
#include <ee_engine_api/InputInterface.hpp>
#include <GLFW/glfw3.h>

class GLFWInput : public ee_engine::IInput
{
public:
    GLFWInput(ee_engine::InputHandlers handlers);
    void Poll() override;
};