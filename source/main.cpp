#include <3ds.h>
#include <citro2d.h>
#include <cstdio>
#include <cstring>

namespace {
constexpr int TOP_W = 400;
constexpr int SCREEN_H = 240;
constexpr int BOTTOM_W = 320;

enum class Page {
    MainMenu,
    Difficulty,
    Info
};

struct MenuItem {
    const char* label;
};

const MenuItem kMainItems[] = {
    {"New Game"},
    {"Continue"},
    {"Achievements"},
    {"Community"},
    {"Changelog"},
    {"Character"},
    {"Options"},
    {"Log out"}
};

const MenuItem kDifficultyItems[] = {
    {"Novice"},
    {"Regular"},
    {"Veteran"},
    {"Back"}
};

C2D_SpriteSheet gSkySheet = nullptr;
C2D_SpriteSheet gCitySheet = nullptr;
C2D_SpriteSheet gLogoSheet = nullptr;
C2D_Image gSky = {};
C2D_Image gCity = {};
C2D_Image gLogo = {};
C2D_TextBuf gTextBuffer = nullptr;

Page gPage = Page::MainMenu;
int gSelection = 0;
const char* gInfoTitle = "Mini DAYZ";
const char* gInfoBody = "This screen is part of the native 3DS menu port.";

const MenuItem* currentItems(int& count)
{
    if (gPage == Page::Difficulty) {
        count = sizeof(kDifficultyItems) / sizeof(kDifficultyItems[0]);
        return kDifficultyItems;
    }

    count = sizeof(kMainItems) / sizeof(kMainItems[0]);
    return kMainItems;
}

void drawImage(C2D_Image image, float x, float y, float width, float height)
{
    if (!image.tex || !image.subtex || image.subtex->width == 0 || image.subtex->height == 0)
        return;

    const float sx = width / image.subtex->width;
    const float sy = height / image.subtex->height;
    C2D_DrawImageAt(image, x, y, 0.0f, nullptr, sx, sy);
}

void drawText(const char* value, float x, float y, float scale, u32 color)
{
    C2D_Text text;
    C2D_TextParse(&text, gTextBuffer, value);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, x, y, 0.0f, scale, scale, color);
}

void drawTopScreen(C3D_RenderTarget* top)
{
    C2D_TargetClear(top, C2D_Color32(10, 17, 20, 255));
    C2D_SceneBegin(top);

    // Original Mini DAYZ title-screen sky, city layer and logo.
    if (gSky.tex)
        drawImage(gSky, 0.0f, 0.0f, TOP_W, SCREEN_H);

    if (gCity.tex)
        drawImage(gCity, 0.0f, 92.0f, TOP_W, 148.0f);

    // Darken the lower edge slightly so the skyline sits naturally.
    C2D_DrawRectSolid(0.0f, 220.0f, 0.1f, TOP_W, 20.0f,
                      C2D_Color32(8, 12, 12, 90));

    if (gLogo.tex) {
        const float logoW = 292.0f;
        const float logoH = logoW * gLogo.subtex->height / gLogo.subtex->width;
        drawImage(gLogo, (TOP_W - logoW) * 0.5f, 12.0f, logoW, logoH);
    } else {
        drawText("MINI DAYZ", 104.0f, 24.0f, 0.9f,
                 C2D_Color32(245, 238, 218, 255));
    }

    C2D_Flush();
}

void drawButton(const char* label, int index, int selected,
                float x, float y, float width, float height)
{
    const bool active = index == selected;
    const u32 fill = active
        ? C2D_Color32(113, 91, 55, 245)
        : C2D_Color32(28, 34, 31, 235);
    const u32 edge = active
        ? C2D_Color32(230, 202, 139, 255)
        : C2D_Color32(104, 111, 99, 255);

    C2D_DrawRectSolid(x, y, 0.0f, width, height, fill);
    C2D_DrawRectSolid(x, y, 0.01f, width, 2.0f, edge);
    C2D_DrawRectSolid(x, y + height - 2.0f, 0.01f, width, 2.0f, edge);

    drawText(label, x + 14.0f, y + 3.5f, 0.39f,
             C2D_Color32(245, 239, 220, 255));
}

void drawBottomScreen(C3D_RenderTarget* bottom)
{
    C2D_TargetClear(bottom, C2D_Color32(14, 19, 18, 255));
    C2D_SceneBegin(bottom);

    C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, BOTTOM_W, SCREEN_H,
                      C2D_Color32(14, 19, 18, 255));
    C2D_DrawRectSolid(0.0f, 0.0f, 0.01f, BOTTOM_W, 3.0f,
                      C2D_Color32(151, 123, 75, 255));

    gTextBuffer && C2D_TextBufClear(gTextBuffer);

    if (gPage == Page::Info) {
        drawText(gInfoTitle, 18.0f, 24.0f, 0.55f,
                 C2D_Color32(245, 239, 220, 255));
        drawText(gInfoBody, 18.0f, 66.0f, 0.36f,
                 C2D_Color32(205, 207, 193, 255));
        drawText("B  BACK", 18.0f, 204.0f, 0.38f,
                 C2D_Color32(230, 202, 139, 255));
        C2D_Flush();
        return;
    }

    const char* heading = gPage == Page::Difficulty
        ? "SELECT DIFFICULTY" : "MAIN MENU";
    drawText(heading, 16.0f, 7.0f, 0.42f,
             C2D_Color32(230, 202, 139, 255));

    int count = 0;
    const MenuItem* items = currentItems(count);
    const float startY = gPage == Page::MainMenu ? 28.0f : 48.0f;
    const float rowH = gPage == Page::MainMenu ? 23.0f : 32.0f;
    const float gap = gPage == Page::MainMenu ? 3.0f : 8.0f;

    for (int i = 0; i < count; ++i) {
        drawButton(items[i].label, i, gSelection, 14.0f,
                   startY + i * (rowH + gap), 292.0f, rowH);
    }

    drawText("D-PAD / A  SELECT     B  BACK", 14.0f, 224.0f, 0.25f,
             C2D_Color32(168, 174, 161, 255));
    C2D_Flush();
}

