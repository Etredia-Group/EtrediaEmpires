module;

#include <GLFW/glfw3.h>

export module GLFWInput;
import Input;
import rml;
import rmlui_glfw;
import std;

export
{
    class GLFWInput final : public ee_engine::IInput {
    public:
        explicit GLFWInput(const ee_engine::InputHandlers &handlers);

        void Poll() override;

        constexpr Rml::SystemInterface* getSystemInterface() noexcept override {return &*rmlSystemInterface;}

    private:
        std::optional<ee_rml::RmlGLFW> rmlSystemInterface;

        void OnLeftMouseButton(std::uint32_t state);
        void OnRightMouseButton(std::uint32_t state);
        void OnMiddleMouseButton(std::uint32_t state) const;
    };
}