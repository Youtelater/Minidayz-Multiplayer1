#pragma once
#include <3ds.h>
#include <citro2d.h>
#include <string>

enum Language {
    LANG_EN = 0,
    LANG_RU,
    LANG_DE,
    LANG_ES,
    LANG_FR,
    LANG_IT,
    LANG_CS,
    LANG_PT
};

struct ShaderMenuConfig {
    int shaderQuality; // 0: Off, 1: Low, 2: Medium, 3: High
    bool shadowsEnabled;
    Language currentLang;
};

class ShaderMenu {
private:
    int cursorIndex;
    ShaderMenuConfig config;

public:
    ShaderMenu();
    void handleInput(u32 kDown);
    void update();
    void render();
    const ShaderMenuConfig& getConfig() const;
};
