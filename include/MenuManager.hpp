#pragma once
#include <3ds.h>
#include <vector>
#include <string>
#include <functional>

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

struct MenuButton {
    std::string id;
    std::string label;
    float x, y, width, height;
    bool enabled;
    bool selected;
    std::function<void()> onClick;
};

struct HostSetup {
    int accessType = 0; // 0: LAN, 1: Public
    int maxPlayers = 4;
    bool pvp = true;
    std::string serverName = "3DS Server";
    bool useCharacterGear = false;
};

struct WorldConfig {
    int difficulty = 1; // 0: Novice, 1: Regular, etc.
    int sizeX = 16;
    int sizeY = 16;
};

enum class MenuTab {
    Main,
    Host,
    Join,
    World,
    Gameplay,
    Advanced
};

class MenuManager {
public:
    MenuManager();
    ~MenuManager();

    void initialize();
    void update(float deltaTime, u32 keysDown, touchPosition touch);
    
    void renderTopScreen();
    void renderBottomScreen();

private:
    void setTab(MenuTab tab);
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

    MenuTab currentTab;
    int selectedButtonIndex;
    std::string statusMessage;
    
    std::vector<MenuButton> currentButtons;
    std::vector<ServerEntry> recentServers;
    HostSetup hostSetup;
    WorldConfig worldConfig;
};
