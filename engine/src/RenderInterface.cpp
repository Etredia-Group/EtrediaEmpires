#include "RenderInterface.hpp"
#include "Logger.h"
#include <allocator.hpp>
#include <stdexcept>

IRenderer::IRenderer(GLFWwindow *window) :
	isRunning(false),
	isDrawing(false)
{
    this->window = window;
	int width = -1, height = -1;
	glfwGetFramebufferSize(window, &width, &height);
	screenWidth.store(width);
	screenHeight.store(height);
}

IRenderer::~IRenderer()
{
	Release();
}

void IRenderer::Start()
{
	if (isRunning)
	{
		LOG(CRITICAL, "Renderer can be started only once");
		throw std::logic_error("Renderer can be started only once");
	}

    isRunning.store(true);
	isDrawing.store(true);
    renderThread = std::jthread([this](std::stop_token stop) {
        this->stoken = &stop;
        Threading();
    });
}

void IRenderer::Threading()
{
    Logger::trace->Register_Thread(L"Renderer", 0);
    LOG(INFO, "Renderer thread started");

	Init();
    Loop();
    Release();
}


void IRenderer::Loop()
{
	RenderCommand commandBuffer;
    while (!stoken->stop_requested())
    {
		while (commands.pop(commandBuffer))
		{
			ProcessCommand(commandBuffer);
		}

		Draw();
    }
}

void IRenderer::ProcessCommand(const RenderCommand &command)
{
	switch(command.type)
	{
		case RenderCommand::SHUTDOWN:
		{
			LOG(INFO, "SHUTDOWN command");
			Shutdown();
			break;
		}
		
		case RenderCommand::RESIZE_WINDOW:
		{
			LOG(INFO, "RESIZE_WINDOW command");

			break;
		}

		default:
		{
			LOG(WARNING, "Unhandled command. Type id: %d", command.type);
			break;
		}
	}
}

void IRenderer::Shutdown()
{
    renderThread.request_stop();
}

void IRenderer::Request(const RenderCommand &command)
{
	commands.push(command);
}

void IRenderer::Request(const RenderCommand::Type &type)
{
	commands.push(RenderCommand {.type = type});
}

void IRenderer::ResizeWindow(int width, int height)
{
	screenWidth.store(width);
	screenHeight.store(height);
	Request(RenderCommand::RESIZE_WINDOW);
}

void IRenderer::Release()
{
	Dispose();
    LOG(INFO, "Renderer is released");
    isRunning.store(false);
}

bool IRenderer::GetThreadStatus() { return isRunning.load(); }
bool IRenderer::GetDrawStatus() { return isDrawing.load(); }
