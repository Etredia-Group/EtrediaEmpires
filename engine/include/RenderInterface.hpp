#pragma once
#include <thread>
#include <atomic>
#include <Boost/lockfree/spsc_queue.hpp>
#include <GLFW/glfw3.h>

struct RenderCommand
{
    enum Type
    {
        NONE,

        RESIZE_WINDOW,
        SHUTDOWN
    } type = NONE;
};

class IRenderer
{
public:
    IRenderer(GLFWwindow *window);
    ~IRenderer();
    
    void Start();
    void Shutdown();
    
    bool GetThreadStatus();
    bool GetDrawStatus();

    void ResizeWindow(int width, int height);

private:
    std::atomic<bool> isRunning;
    std::atomic<bool> isDrawing;
    std::atomic<int> screenWidth, screenHeight;

    std::stop_token *stoken;
    std::jthread renderThread;
    boost::lockfree::spsc_queue<RenderCommand, boost::lockfree::capacity<512>> commands;

    GLFWwindow *window;

    virtual void Draw() = 0;

    void Threading();
    void Request(const RenderCommand &command);
    void Request(const RenderCommand::Type &type);
    virtual void Init() = 0;
    virtual void Dispose() = 0;
    void Loop();
    void ProcessCommand(const RenderCommand &command);
    void Release();
};