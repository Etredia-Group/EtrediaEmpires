export module controllers:UIController;
import std;
import rml;
import views;
import utils;

export namespace ee_engine {

    class UIEventListener final : public Rml::EventListener {
    public:
        explicit UIEventListener(std::function<void(Rml::Event&)> action) : action(std::move(action)) {}
        void ProcessEvent(Rml::Event &event) override { action(event); }
        void OnDetach(Rml::Element*) override { Deallocate(this); }
    private:
        std::function<void(Rml::Event&)> action;
    };

    class UIBaseController {
    public:
        virtual ~UIBaseController() = default;

        void Load(Rml::ElementDocument *document) {
            target = document;
            OnLoad();
        }

    protected:
        Rml::ElementDocument *target = nullptr;

        virtual void OnLoad() {}
    };

    template <IView TView>
    class UIController : public UIBaseController {
    public:
        UIController(
            Rml::Context &rmlContext,
            Rml::Binded<view::GlobalInfo> &globalInfo
        ) :
            data(rmlContext, &globalInfo) {
        }

        Rml::Binded<TView, Rml::Binded<view::GlobalInfo>>& getData() noexcept { return data; }

    protected:
        Rml::Binded<TView, Rml::Binded<view::GlobalInfo>> data;

        void BindEvent(
            const std::string &elementId,
            const std::string &event,
            std::function<void(Rml::Event&)> action
        ) const {
            if (target == nullptr) return;

            if (Rml::Element *element = target->GetElementById(elementId)) {
                element->AddEventListener(event, Allocate<UIEventListener>(std::move(action)));
            } else {
                LOG(ERROR, "Failed to bind event: element [%hs] not found", std::string(elementId).c_str());
            }
        }
    };

    template <typename T> concept IUIController = std::is_base_of_v<UIBaseController, T>;
}
