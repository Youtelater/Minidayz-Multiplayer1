#pragma once
#include <3ds.h>
#include <vector>
#include <string>
#include <functional>

// Make sure ServerEntry is defined somewhere accessible
struct ServerEntry {
    std::string roomName;
    std::string hostName;
    std::string ipAddress;
    int currentPlayers;
    int maxPlayers;
    int ping;
    bool pvp;
    int accessType;
};

// ... other structs like MenuButton, HostSetup, WorldConfig ...

class MenuManager {
public:
    MenuManager();
    ~MenuManager();

    void initialize();
    void update(float deltaTime, uint32_t keysDown, touchPosition touch);
    
    // REQUIRED: Declared so MenuManager.cpp can implement it!
    void renderTopScreen();
    void renderBottomScreen();

private:
    // Tab builders & helpers
    void setTab(int tab); // or enum type
    void buildMainMenu();
    void buildHostTab();
    void buildJoinTab();
    void buildWorldTab();
    void buildGameplayTab();
    void buildAdvancedTab();
    
    void navigateSelection(int direction);
    void executeSelectedButton();
    void handleTouchInput(touchPosition touch);
    void addRecentServer(const ServerEntry& server);

    int currentTab;
    int selectedButtonIndex;
    std::string statusMessage;
    // ... vector of buttons, hostSetup, worldConfig, recentServers, etc.
};
