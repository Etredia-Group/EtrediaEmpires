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

            if (button == GLFW_MOUSE_BUTTON_LEFT && !input->mouseUICaptured[button]) {
                if (action == GLFW_RELEASE) {
                    GLFW_CALLBACK(click, input->mouseX, input->mouseY);
                }
            }
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

    if (glfwGetWindowAttrib(Engine::window, GLFW_HOVERED)) {
        auto [mouseX, mouseY] = WindowToFramebuffer(this->mouseX, this->mouseY);
        handlers.mapHover(mouseX, mouseY);
    }
    else {
        handlers.mapHoverEnd();
    }
}

void GLFWInput::OnLeftMouseButton(const std::uint32_t state) const {

}

void GLFWInput::OnRightMouseButton(const std::uint32_t state) const {
    if (mouseUICaptured[GLFW_MOUSE_BUTTON_RIGHT]) return;

    switch (state) {
        case GLFW_PRESS:
        {
            if (
                glfwGetKey(Engine::window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                glfwGetKey(Engine::window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS
            ) {
                handlers.mapScroll(mouseX, mouseY, lastMouseX, lastMouseY);
            }
            break;
        }
        default: ;
    }
}

void GLFWInput::OnMiddleMouseButton(const std::uint32_t state) const {
    if (mouseUICaptured[GLFW_MOUSE_BUTTON_MIDDLE]) return;

    switch (state) {
        case GLFW_PRESS:
        {
            handlers.mapScroll(mouseX, mouseY, lastMouseX, lastMouseY);
            break;
        }
        default: ;
    }
}

ee_engine::Pos GLFWInput::WindowToFramebuffer(float x, float y) {
    std::int32_t windowWidth, windowHeight;
    glfwGetWindowSize(Engine::window, &windowWidth, &windowHeight);
    std::int32_t framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(Engine::window, &framebufferWidth, &framebufferHeight);

    return {
        windowWidth > 0 ? x * static_cast<float>(framebufferWidth) / static_cast<float>(windowWidth) : x,
        windowHeight > 0 ? y * static_cast<float>(framebufferHeight) / static_cast<float>(windowHeight) : y
    };
}
