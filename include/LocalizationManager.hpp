#pragma once
#include <string>
#include <unordered_map>
#include <vector>

enum class Language {
    EN = 0,
    RU,
    DE,
    ES,
    FR,
    IT,
    CZ,
    PT
};

class LocalizationManager {
public:
    static LocalizationManager& getInstance();

    void setLanguage(Language lang);
    Language getCurrentLanguage() const { return currentLang; }

    // Translates a key (e.g. "panel.host") into the active language string
    std::string getString(const std::string& key);

    // Translates and replaces dynamic placeholders like "{room}" or "{n}"
    std::string getFormattedString(const std::string& key, const std::unordered_map<std::string, std::string>& vars);

    // Auto-detect language from system settings or config
    void detectSystemLanguage();

private:
    LocalizationManager();
    ~LocalizationManager() = default;

    Language currentLang;
    std::unordered_map<std::string, std::vector<std::string>> translationTable;

    void initializeDictionary();
};
