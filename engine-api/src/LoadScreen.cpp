module;

#include <GLFW/glfw3.h>

module LoadScreen;
import Engine;
import std;


ee_engine::LoadScreen::LoadScreen(const Context &context) :
    context(context),
    loadingInfo(&Engine::ui->GetController<view::LoadingInfo>()) {}

void ee_engine::LoadScreen::Run(std::span<const LoadStage> queue, const std::string &targetScene) const {
    using clock = std::chrono::steady_clock;

    std::string targetSceneAsset;
    if (targetScene.empty()) {
        targetSceneAsset = Engine::ui->getScene().asset;
    }
    else {
        targetSceneAsset = targetScene;
    }

    Engine::ui->SwitchScene(context.loadScreenScene);

    const float totalWeight = std::ranges::fold_left(
        queue, 0.0f,
        [](const float sum, const LoadStage &process) { return sum + process.getWeight(); }
    );
    float doneWeight = 0.0f;

    ProcessWindow();

    clock::time_point lastFrame = clock::now();
    for (const LoadStage &process : queue) {
        UpdateStage(process.getLabel());
        for (const float localProgress : process.Run()) {
            if (
                const clock::time_point now = clock::now();
                now - lastFrame < context.frameInterval
            ) {
                continue;
            }

            const float progress = (doneWeight + process.getWeight() * localProgress) / totalWeight;
            UpdateProgress(progress);
            ProcessWindow();

            lastFrame = clock::now();
        }
        doneWeight += process.getWeight();
    }

    UpdateProgress(1.0f);
    ProcessWindow();

    Engine::ui->SwitchScene(targetSceneAsset);
}

void ee_engine::LoadScreen::ProcessWindow() const {
    Engine::ui->Update();

    Engine::renderer->ProcessQueue(context.renderCmdBuffer);
    Engine::renderer->Draw();

    glfwSwapBuffers(context.window);
    Engine::input->Poll();
    glfwPollEvents();
}

void ee_engine::LoadScreen::UpdateProgress(const float progress) const {
    loadingInfo->data.progress = progress * 100.0f;
    loadingInfo->model.DirtyVariable("progress");
}

void ee_engine::LoadScreen::UpdateStage(const std::string &stage) const {
    loadingInfo->data.stage = stage;
    loadingInfo->model.DirtyVariable("stage");
}
