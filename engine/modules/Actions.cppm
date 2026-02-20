export module Actions;
import Input;

export
{
    class Actions {
    public:
        static ee_engine::InputHandlers mappings;

    private:
        static void Zoom(float dx, float dy);
        static void MapScroll(float x, float y, float lastX, float lastY);

        Actions();
    };
}
