#include <nlohmann/json.hpp>
#include "MenuManager.hpp"
#include <citro2d.h>
#include <algorithm>
#include <iostream>

MenuManager::MenuManager() 
    : currentTab(MenuTab::Main),
      selectedButtonIndex(0),
      statusMessage("READY") {}

MenuManager::~MenuManager() {}

void MenuManager::initialize() {
    setTab(MenuTab::Main);
}

void MenuManager::setTab(MenuTab tab) {
    currentTab = tab;
    selectedButtonIndex = 0;
    currentButtons.clear();

    switch (currentTab) {
        case MenuTab::Main:     buildMainMenu(); break;
        case MenuTab::Host:     buildHostTab(); break;
        case MenuTab::Join:     buildJoinTab(); break;
        case MenuTab::World:    buildWorldTab(); break;
        case MenuTab::Gameplay: buildGameplayTab(); break;
        case MenuTab::Advanced: buildAdvancedTab(); break;
    }
}

// -------------------------------------------------------------
// MAIN MENU (Bottom Screen: 320x240)
// -------------------------------------------------------------
void MenuManager::buildMainMenu() {
    float startY = 25.0f;
    float btnW = 260.0f;
    float btnH = 30.0f;
    float posX = (320.0f - btnW) / 2.0f;

    currentButtons.push_back({"continue", "CONTINUE", posX, startY, btnW, btnH, true, true, [this]() {
        statusMessage = "Loading save slot...";
    }});

    currentButtons.push_back({"new_game", "NEW GAME", posX, startY + 36.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::World);
    }});

    currentButtons.push_back({"host_lobby", "HOST MULTIPLAYER", posX, startY + 72.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Host);
    }});

    currentButtons.push_back({"join_lobby", "JOIN MULTIPLAYER", posX, startY + 108.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Join);
    }});

    currentButtons.push_back({"gameplay", "GAMEPLAY OPTIONS", posX, startY + 144.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Gameplay);
    }});

    currentButtons.push_back({"advanced", "ADVANCED SETTINGS", posX, startY + 180.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Advanced);
    }});
}

