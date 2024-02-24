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
const u32 WhiteFont = GS_SETREG_RGBAQ(0xEB, 0xEB, 0xEB, 0x80, 0x00);

GSGLOBAL* gsGlobal;
GSFONTM* gsFontM;

extern u8 sio2man_irx[];
extern int size_sio2man_irx;

extern u8 padman_irx[];
extern int size_padman_irx;

void Init() {
    SifInitRpc(0);
    SifIopReset("", 0);
    while (!SifIopSync());
    SifInitRpc(0);

    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    SifExecModuleBuffer(&sio2man_irx, size_sio2man_irx, 0, nullptr, nullptr);
    SifExecModuleBuffer(&padman_irx, size_padman_irx, 0, nullptr, nullptr);

    gsGlobal = gsKit_init_global();
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsKit_init_screen(gsGlobal);

    gsFontM = gsKit_init_fontm();
    gsKit_fontm_upload(gsGlobal, gsFontM);
    gsFontM->Spacing = 0.9f;

    padInit(0);
    padPortOpen(0, 0, padBuf);

    sceCdStop();
    sceCdSync(0);
}

void RenderUI(const char* gamename) {
    char tempstr[64];
    snprintf(tempstr, sizeof(tempstr), "Selected: %s", gamename);
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 10, 1, 0.7f, WhiteFont, "Welcome to Sestain's bootleg loader");
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 30, 1, 0.7f, WhiteFont, tempstr);
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 50, 1, 0.7f, WhiteFont, "Press start to boot game");
}

int main() {
    struct padButtonStatus buttons;
    u32 new_pad;
    const GameInfo gameInfos[] = {
        {"Crash Twinsanity PAL", "SLES_525.68;1", PAD_CROSS},
        {"Crash Twinsanity NTSC-J", "SLPM_658.01;1", PAD_CIRCLE},
        {"Crash Twinsanity NTSC-U", "SLUS_209.09;1", PAD_SQUARE},
        {"Crash Twinsanity NTSC-U 2.0", "SLUS_209.09_2;1", PAD_TRIANGLE}
    };

    const char* gamename = "none";
    const char* filename = "none";
    bool executing = false;

    Init();

    while (1) {
        if (executing) {
            // Executes ELF binary from disc, if not found exits and returns to PS2 Browser. (Shows debug colors on real hardware)
            LoadELFFromFile(filename, 0, nullptr);
        }

        gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00));

        RenderUI(gamename);

        if (padRead(0, 0, &buttons) != 0) {
            new_pad = 0xffff ^ buttons.btns;

            for (size_t i = 0; i < sizeof(gameInfos) / sizeof(gameInfos[0]); ++i) {
                if (new_pad & gameInfos[i].buttonMask) {
                    gamename = gameInfos[i].name;

                    // Construct the CD-ROM path with the specified format
                    char tempBuffer[256];
                    snprintf(tempBuffer, sizeof(tempBuffer), "cdrom0:\\%s", gameInfos[i].serial);
                    filename = tempBuffer;
                }
            }

            if (new_pad & PAD_START) {
                if (strcmp(gamename, "none") != 0 && !executing)
                    executing = !executing;
            }
        }

        if (executing) {
            char tempstr2[64];
            snprintf(tempstr2, sizeof(tempstr2), "Booting: %s", gamename);
            gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 70, 1, 0.7f, WhiteFont, tempstr2);
        }

        // Refresh display
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_exec(gsGlobal);
        
        // Sleeping so fps is half of Vsync
        usleep(25000);
    }

    return 0;
}