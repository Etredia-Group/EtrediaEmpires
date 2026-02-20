module;

#include <GLFW/glfw3.h>

export module Engine;
export import Render;
export import Input;
export import AssetManager;
export import Client;

import utils;

export
{
    class Engine {
    public:
        inline static GLFWwindow *window;
        inline static ee_engine::IInput *input;
        inline static AssetManager *assetManager;
        inline static ee_engine::IRenderer *renderer;
        inline static Client *client;

        static void Shutdown() {
            if (input != nullptr)
                Deallocate(input);

            if (renderer != nullptr) {
                renderer->Shutdown();
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

    private:
        Engine();
    };
}
