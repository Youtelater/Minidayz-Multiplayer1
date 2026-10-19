#include "ShaderMenu.h"

ShaderMenu::ShaderMenu() {
    cursorIndex = 0;
    config.shaderQuality = 1; // Default: Medium
    config.shadowsEnabled = true;
    config.currentLang = LANG_EN;
}

void ShaderMenu::handleInput(u32 kDown) {
    // Navigate options up and down using the 3DS D-Pad or Circle Pad
    if (kDown & KEY_DOWN) {
        cursorIndex = (cursorIndex + 1) % 3;
    }
    if (kDown & KEY_UP) {
        cursorIndex = (cursorIndex - 1 + 3) % 3;
    }

    // Toggle or cycle options when pressing the A button
    if (kDown & KEY_A) {
        if (cursorIndex == 0) {
            // Cycle shader quality: Off (0) -> Low (1) -> Medium (2) -> High (3)
            config.shaderQuality = (config.shaderQuality + 1) % 4;
        } 
        else if (cursorIndex == 1) {
            // Toggle shadows on/off
            config.shadowsEnabled = !config.shadowsEnabled;
        } 
        else if (cursorIndex == 2) {
            // Cycle through available languages (0 to 7)
            config.currentLang = (Language)((config.currentLang + 1) % 8);
        }
    }
}

void ShaderMenu::update() {
    // Put any continuous logic or animations for the menu here if needed
}

void ShaderMenu::render() {
    // Begin drawing to the 3DS bottom screen (or top screen depending on your setup)
    // C2D_SceneBegin(bottomScreenTarget);

    // TODO: Render your background graphics or menu box here using citro2d
    
    // Example of drawing options text:
    // - Highlight the row matching `cursorIndex`
    // - Display current values based on `config.shaderQuality`, `config.shadowsEnabled`, and `config.currentLang`
}

const ShaderMenuConfig& ShaderMenu::getConfig() const {
    return config;
}
