#include <gsKit.h>
#include <stdio.h>
#include <sifrpc.h>
#include <libpad.h>
#include <libcdvd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <elf-loader.h>
#include <sbv_patches.h>
#include <unistd.h>

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
    gsGlobal = gsKit_init_global();
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsKit_init_screen(gsGlobal);

    gsFontM = gsKit_init_fontm();
    gsKit_fontm_upload(gsGlobal, gsFontM);
    gsFontM->Spacing = 0.9f;

    // Reset the IOP
	SifInitRpc(0);
	while (!SifIopReset("", 0));

	// Sync with the IOP
	while (!SifIopSync());
	SifInitRpc(0);

	// Enable the patch that lets us load modules from memory
	sbv_patch_enable_lmb();

    // Load sio2man & padman drivers (Better in someway than the ones in bios?)
    SifExecModuleBuffer(&sio2man_irx, size_sio2man_irx, 0, 0, 0);
    SifExecModuleBuffer(&padman_irx, size_padman_irx, 0, 0, 0);

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
    u32 new_pad;
    constexpr GameInfo gameInfos[] = {
        {"Crash Twinsanity PAL", "SLES_525.68;1", PAD_CROSS},
        {"Crash Twinsanity NTSC-J", "SLPM_658.01;1", PAD_CIRCLE},
        {"Crash Twinsanity NTSC-U", "SLUS_209.09;1", PAD_SQUARE},
        {"Crash Twinsanity NTSC-U 2.0", "SLUS_209.09_2;1", PAD_TRIANGLE}
    };

    const char* gamename{ "none" };
    const char* filename{ "none" };
    bool executing{ false };
    int screenUpdateNeeded{ 1 };

    Init();

    while (1) {
        if (executing) {
            // Executes ELF binary from disc, if not found, then returns to PS2 Browser. (Shows debug colors on real hardware)
            LoadELFFromFile(filename, 0, nullptr);
        }

        if (padRead(0, 0, &buttons) != 0) {
            new_pad = 0xffff ^ buttons.btns;

            for (size_t i = 0; i < sizeof(gameInfos) / sizeof(gameInfos[0]); ++i) {
                if (new_pad & gameInfos[i].buttonMask) {
                    gamename = gameInfos[i].name;

                    // Construct the CD-ROM path with the specified format
                    char tempBuffer[256];
                    snprintf(tempBuffer, sizeof(tempBuffer), "cdrom0:\\%s", gameInfos[i].serial);
                    filename = tempBuffer;

                    screenUpdateNeeded = 1;
                }
            }

            if (new_pad & PAD_START) {
                if (strcmp(gamename, "none") != 0 && !executing) {
                    executing = !executing;
                    screenUpdateNeeded = 1;
                }
            }
        }

        // Check if a screen update is needed
        if (screenUpdateNeeded) {
            // Perform rendering operations
            gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00));

            RenderUI(gamename, executing);

            // Refresh display
            gsKit_sync_flip(gsGlobal);
            gsKit_queue_exec(gsGlobal);

            screenUpdateNeeded++;
            // Reset the flag after rendering
            if (screenUpdateNeeded > 2)
                screenUpdateNeeded = 0;
        }
        
        // Sleeping so fps is half of Vsync
        usleep(25000);
    }

    // Unreachable
    return 0;
}