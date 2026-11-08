#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <3ds.h>

enum class MenuTab {
    Main,
    Host,
    Join,
    World,
    Gameplay,
    Advanced
};

struct WorldConfig {
    int sizeX = 16;
    int sizeY = 16;
    int cities = 5;
    int villages = 7;
    int militaries = 3;
    int hospitals = 2;
    int firestations = 2;
    int gasStations = 1;
    int roadsideCafes = 1;
    std::string seed = "";
    int difficulty = 1; // 0: Novice, 1: Regular, 2: Veteran, 3: Legend
    bool isSavedWorld = false;
};

struct HostSetup {
    std::string serverName = "3DS Room";
    std::string password = "";
    int maxPlayers = 4;
    int accessType = 0; // 0: LAN, 1: Public
    bool pvp = true;
    bool showHostName = true;
    bool useCharacterGear = false;
};

struct ServerEntry {
    std::string roomName;
    std::string hostName;
    std::string ipOrUrl;
    int players;
    int maxPlayers;
    int pingMs;
};

struct MenuButton {
    std::string id;
    std::string label;
    float x, y, width, height;
    bool enabled;
    bool selected;
    std::function<void()> onClick;
};

class MenuManager {
public:
    MenuManager();
    ~MenuManager();

    void initialize();
    void update(float deltaTime, uint32_t keysDown, touchPosition touch);
    void renderBottomScreen(); // Renders everything exclusively on the 3DS bottom screen (320x240)

    void setTab(MenuTab tab);
    MenuTab getCurrentTab() const { return currentTab; }

    std::string getStatusMessage() const { return statusMessage; }

private:
    MenuTab currentTab;
    int selectedButtonIndex;
    std::vector<MenuButton> currentButtons;

    WorldConfig worldConfig;
    HostSetup hostSetup;
    std::vector<ServerEntry> recentServers;
    std::string statusMessage;

    // Tab Builders
    void buildMainMenu();
    void buildHostTab();
    void buildJoinTab();
    void buildWorldTab();
    void buildGameplayTab();
    void buildAdvancedTab();

    // Input Handlers
    void navigateSelection(int direction);
    void executeSelectedButton();
    void handleTouchInput(touchPosition touch);
    void addRecentServer(const ServerEntry& server);
};
