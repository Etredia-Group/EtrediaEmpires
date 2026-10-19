export module Actions;
import Input;

export
{
    class Actions {
    public:
        static ee_engine::InputHandlers mappings;

        Actions() = delete;

    private:
        static void Zoom(float dx, float dy);
        static void MapScroll(float x, float y, float lastX, float lastY);
        static void MapHover(float x, float y);
        static void MapHoverEnd();
        static void Click(float x, float y);

    };
}
