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

        void OnLeftMouseButton(std::uint32_t state) const;
        void OnRightMouseButton(std::uint32_t state) const;
        void OnMiddleMouseButton(std::uint32_t state) const;

        static ee_engine::Pos WindowToFramebuffer(float x, float y);
    };
}