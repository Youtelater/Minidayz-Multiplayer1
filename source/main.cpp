#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <tex3ds.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum GameState {
    STATE_LOADING,
    STATE_MENU,
    STATE_GAMEPLAY
};

struct ButtonBounds {
    float x, y, w, h;
};

// Helper structure for loading and playing 16-bit PCM WAV audio
typedef struct {
    ndspWaveBuf waveBuf;
    u32* audioData;
    u32 dataSize;
    int sampleRate;
    int channels;
    int bitsPerSample;
} SoundEffect;

bool loadWavFile(const char* path, SoundEffect* sound) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    char header[44];
    fread(header, 1, 44, f);

    sound->channels = header[22] | (header[23] << 8);
    sound->sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    sound->bitsPerSample = header[34] | (header[35] << 8);

    u32 dataSize = 0;
    fseek(f, 40, SEEK_SET);
    fread(&dataSize, 1, 4, f);

    sound->dataSize = dataSize;
    sound->audioData = (u32*)linearAlloc(dataSize);
    
    if (!sound->audioData) {
        fclose(f);
        return false;
    }

    fread(sound->audioData, 1, dataSize, f);
    fclose(f);

    GSP_FlushDataCache(sound->audioData, dataSize);

    memset(&sound->waveBuf, 0, sizeof(ndspWaveBuf));
    sound->waveBuf.data_vaddr = sound->audioData;
    sound->waveBuf.nsamples   = dataSize / (sound->channels * (sound->bitsPerSample / 8));
    sound->waveBuf.looping    = true; // Loop background music
    sound->waveBuf.status     = NDSP_WBUF_FREE;

    return true;
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    romfsInit();
    ndspInit();

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Load textures from RomFS
    C2D_SpriteSheet loadingSheet = C2D_SpriteSheetLoad("romfs:/loading_logo.t3x");
    C2D_SpriteSheet menuSheet    = C2D_SpriteSheetLoad("romfs:/main_menu_logo_sheet0.t3x");
    C2D_SpriteSheet buttonSheet  = C2D_SpriteSheetLoad("romfs:/buttons_sheet0.t3x");

    // Load background music from romfs/wav/ambient-C1EE9F53.wav[span_1](start_span)[span_1](end_span)
    SoundEffect bgm;
    bool audioLoaded = loadWavFile("romfs:/wav/ambient-C1EE9F53.wav", &bgm);
    if (audioLoaded) {
        ndspChnReset(0);
        ndspChnSetFormat(0, (bgm.channels == 2) ? NDSP_CHANNELS_STEREO : NDSP_CHANNELS_MONO);
        ndspChnSetRate(0, bgm.sampleRate);
        ndspChnSetVol(0, 1.0f);
        ndspChnWaveBufAdd(0, &bgm.waveBuf);
    }

    GameState currentState = STATE_LOADING;
    int loadingTimer = 0;
    int selectedButton = 0;
    int totalButtons = 4;

    // Bounding boxes matching where buttons are drawn on the 320x240 bottom touch screen
    ButtonBounds buttonBoxes[4] = {
        { 32.0f, 30.0f,  256.0f, 35.0f }, // Continue
        { 32.0f, 75.0f,  256.0f, 35.0f }, // New Game
        { 32.0f, 120.0f, 256.0f, 35.0f }, // Multiplayer
        { 32.0f, 165.0f, 256.0f, 35.0f }  // Options
    };

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        if (kDown & KEY_START) break;

        if (currentState == STATE_LOADING) {
            loadingTimer++;
            if (loadingTimer > 120 || (kDown & KEY_A)) {
                currentState = STATE_MENU;
            }
        } 
        else if (currentState == STATE_MENU) {
            // D-Pad Navigation
            if (kDown & KEY_UP) {
                selectedButton = (selectedButton - 1 + totalButtons) % totalButtons;
            }
            if (kDown & KEY_DOWN) {
                selectedButton = (selectedButton + 1) % totalButtons;
            }
            
            // Confirm with A button
            if (kDown & KEY_A) {
                if (selectedButton == 0 || selectedButton == 1) {
                    currentState = STATE_GAMEPLAY;
                }
            }

            // Touch Screen Interaction
            if (kDown & KEY_TOUCH) {
                for (int i = 0; i < totalButtons; i++) {
                    if (touch.px >= buttonBoxes[i].x && touch.px <= (buttonBoxes[i].x + buttonBoxes[i].w) &&
                        touch.py >= buttonBoxes[i].y && touch.py <= (buttonBoxes[i].y + buttonBoxes[i].h)) {
                        selectedButton = i;
                        if (i == 0 || i == 1) {
                            currentState = STATE_GAMEPLAY;
                        }
                    }
                }
            }
        }
        else if (currentState == STATE_GAMEPLAY) {
            // Return to menu with SELECT
            if (kDown & KEY_SELECT) {
                currentState = STATE_MENU;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // ==========================================
        // 1. TOP SCREEN (Visuals & Gameplay View)
        // ==========================================
        C2D_SceneBegin(topScreen);

        if (currentState == STATE_LOADING) {
            C2D_TargetClear(topScreen, C2D_Color32(0, 0, 0, 255));
            if (loadingSheet) {
                C2D_Image img = C2D_SpriteSheetGetImage(loadingSheet, 0);
                C2D_DrawImageAt(img, 75.0f, 40.0f, 0.5f, NULL, 0.65f, 0.65f);
            }
        } 
        else if (currentState == STATE_MENU) {
            // Red background and centered logo
            C2D_TargetClear(topScreen, C2D_Color32(180, 25, 25, 255));
            if (menuSheet) {
                C2D_Image logoImg = C2D_SpriteSheetGetImage(menuSheet, 0);
                C2D_DrawImageAt(logoImg, 80.0f, 60.0f, 0.5f, NULL, 0.5f, 0.5f);
            }
        }
        else if (currentState == STATE_GAMEPLAY) {
            C2D_TargetClear(topScreen, C2D_Color32(40, 40, 40, 255));
        }

        // ==========================================
        // 2. BOTTOM SCREEN (Interactive Buttons)
        // ==========================================
        C2D_TargetClear(bottomScreen, C2D_Color32(20, 20, 20, 255));
        C2D_SceneBegin(bottomScreen);

        if (currentState == STATE_MENU && buttonSheet) {
            for (int i = 0; i < totalButtons; i++) {
                C2D_Image btnImg = C2D_SpriteSheetGetImage(buttonSheet, i);
                C2D_DrawImageAt(btnImg, buttonBoxes[i].x, buttonBoxes[i].y, 0.5f, NULL, 0.75f, 0.6f);
            }
        }
        else if (currentState == STATE_GAMEPLAY) {
            // Reserved for future inventory/touch layout
        }

        C3D_FrameEnd(0);
    }

    // Cleanup audio
    if (audioLoaded) {
        ndspChnReset(0);
        linearFree(bgm.audioData);
    }
    ndspExit();

    // Cleanup graphics sheets
    if (loadingSheet) C2D_SpriteSheetFree(loadingSheet);
    if (menuSheet)   C2D_SpriteSheetFree(menuSheet);
    if (buttonSheet) C2D_SpriteSheetFree(buttonSheet);
    
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
