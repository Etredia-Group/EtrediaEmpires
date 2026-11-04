module Localizer;
import Engine;

ee_engine::Localizer::Localizer(const Locale &defaultLocale) :
    currentLocale(defaultLocale) {
    Engine::assetManager->LoadDir<ee_assets::Localisation>("assets/localisation");
}

std::uint8_t ee_engine::Localizer::Subscribe(LocaleListener listener) {
    const std::uint8_t id = nextListenerId++;
    listeners.emplace(id, std::move(listener));
    return id;
}

void ee_engine::Localizer::Unsubscribe(const std::uint8_t id) {
    listeners.erase(id);
}

void ee_engine::Localizer::SetLocale(const Locale &locale) {
    if (locale.code == currentLocale.code) return;

    currentLocale = locale;
    tables.clear();

    for (LocaleListener &listener : listeners | std::views::values) {
        listener(currentLocale);
    }
}

std::string_view ee_engine::Localizer::Localize(const std::string_view table, const std::uint32_t key) {
    const ee_assets::Localisation *localisation = GetTable(table);

    const auto result = localisation->loc.find(key);
    if (result == localisation->loc.end()) {
        return {};
    }

    return result->second;
}

std::string_view ee_engine::Localizer::Localize(const std::string_view table, const std::string_view key) {
    const ee_assets::Localisation *localisation = GetTable(table);

    const auto result = localisation->loc.find(key);
    if (result == localisation->loc.end()) {
        return {};
    }

    return result->second;
}

const ee_assets::Localisation * ee_engine::Localizer::GetTable(const std::string_view table) {
    const ee_assets::Localisation *localisation;

    if (const auto cache = tables.find(table); cache == tables.end()) {
        const std::string &tableName = std::string(table);
        localisation = &Engine::assetManager->FindOnce<ee_assets::Localisation>(
            "assets/localisation/" + currentLocale.code + "/" + tableName + ".loc"
        );
        tables[tableName] = localisation;
    }
    else {
        localisation = cache->second;
    }

    return localisation;
}
