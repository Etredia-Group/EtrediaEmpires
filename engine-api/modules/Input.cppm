export module Input;
import std;

export namespace ee_engine
{
    struct InputHandlers
    {
        std::function<void(float dx, float dy)> zooming;
        std::function<void(float x, float y, float lastX, float lastY)> mapScroll;
    };

    class IInput
    {
    public:
        virtual ~IInput() {};
        virtual void Poll() = 0;

    protected:
        InputHandlers handlers;

        float mouseX = 0.0f, mouseY = 0.0f;
        float lastMouseX = 0.0f, lastMouseY = 0.0f;

        explicit IInput(const InputHandlers &handlers) {this->handlers = handlers;};
    };
}