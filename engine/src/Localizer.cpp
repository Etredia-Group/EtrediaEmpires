module Localizer;

Localizer::Localizer(const Locale &defaultLocale, ee_engine::AssetManager &assetManager) :
    currentLocale(defaultLocale),
    assetManager(assetManager) {
    assetManager.LoadDir<ee_assets::Localisation>("assets/localisation");
}

void Localizer::SetLocale(const Locale &locale) {
    currentLocale = locale;
}

const std::string & Localizer::Localize(const std::string &table, const std::uint32_t &key) {
    const ee_assets::Localisation *localisation = GetTable(table);

    const auto result = localisation->loc.find(key);
    if (result == localisation->loc.end()) {
        return table;
    }

    return result->second;
}

const std::string& Localizer::Localize(const std::string &table, const std::string &key) {
    const ee_assets::Localisation *localisation = GetTable(table);

    const auto result = localisation->loc.find(key);
    if (result == localisation->loc.end()) {
        return key;
    }

    return result->second;
}

const ee_assets::Localisation * Localizer::GetTable(const std::string &table) {
    const ee_assets::Localisation *localisation;

    if (const auto cache = tables.find(table); cache == tables.end()) {
        localisation = &assetManager.FindOnce<ee_assets::Localisation>(
            "assets/localisation/" + currentLocale.code + "/" + table + ".loc"
        );
        tables[table] = localisation;
    }
    else {
        localisation = cache->second;
    }

    return localisation;
}
