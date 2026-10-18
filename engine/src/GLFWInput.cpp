module;

#include <GLFW/glfw3.h>

#define GLFW_GET_INPUT reinterpret_cast<GLFWInput*>(glfwGetWindowUserPointer(window))
#define GLFW_CALLBACK(callback, ...) GLFW_GET_INPUT->handlers.callback(__VA_ARGS__);

module GLFWInput;
import Engine;
import utils;
import std;

GLFWInput::GLFWInput(const ee_engine::InputHandlers &handlers) : IInput(handlers) {
    rmlSystemInterface.emplace(Engine::window);

    glfwSetWindowUserPointer(Engine::window, this);

    glfwSetScrollCallback(Engine::window, [](GLFWwindow *window, const double dx, const double dy) {
        GLFW_CALLBACK(zooming, static_cast<float>(dx), static_cast<float>(dy));
    });

    glfwSetCursorPosCallback(Engine::window, [](GLFWwindow *window, const double x, const double y) {
        if (const GLFWInput *input = GLFW_GET_INPUT; input->uiHandlers.mouseMove) {
            input->uiHandlers.mouseMove(static_cast<float>(x), static_cast<float>(y));
        }
    });

    glfwSetCursorEnterCallback(Engine::window, [](GLFWwindow *window, const int entered) {
        if (const GLFWInput *input = GLFW_GET_INPUT; !entered && input->uiHandlers.mouseLeave) {
            input->uiHandlers.mouseLeave();
        }
    });

    glfwSetMouseButtonCallback(
        Engine::window,
        [](GLFWwindow *window, const int button, const int action, int _) {
            GLFWInput *input = GLFW_GET_INPUT;
            if (button < 0 || button >= input->mouseUICaptured.size()) return;

            const bool pressed = action == GLFW_PRESS;

            const bool consumed = input->uiHandlers.mouseButton && input->uiHandlers.mouseButton(button, pressed);
            input->mouseUICaptured[button] = pressed ? consumed : false;
        }
    );

    glfwSetKeyCallback(
        Engine::window,
        [](GLFWwindow *window, const int key, int, const int action, int) {
            if (const GLFWInput *input = GLFW_GET_INPUT; input->uiHandlers.key) {
                bool _ = input->uiHandlers.key(key, action != GLFW_RELEASE);
            }
            // TODO
        }
    );

    glfwSetCharCallback(Engine::window, [](GLFWwindow *window, const unsigned int cp) {
        if (const GLFWInput *input = GLFW_GET_INPUT; input->uiHandlers.text) {
            bool _ = input->uiHandlers.text(cp);
        }
    });

    LOG(INFO, "Input mappings initialized");
}

void GLFWInput::Poll() {
    double mouseX, mouseY;
    glfwGetCursorPos(Engine::window, &mouseX, &mouseY);
    this->mouseX = static_cast<float>(mouseX);
    this->mouseY = static_cast<float>(mouseY);

    const std::uint32_t LMB = glfwGetMouseButton(Engine::window, GLFW_MOUSE_BUTTON_LEFT);
    const std::uint32_t RMB = glfwGetMouseButton(Engine::window, GLFW_MOUSE_BUTTON_RIGHT);
    const std::uint32_t MMB = glfwGetMouseButton(Engine::window, GLFW_MOUSE_BUTTON_MIDDLE);

    OnLeftMouseButton(LMB);
    OnRightMouseButton(RMB);
    OnMiddleMouseButton(MMB);

    lastMouseX = this->mouseX;
    lastMouseY = this->mouseY;
}

void GLFWInput::OnLeftMouseButton(std::uint32_t state) {}
void GLFWInput::OnRightMouseButton(std::uint32_t state) {}

void GLFWInput::OnMiddleMouseButton(const std::uint32_t state) const {
    if (mouseUICaptured[GLFW_MOUSE_BUTTON_MIDDLE]) return;

    switch (state) {
        case GLFW_PRESS:
        {
            handlers.mapScroll(mouseX, mouseY, lastMouseX, lastMouseY);
            break;
        }

        case GLFW_RELEASE:
        {
            break;
        }

        default: ;
    }
}
