export module Localizer;
import AssetManager;
import std;

export {

    struct Locale {
        std::string code;

        // ReSharper disable CppNonExplicitConvertingConstructor
        Locale(const std::string &code) : code(code) {}
        Locale(const char *code) : code(code) {}
        // ReSharper restore CppNonExplicitConvertingConstructor
    };

    class Localizer {
    public:
        Localizer(const Locale &defaultLocale, ee_engine::AssetManager &assetManager);

        void SetLocale(const Locale &locale);
        const std::string& Localize(const std::string &table, const std::uint32_t &key);
        const std::string& Localize(const std::string &table, const std::string &key);

    private:
        Locale currentLocale;
        ee_engine::AssetManager &assetManager;

        std::unordered_map<std::string, const ee_assets::Localisation*> tables;

        const ee_assets::Localisation* GetTable(const std::string &table);
    };

}