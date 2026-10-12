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

    // Fixed: Use DSP_FlushDataCache instead of GSP_FlushDataCache
    DSP_FlushDataCache(sound->audioData, dataSize);

    memset(&sound->waveBuf, 0, sizeof(ndspWaveBuf));
    sound->waveBuf.data_vaddr = sound->audioData;
    sound->waveBuf.nsamples   = dataSize / (sound->channels * (sound->bitsPerSample / 8));
    sound->waveBuf.looping    = true;
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

    C2D_SpriteSheet loadingSheet = C2D_SpriteSheetLoad("romfs:/loading_logo.t3x");
    C2D_SpriteSheet menuSheet    = C2D_SpriteSheetLoad("romfs:/main_menu_logo_sheet0.t3x");
    C2D_SpriteSheet buttonSheet  = C2D_SpriteSheetLoad("romfs:/buttons_sheet0.t3x");

    SoundEffect bgm;
    bool audioLoaded = loadWavFile("romfs:/wav/ambient-C1EE9F53.wav", &bgm);
    if (audioLoaded) {
        ndspChnReset(0);
        // Fixed: Use standard libctru channel format constants (1 = Mono, 2 = Stereo)
        ndspChnSetFormat(0, (bgm.channels == 2) ? NDSP_FORMAT_STEREO_16BIT : NDSP_FORMAT_MONO_16BIT);
        ndspChnSetRate(0, bgm.sampleRate);
        
        // Fixed: Use ndspChnSetMix to set volume levels for stereo channels
        float mix[12] = {0};
        mix[0] = 1.0f; // Left volume
        mix[1] = 1.0f; // Right volume
        ndspChnSetMix(0, mix);
        
        ndspChnWaveBufAdd(0, &bgm.waveBuf);
    }

    GameState currentState = STATE_LOADING;
    int loadingTimer = 0;
    int selectedButton = 0;
    int totalButtons = 4;

    ButtonBounds buttonBoxes[4] = {
        { 32.0f, 30.0f,  256.0f, 35.0f },
        { 32.0f, 75.0f,  256.0f, 35.0f },
        { 32.0f, 120.0f, 256.0f, 35.0f },
        { 32.0f, 165.0f, 256.0f, 35.0f }
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
            if (kDown & KEY_UP) {
                selectedButton = (selectedButton - 1 + totalButtons) % totalButtons;
            }
            if (kDown & KEY_DOWN) {
                selectedButton = (selectedButton + 1) % totalButtons;
            }
            
            if (kDown & KEY_A) {
                if (selectedButton == 0 || selectedButton == 1) {
                    currentState = STATE_GAMEPLAY;
                }
            }

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
            if (kDown & KEY_SELECT) {
                currentState = STATE_MENU;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_SceneBegin(topScreen);
        if (currentState == STATE_LOADING) {
            C2D_TargetClear(topScreen, C2D_Color32(0, 0, 0, 255));
            if (loadingSheet) {
                C2D_Image img = C2D_SpriteSheetGetImage(loadingSheet, 0);
                C2D_DrawImageAt(img, 75.0f, 40.0f, 0.5f, NULL, 0.65f, 0.65f);
            }
        } 
        else if (currentState == STATE_MENU) {
            C2D_TargetClear(topScreen, C2D_Color32(180, 25, 25, 255));
            if (menuSheet) {
                C2D_Image logoImg = C2D_SpriteSheetGetImage(menuSheet, 0);
                C2D_DrawImageAt(logoImg, 80.0f, 60.0f, 0.5f, NULL, 0.5f, 0.5f);
            }
        }
        else if (currentState == STATE_GAMEPLAY) {
            C2D_TargetClear(topScreen, C2D_Color32(40, 40, 40, 255));
        }

        C2D_TargetClear(bottomScreen, C2D_Color32(20, 20, 20, 255));
        C2D_SceneBegin(bottomScreen);

        if (currentState == STATE_MENU && buttonSheet) {
            for (int i = 0; i < totalButtons; i++) {
                C2D_Image btnImg = C2D_SpriteSheetGetImage(buttonSheet, i);
                C2D_DrawImageAt(btnImg, buttonBoxes[i].x, buttonBoxes[i].y, 0.5f, NULL, 0.75f, 0.6f);
            }
        }

        C3D_FrameEnd(0);
    }

    if (audioLoaded) {
        ndspChnReset(0);
        linearFree(bgm.audioData);
    }
    ndspExit();

    if (loadingSheet) C2D_SpriteSheetFree(loadingSheet);
    if (menuSheet)   C2D_SpriteSheetFree(menuSheet);
    if (buttonSheet) C2D_SpriteSheetFree(buttonSheet);
    
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
