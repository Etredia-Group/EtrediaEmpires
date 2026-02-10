module;

#include <GLFW/glfw3.h>

export module GLFWInput;
import Input;

export {
    class GLFWInput : public ee_engine::IInput
    {
    public:
        GLFWInput(ee_engine::InputHandlers handlers);

        void Poll() override;
    };
}