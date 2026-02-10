module;

#include <functional>

export module Input;

export namespace ee_engine
{
    struct InputHandlers
    {
        std::function<void(float dx, float dy)> zooming;
        std::function<void(float dx, float dy)> mapScroll;
    };

    class IInput
    {
    public:
        virtual ~IInput() {};
        virtual void Poll() = 0;

    protected:
        InputHandlers handlers;

        explicit IInput(const InputHandlers &handlers) {this->handlers = handlers;};
    };
}