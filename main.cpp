#include <gsKit.h>
#include <stdio.h>
#include <sifrpc.h>
#include <libpad.h>
#include <libcdvd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <elf-loader.h>
#include <sbv_patches.h>

char padBuf[256] __attribute__((aligned(64)));
const u32 WhiteFont = GS_SETREG_RGBAQ(0xEB, 0xEB, 0xEB, 0x80, 0x00);

GSGLOBAL* gsGlobal;
GSFONTM* gsFontM;

extern u8 sio2man_irx[];
extern int size_sio2man_irx;

extern u8 padman_irx[];
extern int size_padman_irx;

void Init() {
    int ret;
    
    SifInitRpc(0);
    SifIopReset("", 0);
    while (!SifIopSync());
    SifInitRpc(0);

    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    SifExecModuleBuffer(&sio2man_irx, size_sio2man_irx, 0, nullptr, &ret);
    SifExecModuleBuffer(&padman_irx, size_padman_irx, 0, nullptr, &ret);

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

int main() {
    struct padButtonStatus buttons;
    u32 new_pad;
    const char* filename = "none";
    const char* gamename = "none";
    int executing = 0;
    
    Init();

    while (1) {
        gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00));

        if (executing == 1) {
            LoadELFFromFile(filename, 0, nullptr);
        }

        char tempstr[64];
        snprintf(tempstr, sizeof(tempstr), "Selected: %s", gamename);
        
        gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 10, 1, 0.7f, WhiteFont, "Welcome to Sestain's bootleg loader");
        gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 30, 1, 0.7f, WhiteFont, tempstr);
        gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 50, 1, 0.7f, WhiteFont, "Press start to boot game");

        if (padRead(0, 0, &buttons) != 0) {
            new_pad = 0xffff ^ buttons.btns;

            if (new_pad & PAD_CROSS) {
                gamename = "Crash Twinsanity PAL"; // PAL
                filename = "cdrom0:\\SLES_525.68;1";
            } else if (new_pad & PAD_CIRCLE) {
                gamename = "Crash Twinsanity NTSC-J"; // NTSC-J
                filename = "cdrom0:\\SLPM_658.01;1";
            } else if (new_pad & PAD_SQUARE) {
                gamename = "Crash Twinsanity NTSC-U"; // NTSC-U
                filename = "cdrom0:\\SLUS_209.09;1";
            } else if (new_pad & PAD_TRIANGLE) {
                gamename = "Crash Twinsanity NTSC-U 2.0"; // NTSC-U 2.0
                filename = "cdrom0:\\SLUS_209.09_2;1";
            }
            if (new_pad & PAD_START) {
                if (strcmp(gamename, "none") != 0 && executing != 1)
                    executing = 1;
            }
        }

        if (executing == 1) {
            char tempstr2[64];
            snprintf(tempstr2, sizeof(tempstr2), "Booting: %s", gamename);
            gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 70, 1, 0.7f, WhiteFont, tempstr2);
        }

        gsKit_sync_flip(gsGlobal);
        gsKit_queue_exec(gsGlobal);
    }

    return 0;
}