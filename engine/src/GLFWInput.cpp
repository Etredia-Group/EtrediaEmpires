module;

#include "Logger.h"
#include <GLFW/glfw3.h>

#define GLFW_GET_INPUT reinterpret_cast<GLFWInput*>(glfwGetWindowUserPointer(window))
#define GLFW_CALLBACK(callback, ...) GLFW_GET_INPUT->handlers.callback(__VA_ARGS__);

module GLFWInput;
import Engine;

GLFWInput::GLFWInput(ee_engine::InputHandlers handlers) : ee_engine::IInput(handlers)
{
    glfwSetWindowUserPointer(Engine::window, this);
    glfwSetScrollCallback(Engine::window, [](GLFWwindow *window, double dx, double dy) {
        GLFW_CALLBACK(zooming, static_cast<float>(dx), static_cast<float>(dy));
    });

    LOG(INFO, "Input mappings initialized");
}

void GLFWInput::Poll()
{
    double mouseX, mouseY;
    glfwGetCursorPos(Engine::window, &mouseX, &mouseY);
    this->mouseX = static_cast<float>(mouseX);
    this->mouseY = static_cast<float>(mouseY);

    int LMB = glfwGetMouseButton(Engine::window, GLFW_MOUSE_BUTTON_LEFT);
    int RMB = glfwGetMouseButton(Engine::window, GLFW_MOUSE_BUTTON_RIGHT);
    int MMB = glfwGetMouseButton(Engine::window, GLFW_MOUSE_BUTTON_MIDDLE);

    OnLeftMouseButton(LMB);
    OnRightMouseButton(RMB);
    OnMiddleMouseButton(MMB);

    lastMouseX = this->mouseX;
    lastMouseY = this->mouseY;
}

void GLFWInput::OnLeftMouseButton(int state){}
void GLFWInput::OnRightMouseButton(int state){}

void GLFWInput::OnMiddleMouseButton(int state)
{
    switch(state)
    {
        case GLFW_PRESS:
        {
            handlers.mapScroll(mouseX, mouseY, lastMouseX, lastMouseY);
            break;
        }

        case GLFW_RELEASE:
        {
            break;
        }
    }
}