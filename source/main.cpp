extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
}

// --- DATA STRUCTURES (Ported from mp_itemtable.js & mp_entities.js) ---
struct Item {
    int id;
    char name[32];
    int stackSize;
    int spriteID;
};

struct Entity {
    int id;
    float x;
    float y;
    float speed;
    int health;
    int maxHealth;
    int spriteID;
    bool isHostile;
};

// --- MENU OPTIONS ---
enum MenuOption {
    MENU_START = 0,
    MENU_CONTINUE,
    MENU_OPTIONS,
    MENU_COUNT
};

// --- GLOBAL TEXTURE STRUCTURES ---
static C3D_Tex g_menuTex;
static C2D_Image g_menuImg;
static Tex3DS_SubTexture g_subtex;

// Prepares sprite image pointer for Citro2D drawing
C2D_Image GetSpriteImage(u32 spriteID) {
    g_subtex.width = 256;
    g_subtex.height = 256;
    g_subtex.left = 0.0f;
    g_subtex.top = 0.0f;
    g_subtex.right = 1.0f;
    g_subtex.bottom = 1.0f;

    g_menuImg.tex = &g_menuTex;
    g_menuImg.subtex = &g_subtex;

    return g_menuImg;
}

// Simple XML Reader test function for files in /romfs
void ReadConfigFile(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (file) {
        char line[128];
        // Read up to first 5 lines for verification
        for (int i = 0; i < 5 && fgets(line, sizeof(line), file); i++) {
            // Processing lines from l_eng_ui.xml or spawn_npc_arrays.txt
        }
        fclose(file);
    }
}

int main(int argc, char* argv[]) {
    // 1. Initialize Hardware Services & RomFS
    gfxInitDefault();
    romfsInit();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        while (aptMainLoop()) { gspWaitForVBlank(); }
    }

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        while (aptMainLoop()) { gspWaitForVBlank(); }
    }

    C2D_Prepare();

    // Create Render Targets for both screens
    C3D_RenderTarget* topTarget    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Initialize VRAM Texture Buffer
    if (!C3D_TexInit(&g_menuTex, 256, 256, GPU_RGBA5551)) {
        // Texture Allocation Failure Fallback
    }
    C3D_TexSetFilter(&g_menuTex, GPU_NEAREST, GPU_NEAREST);

    // Load RomFS text file test
    ReadConfigFile("romfs:/l_eng_ui.xml");

    // Initialize Player Entity (Translated from mp_players.js)
    Entity player;
    player.id = 1;
    player.x = 200.0f;  // Center of Top Screen (400x240)
    player.y = 120.0f;
    player.speed = 2.0f;
    player.health = 100;
    player.maxHealth = 100;
    player.spriteID = 0;
    player.isHostile = false;

    // Asset IDs from asset_map.txt
    const u32 BG_SPRITE_ID    = 0;
    const u32 BTN_START_ID    = 1;
    const u32 BTN_CONTINUE_ID = 2;
    const u32 BTN_OPTIONS_ID  = 3;

    int selectedOption = MENU_START;

    // Main Game Loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        // --- PLAYER MOVEMENT LOGIC (Circle Pad & D-Pad) ---
        circlePosition circle;
        hidCircleRead(&circle);

        if (circle.dx > 15 || (kHeld & KEY_DRIGHT)) player.x += player.speed;
        if (circle.dx < -15 || (kHeld & KEY_DLEFT)) player.x -= player.speed;
        if (circle.dy > 15 || (kHeld & KEY_DUP))    player.y -= player.speed;
        if (circle.dy < -15 || (kHeld & KEY_DDOWN))  player.y += player.speed;

        // Screen Clamp for Player (Top Screen Bounds: 400x240)
        if (player.x < 0) player.x = 0;
        if (player.x > 384) player.x = 384;
        if (player.y < 0) player.y = 0;
        if (player.y > 224) player.y = 224;

        // --- BOTTOM SCREEN MENU NAVIGATION ---
        if (kDown & KEY_CSTICK_DOWN) {
            selectedOption = (selectedOption + 1) % MENU_COUNT;
        } 
        else if (kDown & KEY_CSTICK_UP) {
            selectedOption = (selectedOption == 0) ? (MENU_COUNT - 1) : (selectedOption - 1);
        }

        // --- RENDER FRAME ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // ==========================================
        // 1. TOP SCREEN (Gameplay World)
        // ==========================================
        C2D_TargetClear(topTarget, C2D_Color32(0x20, 0x50, 0x20, 0xFF)); // Green Grass
        C2D_SceneBegin(topTarget);

        // Draw Player Sprite (16x16 red square placeholder/sprite)
        C2D_DrawRectSolid(player.x, player.y, 0.5f, 16.0f, 16.0f, C2D_Color32(255, 0, 0, 255));

        // ==========================================
        // 2. BOTTOM SCREEN (Interactive UI Menu)
        // ==========================================
        C2D_TargetClear(bottomTarget, C2D_Color32(0x10, 0x10, 0x10, 0xFF));
        C2D_SceneBegin(bottomTarget);

        // Draw Menu Background
        C2D_Image bgImg = GetSpriteImage(BG_SPRITE_ID);
        if (bgImg.tex != NULL) {
            C2D_DrawImageAt(bgImg, 0.0f, 0.0f, 0.1f, NULL, 1.25f, 0.93f);
        }

        // Selection Highlight Tint
        C2D_ImageTint highlightTint;
        C2D_PlainImageTint(&highlightTint, C2D_Color32(255, 255, 100, 255), 0.8f);

        // Option 1: Start Game
        C2D_Image btnStart = GetSpriteImage(BTN_START_ID);
        if (btnStart.tex != NULL) {
            C2D_ImageTint* tint = (selectedOption == MENU_START) ? &highlightTint : NULL;
            C2D_DrawImageAt(btnStart, 90.0f, 40.0f, 0.5f, tint, 1.0f, 1.0f);
        }

        // Option 2: Continue
        C2D_Image btnContinue = GetSpriteImage(BTN_CONTINUE_ID);
        if (btnContinue.tex != NULL) {
            C2D_ImageTint* tint = (selectedOption == MENU_CONTINUE) ? &highlightTint : NULL;
            C2D_DrawImageAt(btnContinue, 90.0f, 95.0f, 0.5f, tint, 1.0f, 1.0f);
        }

        // Option 3: Options
        C2D_Image btnOptions = GetSpriteImage(BTN_OPTIONS_ID);
        if (btnOptions.tex != NULL) {
            C2D_ImageTint* tint = (selectedOption == MENU_OPTIONS) ? &highlightTint : NULL;
            C2D_DrawImageAt(btnOptions, 90.0f, 150.0f, 0.5f, tint, 1.0f, 1.0f);
        }

        C3D_FrameEnd(0);
    }

    // Clean up hardware resources
    C3D_TexDelete(&g_menuTex);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