void showInfo(const char* title, const char* body)
{
    gPage = Page::Info;
    gSelection = 0;
    gInfoTitle = title;
    gInfoBody = body;
}

void activateSelection()
{
    if (gPage == Page::Difficulty) {
        if (gSelection == 3) {
            gPage = Page::MainMenu;
            gSelection = 0;
            return;
        }

        static const char* difficultyNames[] = {"Novice", "Regular", "Veteran"};
        static char message[96];
        std::snprintf(message, sizeof(message),
                      "%s selected. The original world and survival systems are the next porting phase.",
                      difficultyNames[gSelection]);
        showInfo("NEW GAME", message);
        return;
    }

    switch (gSelection) {
        case 0:
            gPage = Page::Difficulty;
            gSelection = 0;
            break;
        case 1:
            showInfo("CONTINUE", "The native save-game loader has not been ported yet.");
            break;
        case 2:
            showInfo("ACHIEVEMENTS", "The original achievement screen is queued for the next UI pass.");
            break;
        case 3:
            showInfo("COMMUNITY", "Community links need an online-service implementation.");
            break;
        case 4:
            showInfo("CHANGELOG", "The original changelog screen has not been ported yet.");
            break;
        case 5:
            showInfo("CHARACTER", "The original survivor roster and unlock rules are queued for porting.");
            break;
        case 6:
            showInfo("OPTIONS", "Native 3DS options will be connected here.");
            break;
        case 7:
            showInfo("LOG OUT", "There is no account session in the offline 3DS port.");
            break;
    }
}

void moveSelection(int delta)
{
    int count = 0;
    currentItems(count);
    if (count <= 0)
        return;

    gSelection = (gSelection + delta + count) % count;
}
} // namespace

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    gfxInitDefault();
    romfsInit();

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    gTextBuffer = C2D_TextBufNew(2048);

    // These are the original title-screen assets copied into gfx/ and
    // converted to T3X by the existing Makefile graphics target.
    gSkySheet = C2D_SpriteSheetLoad("romfs:/gfx/tiledbgmenu_sky_sprite_sheet0.t3x");
    gCitySheet = C2D_SpriteSheetLoad("romfs:/gfx/tiledbgmenu_city_up.t3x");
    gLogoSheet = C2D_SpriteSheetLoad("romfs:/gfx/main_menu_logo_sheet0.t3x");

    if (gSkySheet && C2D_SpriteSheetCount(gSkySheet) > 0)
        gSky = C2D_SpriteSheetGetImage(gSkySheet, 0);
    if (gCitySheet && C2D_SpriteSheetCount(gCitySheet) > 0)
        gCity = C2D_SpriteSheetGetImage(gCitySheet, 0);
    if (gLogoSheet && C2D_SpriteSheetCount(gLogoSheet) > 0)
        gLogo = C2D_SpriteSheetGetImage(gLogoSheet, 0);

    int lastSelection = -1;
    Page lastPage = static_cast<Page>(-1);

    while (aptMainLoop()) {
        hidScanInput();
        const u32 down = hidKeysDown();

        if (down & KEY_START)
            break;

        if (gPage != Page::Info) {
            if (down & KEY_UP)
                moveSelection(-1);
            if (down & KEY_DOWN)
                moveSelection(1);
        }

        if (down & KEY_B) {
            if (gPage != Page::MainMenu) {
                gPage = Page::MainMenu;
                gSelection = 0;
            }
        }

        if (down & KEY_A) {
            if (gPage == Page::Info) {
                gPage = Page::MainMenu;
                gSelection = 0;
            } else {
                activateSelection();
            }
        }

        if (down & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);

            if (gPage == Page::Info) {
                if (touch.py > 180) {
                    gPage = Page::MainMenu;
                    gSelection = 0;
                }
            } else {
                int count = 0;
                currentItems(count);
                const float startY = gPage == Page::MainMenu ? 28.0f : 48.0f;
                const float rowH = gPage == Page::MainMenu ? 23.0f : 32.0f;
                const float gap = gPage == Page::MainMenu ? 3.0f : 8.0f;
                const float pitch = rowH + gap;

                if (touch.px >= 14 && touch.px <= 306 && touch.py >= startY) {
                    const int tapped = static_cast<int>((touch.py - startY) / pitch);
                    if (tapped >= 0 && tapped < count &&
                        (touch.py - startY) - tapped * pitch <= rowH) {
                        gSelection = tapped;
                        activateSelection();
                    }
                }
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawTopScreen(top);
        drawBottomScreen(bottom);
        C3D_FrameEnd(0);

        lastSelection = gSelection;
        lastPage = gPage;
        (void)lastSelection;
        (void)lastPage;
    }

    if (gTextBuffer)
        C2D_TextBufDelete(gTextBuffer);
    if (gSkySheet)
        C2D_SpriteSheetFree(gSkySheet);
    if (gCitySheet)
        C2D_SpriteSheetFree(gCitySheet);
    if (gLogoSheet)
        C2D_SpriteSheetFree(gLogoSheet);

    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
