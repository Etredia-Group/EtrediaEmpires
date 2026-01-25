#include "GLFWInput.hpp"
#include "Engine.hpp"
#include "Logger.h"

GLFWInput::GLFWInput(ee_engine::InputHandlers handlers) : ee_engine::IInput(handlers)
{
    glfwSetWindowUserPointer(Engine::window, this);

    glfwSetScrollCallback(Engine::window, [](GLFWwindow *window, double dx, double dy)  {
        reinterpret_cast<GLFWInput*>(glfwGetWindowUserPointer(window))->
            handlers.zooming(static_cast<float>(dx), static_cast<float>(dy));
    });

    LOG(INFO, "Input mappings initialized");
}

void GLFWInput::Poll()
{

}