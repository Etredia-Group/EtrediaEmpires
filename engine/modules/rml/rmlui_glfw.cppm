module;

#include <GLFW/glfw3.h>

export module rmlui_glfw;
import Input;
import rml;
import std;

namespace ee_rml {
    std::int32_t Modifiers(GLFWwindow *window) {
        auto isKeyDown = [&](const std::int32_t key) { return glfwGetKey(window, key) == GLFW_PRESS; };

        int m = 0;
        if (isKeyDown(GLFW_KEY_LEFT_CONTROL) || isKeyDown(GLFW_KEY_RIGHT_CONTROL))
            m |= Rml::Input::KeyModifier::KM_CTRL;
        if (isKeyDown(GLFW_KEY_LEFT_SHIFT) || isKeyDown(GLFW_KEY_RIGHT_SHIFT))
            m |= Rml::Input::KM_SHIFT;
        if (isKeyDown(GLFW_KEY_LEFT_ALT) || isKeyDown(GLFW_KEY_RIGHT_ALT))
            m |= Rml::Input::KM_ALT;
        if (isKeyDown(GLFW_KEY_LEFT_SUPER) || isKeyDown(GLFW_KEY_RIGHT_SUPER))
            m |= Rml::Input::KM_META;
        return m;
    }

    Rml::Input::KeyIdentifier ConvertKey(const std::uint32_t key) {
        using namespace Rml::Input;
        auto offsetFrom = [](const KeyIdentifier base, const std::int32_t delta) {
            return static_cast<KeyIdentifier>(static_cast<int>(base) + delta);
        };

        if (key >= GLFW_KEY_A  && key <= GLFW_KEY_Z)  return offsetFrom(KeyIdentifier::KI_A, key - GLFW_KEY_A);
        if (key >= GLFW_KEY_0  && key <= GLFW_KEY_9)  return offsetFrom(KI_0, key - GLFW_KEY_0);
        if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) return offsetFrom(KI_F1, key - GLFW_KEY_F1);
        if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) return offsetFrom(KI_NUMPAD0, key - GLFW_KEY_KP_0);

        switch (key) {
        case GLFW_KEY_SPACE:      return KI_SPACE;
        case GLFW_KEY_ENTER:      return KI_RETURN;
        case GLFW_KEY_KP_ENTER:   return KI_NUMPADENTER;
        case GLFW_KEY_ESCAPE:     return KI_ESCAPE;
        case GLFW_KEY_TAB:        return KI_TAB;
        case GLFW_KEY_BACKSPACE:  return KI_BACK;
        case GLFW_KEY_DELETE:     return KI_DELETE;
        case GLFW_KEY_INSERT:     return KI_INSERT;
        case GLFW_KEY_HOME:       return KI_HOME;
        case GLFW_KEY_END:        return KI_END;
        case GLFW_KEY_PAGE_UP:    return KI_PRIOR;
        case GLFW_KEY_PAGE_DOWN:  return KI_NEXT;
        case GLFW_KEY_LEFT:       return KI_LEFT;
        case GLFW_KEY_RIGHT:      return KI_RIGHT;
        case GLFW_KEY_UP:         return KI_UP;
        case GLFW_KEY_DOWN:       return KI_DOWN;
        case GLFW_KEY_LEFT_SHIFT:    return KI_LSHIFT;
        case GLFW_KEY_RIGHT_SHIFT:   return KI_RSHIFT;
        case GLFW_KEY_LEFT_CONTROL:  return KI_LCONTROL;
        case GLFW_KEY_RIGHT_CONTROL: return KI_RCONTROL;
        case GLFW_KEY_LEFT_ALT:      return KI_LMENU;
        case GLFW_KEY_RIGHT_ALT:     return KI_RMENU;
        case GLFW_KEY_SEMICOLON:     return KI_OEM_1;
        case GLFW_KEY_EQUAL:         return KI_OEM_PLUS;
        case GLFW_KEY_COMMA:         return KI_OEM_COMMA;
        case GLFW_KEY_MINUS:         return KI_OEM_MINUS;
        case GLFW_KEY_PERIOD:        return KI_OEM_PERIOD;
        case GLFW_KEY_SLASH:         return KI_OEM_2;
        case GLFW_KEY_GRAVE_ACCENT:  return KI_OEM_3;
        case GLFW_KEY_LEFT_BRACKET:  return KI_OEM_4;
        case GLFW_KEY_BACKSLASH:     return KI_OEM_5;
        case GLFW_KEY_RIGHT_BRACKET: return KI_OEM_6;
        case GLFW_KEY_APOSTROPHE:    return KI_OEM_7;
        default:                     return KI_UNKNOWN;
        }
    }
}

export namespace ee_rml {

    ee_engine::UIInputHandlers MakeInputHandlers(Rml::Context *ctx, GLFWwindow *window) {
        std::int32_t windowWidth, windowHieght;
        glfwGetWindowSize(window, &windowWidth, &windowHieght);

        std::int32_t framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        const float scaleX = windowWidth > 0 ? static_cast<float>(framebufferWidth) / windowWidth : 1.0f;
        const float scaleY = windowHieght > 0 ? static_cast<float>(framebufferHeight) / windowHieght : 1.0f;

        return {
            .mouseMove = [=](const float x, const float y) {
                return !ctx->ProcessMouseMove(
                    static_cast<int32_t>(x * scaleX),
                    static_cast<int32_t>(y * scaleY),
                    Modifiers(window)
                );
            },
            .mouseButton = [=](const std::int32_t button, const bool pressed) {
                if (pressed) {
                    return !ctx->ProcessMouseButtonDown(button, Modifiers(window));
                }
                return !ctx->ProcessMouseButtonUp(button, Modifiers(window));
            },
            .scroll = [=](const float dx, const float dy) {
                return !ctx->ProcessMouseWheel(Rml::Vector2f{-dx, -dy}, Modifiers(window));
            },
            .key = [=](const std::int32_t key, const bool pressed) {
                const auto id = ConvertKey(key);
                if (pressed) {
                    return !ctx->ProcessKeyDown(id, Modifiers(window));
                }
                return !ctx->ProcessKeyUp(id, Modifiers(window));
            },
            .text = [=](const std::uint32_t cp) {
                return !ctx->ProcessTextInput(static_cast<Rml::Character>(cp));
            },
            .mouseLeave = [=] { ctx->ProcessMouseLeave(); },
        };
    }

    class RmlGLFW final : public Rml::SystemInterface {
    public:
        explicit RmlGLFW(GLFWwindow *window) : window(window){}

        double GetElapsedTime() override { return glfwGetTime(); }

        void SetClipboardText(const Rml::String &text) override {
            glfwSetClipboardString(window, text.c_str());
        }

        void GetClipboardText(Rml::String &text) override {
            const char *str = glfwGetClipboardString(window);
            text = str ? str : "";
        }

    private:
        GLFWwindow *window;
    };

}