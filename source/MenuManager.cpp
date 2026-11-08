#include "MenuManager.hpp"
#include <iostream>
#include <algorithm>

MenuManager::MenuManager() 
    : currentTab(MenuTab::Main),
      selectedButtonIndex(0),
      joinRoomNameInput(""),
      joinPasswordInput(""),
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
        default:                buildMainMenu(); break;
    }
}

// -------------------------------------------------------------
// 1. MAIN MENU BUILDER
// -------------------------------------------------------------
void MenuManager::buildMainMenu() {
    float startY = 30.0f;
    float btnW = 220.0f;
    float btnH = 32.0f;
    float posX = (320.0f - btnW) / 2.0f;

    currentButtons.push_back({"continue", "CONTINUE", posX, startY, btnW, btnH, true, true, [this]() {
        statusMessage = "Loading local save...";
    }});

    currentButtons.push_back({"new_game", "NEW GAME", posX, startY + 38.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::World);
    }});

    currentButtons.push_back({"host_lobby", "HOST MULTIPLAYER", posX, startY + 76.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Host);
    }});

    currentButtons.push_back({"join_lobby", "JOIN MULTIPLAYER", posX, startY + 114.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Join);
    }});

    currentButtons.push_back({"settings", "ADVANCED SETTINGS", posX, startY + 152.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Advanced);
    }});
}

// -------------------------------------------------------------
// 2. HOST TAB BUILDER (Server setup, PvP, Player caps)
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
        hostSetup.maxPlayers = (hostSetup.maxPlayers % 4) + 1; // 1 to 4 players on 3DS
        setTab(MenuTab::Host);
    }});

    currentButtons.push_back({"pvp", "PVP: " + std::string(hostSetup.pvp ? "ENABLED" : "DISABLED"), posX, 84.0f, btnW, btnH, true, false, [this]() {
        hostSetup.pvp = !hostSetup.pvp;
        setTab(MenuTab::Host);
    }});

    currentButtons.push_back({"start_host", "CREATE WORLD & HOST", posX, 130.0f, btnW, btnH, true, false, [this]() {
        statusMessage = "Starting server hosting...";
        ServerEntry localServer{hostSetup.serverName, "Host Player", "127.0.0.1", 1, hostSetup.maxPlayers, 0, false, 0};
        addRecentServer(localServer);
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 170.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// 3. JOIN TAB BUILDER (LAN scanning & recent servers)
// -------------------------------------------------------------
void MenuManager::buildJoinTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    currentButtons.push_back({"scan_lan", "SCAN LAN SERVERS", posX, 20.0f, btnW, btnH, true, true, [this]() {
        statusMessage = "Scanning local network...";
    }});

    currentButtons.push_back({"connect_direct", "DIRECT CONNECT TO IP", posX, 54.0f, btnW, btnH, true, false, [this]() {
        statusMessage = "Connecting to direct IP...";
    }});

    currentButtons.push_back({"recent_history", "RECENT SERVERS (" + std::to_string(recentServers.size()) + ")", posX, 88.0f, btnW, btnH, !recentServers.empty(), false, [this]() {
        statusMessage = "Loading recent history...";
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 150.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// 4. WORLD GENERATION TAB BUILDER (_xd configuration)
// -------------------------------------------------------------
void MenuManager::buildWorldTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    std::string diffStr = "REGULAR";
    if (worldConfig.difficulty == 0) diffStr = "NOVICE";
    else if (worldConfig.difficulty == 2) diffStr = "VETERAN";
    else if (worldConfig.difficulty == 3) diffStr = "LEGEND";

    currentButtons.push_back({"world_diff", "DIFFICULTY: " + diffStr, posX, 20.0f, btnW, btnH, true, true, [this]() {
        worldConfig.difficulty = (worldConfig.difficulty + 1) % 4;
        setTab(MenuTab::World);
    }});

    currentButtons.push_back({"world_size", "MAP SIZE: " + std::to_string(worldConfig.sizeX) + "x" + std::to_string(worldConfig.sizeY), posX, 54.0f, btnW, btnH, true, false, [this]() {
        worldConfig.sizeX = (worldConfig.sizeX == 16) ? 32 : 16;
        worldConfig.sizeY = worldConfig.sizeX;
        setTab(MenuTab::World);
    }});

    currentButtons.push_back({"start_solo", "START SINGLE PLAYER", posX, 110.0f, btnW, btnH, true, false, [this]() {
        statusMessage = "Generating map and starting game...";
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 160.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// 5. GAMEPLAY TAB BUILDER
// -------------------------------------------------------------
void MenuManager::buildGameplayTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    currentButtons.push_back({"char_gear", "START GEAR: " + std::string(hostSetup.useCharacterGear ? "CUSTOM" : "DEFAULT"), posX, 30.0f, btnW, btnH, true, true, [this]() {
        hostSetup.useCharacterGear = !hostSetup.useCharacterGear;
        setTab(MenuTab::Gameplay);
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 150.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// 6. ADVANCED SETTINGS TAB BUILDER
// -------------------------------------------------------------
void MenuManager::buildAdvancedTab() {
    float posX = 20.0f;
    float btnW = 280.0f;
    float btnH = 28.0f;

    currentButtons.push_back({"show_name", "SHOW OWN NAME: YES", posX, 30.0f, btnW, btnH, true, true, [this]() {
        // Toggle own name visibility overlay
    }});

    currentButtons.push_back({"back", "< BACK TO MAIN", posX, 150.0f, btnW, btnH, true, false, [this]() {
        setTab(MenuTab::Main);
    }});
}

// -------------------------------------------------------------
// INPUT & UPDATE ENGINE
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
    if (recentServers.size() > 8) {
        recentServers.pop_back(); // Cap history to 8 items
    }
}

void MenuManager::renderTopScreen() {
    // Top Screen (400x240): Visual banner, room info, status logs
}

void MenuManager::renderBottomScreen() {
    // Bottom Screen (320x240): Render layout buttons & selection states
}
