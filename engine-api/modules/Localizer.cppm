export module Localizer;
import AssetData;
import utils;
import std;

export namespace ee_engine {

    struct Locale {
        std::string code;

        // ReSharper disable CppNonExplicitConvertingConstructor
        Locale(const std::string &code) : code(code) {}
        Locale(const char *code) : code(code) {}
        // ReSharper restore CppNonExplicitConvertingConstructor
    };

    using LocaleListener = std::move_only_function<void(const Locale&)>;

    class Localizer {
    public:
        Localizer(const Locale &defaultLocale);
        std::uint8_t Subscribe(LocaleListener listener);
        void Unsubscribe(std::uint8_t id);

        void SetLocale(const Locale &locale);
        std::string_view Localize(std::string_view table, std::uint32_t key);
        std::string_view Localize(std::string_view table, std::string_view key);

        const Locale& getLocale() const noexcept { return currentLocale; }

    private:
        Locale currentLocale;

        std::unordered_map<std::uint8_t, LocaleListener> listeners;
        std::uint8_t nextListenerId = 0;

        using LocCache =
            std::unordered_map<std::string, const ee_assets::Localisation*, ee_utils::StringHash, ee_utils::StringEqual>;
        LocCache tables;

        const ee_assets::Localisation* GetTable(std::string_view table);
    };

}