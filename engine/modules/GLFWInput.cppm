module;

#include <GLFW/glfw3.h>

export module GLFWInput;
import Input;

export
{
    class GLFWInput final : public ee_engine::IInput {
    public:
        GLFWInput(ee_engine::InputHandlers handlers);

        void Poll() override;

    private:
        void OnLeftMouseButton(int state);
        void OnRightMouseButton(int state);
        void OnMiddleMouseButton(int state);
    };
}
