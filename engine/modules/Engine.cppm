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

export
{
    class Engine {
    public:
        inline static GLFWwindow *window;
        inline static ee_engine::IInput *input;
        inline static ee_engine::AssetManager *assetManager;
        inline static ee_engine::IRenderer *renderer;
        inline static UI *ui;
        inline static LoadScreen *loadScreen;
        inline static Localizer *localizer;
        inline static Client *client;

        static void Shutdown() {
            renderer->Shutdown();

            if (loadScreen != nullptr) {
                Deallocate(loadScreen);
            }
            loadScreen = nullptr;

            if (input != nullptr)
                Deallocate(input);
            input = nullptr;

            if (ui != nullptr) {
                Deallocate(ui);
            }
            ui = nullptr;

            if (localizer != nullptr) {
                Deallocate(localizer);
            }
            localizer = nullptr;

            if (renderer != nullptr) {
                Deallocate(renderer);
            }
            renderer = nullptr;

            WaitThreadShutdown(assetManager);
            WaitThreadShutdown(client);

            glfwDestroyWindow(window);
            glfwTerminate();
        }

        template <ee_utils::IThreadedWorker T>
        static void WaitThreadShutdown(T * & thread) {
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
