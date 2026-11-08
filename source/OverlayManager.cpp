#include <3ds.h>
#include <nlohmann/json.hpp>
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

struct StepItem {
    std::string text;
    std::string state; // "todo", "wait", "ok", "warn", "fail"
};

struct DialogConfig {
    std::string titleKey;
    std::string textKey;
    std::string code;
    std::function<void()> onOk;
};

class OverlayManager {
public:
    OverlayManager();
    ~OverlayManager();

    // Loading Overlay Management
    void showLoading(const std::string& title);
    void hideLoading();
    bool isLoadingVisible() const;
    void setStatus(const std::string& status);
    void setProgress(float progress); // 0.0 to 1.0, < 0 for pulse/indeterminate
    void setSteps(const std::vector<StepItem>& steps);
    void startCountdown(int seconds, std::function<void()> onComplete);
    void cancelLoading();

    // Dialog Modal Management
    void showDialog(const std::string& reasonKey, const std::string& extraText = "", std::function<void()> onOk = nullptr);
    void hideDialog();
    bool isDialogVisible() const;

    // Callbacks & Interactivity
    void setCancelCallback(std::function<void()> cb);

    // Frame Update (Handles countdowns and animations)
    void update(float deltaTime);

    // Rendering (Called in the 3DS frame render loop)
    void render();

private:
    bool loadingVisible;
    bool dialogVisible;
    
    // Loading State
    std::string loadingTitle;
    std::string statusText;
    float progressValue; // -1.0 for pulse
    std::vector<StepItem> stepsList;

    // Countdown State
    int countdownRemaining;
    float countdownTimer;
    std::function<void()> countdownCallback;

    // Dialog State
    DialogConfig currentDialog;

    // Callbacks
    std::function<void()> onCancelCallback;

    // Error reasons lookup dictionary (equivalent to _x8a04e in JS)
    const std::unordered_map<std::string, DialogConfig> errorReasons = {
        {"host_left", {"dlg.serverClosed", "dlg.hostLeft", "MP-100 HOST_CLOSED", nullptr}},
        {"connection_lost", {"dlg.connLost", "dlg.linkDropped", "MP-200 LINK_DROPPED", nullptr}},
        {"kicked", {"dlg.kickedTitle", "dlg.kickedText", "MP-402 KICKED", nullptr}},
        {"banned", {"dlg.bannedTitle", "dlg.bannedText", "MP-403 BANNED", nullptr}}
    };
};