// -------------------------------------------------------------
// HOST TAB
// -------------------------------------------------------------
void MenuManager::buildHostTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    currentButtons.push_back({"access", "ACCESS: " + std::string(hostSetup.accessType == 0 ? "LAN" : "PUBLIC"), posX, 20.0f, btnW, btnH, true, true, [this]() {
        hostSetup.accessType = (hostSetup.accessType + 1) % 2;
        setTab(MenuTab::Host);
    }});

    currentButtons.push_back({"max_players", "MAX PLAYERS: " + std::to_string(hostSetup.maxPlayers), posX, 52.0f, btnW, btnH, true, false, [this]() {
        hostSetup.maxPlayers = (hostSetup.maxPlayers % 4) + 1;
        setTab(MenuTab::Host);
    }});

    currentButtons.push_back({"pvp", "PVP: " + std::string(hostSetup.pvp ? "ENABLED" : "DISABLED"), posX, 84.0f, btnW, btnH, true, false, [this]() {
        hostSetup.pvp = !hostSetup.pvp;
        setTab(MenuTab::Host);
    }});

    currentButtons.push_back({"start_host", "CREATE WORLD & HOST", posX, 130.0f, btnW, btnH, true, false, [this]() {
        statusMessage = "Hosting server...";
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 175.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// JOIN TAB
// -------------------------------------------------------------
void MenuManager::buildJoinTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    currentButtons.push_back({"scan_lan", "SCAN LAN SERVERS", posX, 20.0f, btnW, btnH, true, true, [this]() {
        statusMessage = "Scanning local subnet...";
    }});

    currentButtons.push_back({"connect_ip", "DIRECT IP CONNECT", posX, 55.0f, btnW, btnH, true, false, [this]() {
        statusMessage = "Awaiting IP input...";
    }});

    currentButtons.push_back({"recent", "RECENT SERVERS (" + std::to_string(recentServers.size()) + ")", posX, 90.0f, btnW, btnH, !recentServers.empty(), false, [this]() {
        statusMessage = "Loading recent list...";
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 175.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// WORLD TAB
// -------------------------------------------------------------
void MenuManager::buildWorldTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    std::string diffStr = "REGULAR";
    if (worldConfig.difficulty == 0) diffStr = "NOVICE";
    else if (worldConfig.difficulty == 2) diffStr = "VETERAN";
    else if (worldConfig.difficulty == 3) diffStr = "LEGEND";

    currentButtons.push_back({"diff", "DIFFICULTY: " + diffStr, posX, 20.0f, btnW, btnH, true, true, [this]() {
        worldConfig.difficulty = (worldConfig.difficulty + 1) % 4;
        setTab(MenuTab::World);
    }});

    currentButtons.push_back({"size", "MAP SIZE: " + std::to_string(worldConfig.sizeX) + "x" + std::to_string(worldConfig.sizeY), posX, 55.0f, btnW, btnH, true, false, [this]() {
        worldConfig.sizeX = (worldConfig.sizeX == 16) ? 32 : 16;
        worldConfig.sizeY = worldConfig.sizeX;
        setTab(MenuTab::World);
    }});

    currentButtons.push_back({"start_solo", "START SINGLE PLAYER", posX, 120.0f, btnW, btnH, true, false, [this]() {
        statusMessage = "Generating world...";
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 175.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// GAMEPLAY TAB
// -------------------------------------------------------------
void MenuManager::buildGameplayTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    currentButtons.push_back({"gear", "START GEAR: " + std::string(hostSetup.useCharacterGear ? "CUSTOM" : "DEFAULT"), posX, 30.0f, btnW, btnH, true, true, [this]() {
        hostSetup.useCharacterGear = !hostSetup.useCharacterGear;
        setTab(MenuTab::Gameplay);
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 175.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// ADVANCED TAB
// -------------------------------------------------------------
void MenuManager::buildAdvancedTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    currentButtons.push_back({"name", "SHOW OWN NAME: YES", posX, 30.0f, btnW, btnH, true, true, [this]() {
        // Toggle name display
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 175.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// UPDATE & INPUT LOOP
// -------------------------------------------------------------
void MenuManager::update(float deltaTime, uint32_t keysDown, touchPosition touch) {
    if (keysDown & KEY_DUP) {
        navigateSelection(-1);
    } else if (keysDown & KEY_DDOWN) {
        navigateSelection(1);
    }

    if (keysDown & KEY_A) {
        executeSelectedButton();
    }

    if (keysDown & KEY_B) {
        if (currentTab != MenuTab::Main) {
            setTab(MenuTab::Main);
        }
    }

    if (keysDown & KEY_TOUCH) {
        handleTouchInput(touch);
    }
}

void MenuManager::navigateSelection(int direction) {
    if (currentButtons.empty()) return;
    currentButtons[selectedButtonIndex].selected = false;
    selectedButtonIndex += direction;

    if (selectedButtonIndex < 0) {
        selectedButtonIndex = static_cast<int>(currentButtons.size()) - 1;
    } else if (selectedButtonIndex >= static_cast<int>(currentButtons.size())) {
        selectedButtonIndex = 0;
    }
    currentButtons[selectedButtonIndex].selected = true;
}

void MenuManager::executeSelectedButton() {
    if (selectedButtonIndex >= 0 && selectedButtonIndex < static_cast<int>(currentButtons.size())) {
        if (currentButtons[selectedButtonIndex].enabled && currentButtons[selectedButtonIndex].onClick) {
            currentButtons[selectedButtonIndex].onClick();
        }
    }
}

void MenuManager::handleTouchInput(touchPosition touch) {
    for (size_t i = 0; i < currentButtons.size(); ++i) {
        auto& btn = currentButtons[i];
        if (btn.enabled &&
            touch.px >= btn.x && touch.px <= (btn.x + btn.width) &&
            touch.py >= btn.y && touch.py <= (btn.y + btn.height)) {
            
            currentButtons[selectedButtonIndex].selected = false;
            selectedButtonIndex = static_cast<int>(i);
            btn.selected = true;
            
            executeSelectedButton();
            break;
        }
    }
}

void MenuManager::addRecentServer(const ServerEntry& server) {
    recentServers.erase(std::remove_if(recentServers.begin(), recentServers.end(),
        [&server](const ServerEntry& s) { return s.roomName == server.roomName; }), recentServers.end());

    recentServers.insert(recentServers.begin(), server);
    if (recentServers.size() > 8) recentServers.pop_back();
}

// -------------------------------------------------------------
// CITRO2D BOTTOM SCREEN RENDERER (320x240)
// -------------------------------------------------------------
void MenuManager::renderBottomScreen() {
    // Render button plates and highlights
    for (const auto& btn : currentButtons) {
        u32 bgColor = btn.selected ? C2D_Color32(130, 45, 40, 255)   // Selected state (Reddish wood)
                                   : C2D_Color32(40, 36, 32, 255);  // Normal state (Dark wood)

        C2D_DrawRectSolid(btn.x, btn.y, 0.5f, btn.width, btn.height, bgColor);
        
        // Border outline
        C2D_DrawRectSolid(btn.x, btn.y, 0.5f, btn.width, 2.0f, C2D_Color32(90, 80, 70, 255));
        C2D_DrawRectSolid(btn.x, btn.y + btn.height - 2.0f, 0.5f, btn.width, 2.0f, C2D_Color32(20, 18, 16, 255));
    }
}
