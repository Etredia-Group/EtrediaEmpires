module;

#include <GLFW/glfw3.h>

export module Engine;
export import Render;
export import Input;
export import UI;
export import AssetManager;
export import LoadScreen;
export import Localizer;
export import Client;

import utils;

export namespace ee_engine
{
    class Engine {
    public:
        inline static GLFWwindow *window;
        inline static IInput *input;
        inline static AssetManager *assetManager;
        inline static IRenderer *renderer;
        inline static UI *ui;
        inline static LoadScreen *loadScreen;
        inline static Localizer *localizer;
        inline static Client *client;

        static void Shutdown() {
            renderer->Shutdown();

            SystemShutdown(loadScreen);
            SystemShutdown(input);
            SystemShutdown(ui);
            SystemShutdown(localizer);
            SystemShutdown(renderer);

            WaitThreadShutdown(assetManager);
            WaitThreadShutdown(client);

            glfwDestroyWindow(window);
            glfwTerminate();
        }

    private:

        template <typename T>
        static void SystemShutdown(T *&system) {
            if (system != nullptr) {
                Deallocate(system);
            }
            system = nullptr;
        }

        template <ee_utils::IThreadedWorker T>
        static void WaitThreadShutdown(T *&thread) {
            if (thread != nullptr) {
                thread->Shutdown();
                while (thread->isRunning.load()) {}
                Deallocate(thread);
            }
            thread = nullptr;
        }

        Engine() = delete;
    };
}
