module;

#include <GLFW/glfw3.h>
#include <RmlUi/Core/Unit.h>

export module LoadScreen;
import Input;
import Render;
import Localizer;
import UI;
import std;

export {

    // ReSharper disable CppNonExplicitConvertingConstructor
    class LoadStage {
    public:
        template <std::invocable TProcess>
        LoadStage(TProcess process, const std::string &label, const float weight=1.0f) :
            label(label),
            weight(weight) {
            if constexpr (std::is_void_v<std::invoke_result_t<TProcess&>>) {
                this->process = [process = std::move(process)] { return RunSync(process); };
            }
            else {
                this->process = std::move(process);
            }
        }

        std::generator<float> Run() const { return process(); }
        constexpr float getWeight() const noexcept {return weight;}
        constexpr const std::string& getLabel() const noexcept {return label;}

    private:
        std::function<std::generator<float>()> process;
        const std::string label;
        const float weight;

        template <class TProcess>
        static std::generator<float> RunSync(TProcess process) {
            process();
            co_yield 1.0f;
        }
    };
    // ReSharper restore CppNonExplicitConvertingConstructor

    template <std::size_t size> using LoadStages = std::array<LoadStage, size>;

    class LoadScreen {
    public:
        struct Context {
            GLFWwindow *window;
            ee_engine::IInput &input;
            ee_engine::IRenderer &renderer;
            Localizer &localizer;
            UI &ui;

            ee_engine::RendererCommand &renderCmdBuffer;
            const std::string loadScreenScene;
            std::chrono::milliseconds frameInterval{16};
        };

        explicit LoadScreen(const Context &context) : context(context) {}

        void Run(const std::span<const LoadStage> queue, const std::string &targetScene="") const {
            using clock = std::chrono::steady_clock;

            std::string targetSceneAsset;
            if (targetScene.empty()) {
                targetSceneAsset = context.ui.getScene().asset;
            }
            else {
                targetSceneAsset = targetScene;
            }

            context.ui.SwitchScene(context.loadScreenScene);

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

            context.ui.SwitchScene(targetSceneAsset);
        }

    private:
        const Context context;

        void ProcessWindow() const {
            context.ui.Update();

            context.renderer.ProcessQueue(context.renderCmdBuffer);
            context.renderer.Draw();

            glfwSwapBuffers(context.window);
            context.input.Poll();
            glfwPollEvents();
        }

        void UpdateProgress(const float progress) const {
            Rml::Binded<ee_engine::LoadingInfo> &loadingInfo = context.ui.loadingInfo;
            loadingInfo.data.progress = progress * 100.0f;
            loadingInfo.model.DirtyVariable("progress");
        }

        void UpdateStage(const std::string &stage) const {
            Rml::Binded<ee_engine::LoadingInfo> &loadingInfo = context.ui.loadingInfo;
            //TODO: https://mikke89.github.io/RmlUiDoc/pages/localisation.html
            loadingInfo.data.stage = context.localizer.Localize("load-stages", stage);
            loadingInfo.model.DirtyVariable("stage");
        }
    };
}