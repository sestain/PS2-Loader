#include <gsKit.h>
#include <stdio.h>
#include <sifrpc.h>
#include <libpad.h>
#include <libcdvd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <elf-loader.h>
#include <sbv_patches.h>
#include <kernel.h>
#include <ps2sdkapi.h>

// #include "elf.h"

struct GameInfo {
    const char* name;
    const char* serial;
    u32 buttonMask;
};

char padBuf[256] __attribute__((aligned(64)));
constexpr u32 WhiteFont = GS_SETREG_RGBAQ(0xEB, 0xEB, 0xEB, 0x80, 0x00);

GSGLOBAL* gsGlobal;
GSFONTM* gsFontM;

extern unsigned char sio2man_irx[] __attribute__((aligned(16)));
extern const unsigned int size_sio2man_irx;

extern unsigned char padman_irx[] __attribute__((aligned(16)));
extern const unsigned int size_padman_irx;

void Init() {
    // Reset the IOP
	SifInitRpc(0);
	while (!SifIopReset(NULL, 0)) {}

	// Sync with the IOP
	while (!SifIopSync()) {} 
	SifInitRpc(0);

	// Enable the patch that lets us load modules from memory
	sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    // Load sio2man & padman drivers (Better in someway than the ones in bios?)
    SifExecModuleBuffer(&sio2man_irx, size_sio2man_irx, 0, 0, 0);
    SifExecModuleBuffer(&padman_irx, size_padman_irx, 0, 0, 0);
    
    gsGlobal = gsKit_init_global();
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsGlobal->DoubleBuffering = GS_SETTING_OFF;
    gsKit_init_screen(gsGlobal);
    gsKit_sync_flip(gsGlobal);

    gsFontM = gsKit_init_fontm();
    gsKit_fontm_upload(gsGlobal, gsFontM);
    gsFontM->Spacing = 0.9f;

    // Enables controller
    padInit(0);
    padPortOpen(0, 0, padBuf);
    
    // Stops disc from spinning once everything is loaded
    sceCdStop();
    sceCdSync(0);
}

void RenderUI(const char* gamename, const bool executing) {
    static char tempstr[64];
    static char tempstr2[64];
    snprintf(tempstr, sizeof(tempstr), "Selected: %s", gamename);
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 10, 1, 0.7f, WhiteFont, "Welcome to Sestain's bootleg loader");
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 30, 1, 0.7f, WhiteFont, tempstr);
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 50, 1, 0.7f, WhiteFont, "Press start to boot game");

    if (executing) {
        snprintf(tempstr2, sizeof(tempstr2), "Booting: %s", gamename);
        gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 70, 1, 0.7f, WhiteFont, tempstr2);
    }
}

int main() {
    struct padButtonStatus buttons;
    u32 buttonState;
    constexpr GameInfo gameInfos[] = {
        {"Crash Twinsanity PAL", "SLES_525.68;1", PAD_CROSS},
        {"Crash Twinsanity NTSC-J", "SLPM_658.01;1", PAD_CIRCLE},
        {"Crash Twinsanity NTSC-U", "SLUS_209.09;1", PAD_SQUARE},
        {"Crash Twinsanity NTSC-U 2.0", "SLUS_209.09_2;1", PAD_TRIANGLE}
    };

    const char* gamename{ "none" };
    const char* filename{ "none" };
    bool executing{ false };
    bool screenUpdateNeeded{ true };

    Init();

    while (1) {
        if (executing) {
            char tempBuffer[32];
            snprintf(tempBuffer, sizeof(tempBuffer), "cdrom0:\\%s", filename);

            // Load and execute the ELF file, if not found, then returns to PS2 Browser. (Shows debug colors on real hardware [Now only if in debug mode])
            LoadELFFromFile(tempBuffer, 0, nullptr);

            gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 90, 1, 0.7f, WhiteFont, "Error, game not found");
            gsKit_queue_exec(gsGlobal);
            executing = false;
            sceCdStop();
            sceCdSync(0);
        }

        if (padRead(0, 0, &buttons) != 0) {
            buttonState = 0xffff ^ buttons.btns;

            for (size_t i = 0; i < sizeof(gameInfos) / sizeof(gameInfos[0]); ++i) {
                if (buttonState & gameInfos[i].buttonMask) {
                    gamename = gameInfos[i].name;
                    filename = gameInfos[i].serial;
                    screenUpdateNeeded = true;
                }
            }

            if (buttonState & PAD_START) {
                if (strcmp(gamename, "none") != 0 && !executing) {
                    executing = !executing;
                    screenUpdateNeeded = true;
                }
            }
        }

        // Check if a screen update is needed
        if (screenUpdateNeeded) {
            // Perform rendering operations
            gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00));

            RenderUI(gamename, executing);

            // Refresh display
            gsKit_queue_exec(gsGlobal);

            // Reset the flag after rendering
            screenUpdateNeeded = false;
        }
        
        // Sleeping so fps is half of Vsync (Duration is in microseconds)
        usleep(40000);
    }

    // Unreachable
    return 0;
}

#ifndef SKIP_SIZE_REDUCTION_MACROS
void _libcglue_init() {}
void _libcglue_deinit() {}
// void _libcglue_args_parse() {} // dont use this if your app depends on relative paths accesing
DISABLE_PATCHED_FUNCTIONS();
DISABLE_EXTRA_TIMERS_FUNCTIONS();
PS2_DISABLE_AUTOSTART_PTHREAD();
#endif
