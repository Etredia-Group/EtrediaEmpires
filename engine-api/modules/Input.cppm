export module Input;
import std;
import rml;

export namespace ee_engine {
    struct InputHandlers {
        std::function<void(float x, float y)> click;
        std::function<void(float dx, float dy)> zooming;
        std::function<void(float x, float y, float lastX, float lastY)> mapScroll;
        std::function<void(float x, float y)> mapHover;
        std::function<void()> mapHoverEnd;
    };

    struct UIInputHandlers {
        std::function<bool(float x, float y)> mouseMove;
        std::function<bool(std::int32_t button, bool pressed)> mouseButton;
        std::function<bool(float dx, float dy)> scroll;
        std::function<bool(std::int32_t key, bool pressed)> key;
        std::function<bool(std::uint32_t codepoint)> text;
        std::function<void()> mouseLeave;
    };

    using Pos = std::pair<float, float>;

    class IInput {
    public:
        virtual ~IInput() {};

        virtual void Poll() = 0;
        void SetUIHandlers(UIInputHandlers handlers) { uiHandlers = std::move(handlers); }
        virtual constexpr Rml::SystemInterface* getSystemInterface() noexcept = 0;

    protected:
        InputHandlers handlers;
        UIInputHandlers uiHandlers;
        std::array<bool, 8> mouseUICaptured {};

        float mouseX = 0.0f, mouseY = 0.0f;
        float lastMouseX = 0.0f, lastMouseY = 0.0f;

        explicit IInput(const InputHandlers& handlers) { this->handlers = handlers; };
    };
}
