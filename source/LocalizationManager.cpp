#include <3ds.h>
#include <nlohmann/json.hpp>
#include "LocalizationManager.hpp"
#include <algorithm>
#include <3ds.h>

LocalizationManager& LocalizationManager::getInstance() {
    static LocalizationManager instance;
    return instance;
}

LocalizationManager::LocalizationManager() : currentLang(Language::EN) {
    initializeDictionary();
    detectSystemLanguage();
}

void LocalizationManager::setLanguage(Language lang) {
    currentLang = lang;
}

void LocalizationManager::detectSystemLanguage() {
    u8 sysLanguage = 0;
    if (R_SUCCEEDED(cfguInit())) {
        CFGU_GetSystemLanguage(&sysLanguage);
        cfguExit();
    }

    // Map 3DS system languages to our supported enum
    // 0: Japanese, 1: English, 2: French, 3: German, 4: Italian, 5: Spanish, 6: Simplified Chinese, 7: Korean, 8: Dutch, 9: Portuguese, 10: Russian, 11: Traditional Chinese
    switch (sysLanguage) {
        case 2: currentLang = Language::FR; break;
        case 3: currentLang = Language::DE; break;
        case 4: currentLang = Language::IT; break;
        case 5: currentLang = Language::ES; break;
        case 9: currentLang = Language::PT; break;
        case 10: currentLang = Language::RU; break;
        case 1: default: currentLang = Language::EN; break;
    }
}

std::string LocalizationManager::getString(const std::string& key) {
    auto it = translationTable.find(key);
    if (it == translationTable.end()) {
        return key; // Fallback to key if missing
    }

    int langIdx = static_cast<int>(currentLang);
    const auto& translations = it->second;

    if (langIdx >= 0 && langIdx < static_cast<int>(translations.size()) && !translations[langIdx].empty()) {
        return translations[langIdx];
    }

    return translations[0]; // Fallback to English (Index 0)
}

std::string LocalizationManager::getFormattedString(const std::string& key, const std::unordered_map<std::string, std::string>& vars) {
    std::string text = getString(key);

    for (const auto& pair : vars) {
        std::string placeholder = "{" + pair.first + "}";
        size_t pos = 0;
        while ((pos = text.find(placeholder, pos)) != std::string::npos) {
            text.replace(pos, placeholder.length(), pair.second);
            pos += pair.second.length();
        }
    }

    return text;
}

void LocalizationManager::initializeDictionary() {
    // Populate dictionary entries corresponding to the JS translation map
    translationTable["panel.host"] = {"HOST", "ХОСТ", "HOSTEN", "CREAR", "HÉBERGER", "OSPITA", "HOSTOVAT", "CRIAR"};
    translationTable["panel.join"] = {"JOIN", "ВОЙТИ", "BEITRETEN", "UNIRSE", "REJOINDRE", "ENTRA", "PŘIPOJIT", "ENTRAR"};
    translationTable["panel.disconnect"] = {"DISCONNECT", "ОТКЛЮЧИТЬСЯ", "TRENNEN", "DESCONECTAR", "QUITTER", "DISCONNETTI", "ODPOJIT", "DESLIGAR"};
    translationTable["menu.title"] = {"MULTIPLAYER", "МУЛЬТИПЛЕЕР", "MEHRSPIELER", "MULTIJUGADOR", "MULTIJOUEUR", "MULTIGIOCATORE", "MULTIPLAYER", "MULTIJOGADOR"};
    translationTable["death.title"] = {"You are dead", "Вы мертвы", "Du bist tot", "Estás muerto", "Vous êtes mort", "Sei morto", "Jsi mrtvý", "Você está morto"};
    translationTable["death.respawn"] = {"Respawn", "Возродиться", "Wiederbeleben", "Reaparecer", "Réapparaître", "Rinasci", "Oživit", "Renascer"};
}
