module;

#include <GLFW/glfw3.h>

export module LoadScreen;
import Render;
import UI;
import std;

export namespace ee_engine {

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
        const float& getWeight() const noexcept {return weight;}
        const std::string& getLabel() const noexcept {return label;}

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
            RendererCommand &renderCmdBuffer;
            const std::string loadScreenScene;
            std::chrono::milliseconds frameInterval{16};
        };

        explicit LoadScreen(const Context &context);
        void Run(std::span<const LoadStage> queue, const std::string &targetScene="") const;

    private:
        const Context context;
        UI::Binded<view::LoadingInfo> *loadingInfo;

        void ProcessWindow() const;
        void UpdateProgress(float progress) const;
        void UpdateStage(const std::string &stage) const;
    };
}