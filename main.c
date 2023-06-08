#include <gsKit.h>
#include <stdio.h>
#include <sifrpc.h>
#include <libpad.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <elf-loader.h>

// Colors
static u64 Black __attribute__((unused)) = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00);
static u32 WhiteFont __attribute__((unused)) = GS_SETREG_RGBAQ(0xEB,0xEB,0xEB,0x80,0x00);

// GS
GSGLOBAL *gsGlobal;
GSFONTM *gsFontM;

// pad_dma_buf is provided by the user, one buf for each pad
// contains the pad's current state
static char padBuf[256] __attribute__((aligned(64)));
static char actAlign[6];
static int actuators;

static void waitPadReady(int port, int slot) {
    int state;
    char stateString[16];

    do {
        state = padGetState(port, slot);
        if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) {
            padStateInt2String(state, stateString);
        }
    } while (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1);
}

static int initializePad(int port, int slot) {
    int modes;
    int i;

    waitPadReady(port, slot);

    modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
    if (modes > 0) {
        for (i = 0; i < modes; i++) {
            padInfoMode(port, slot, PAD_MODETABLE, i);
        }

        i = 0;
        while (i < modes && padInfoMode(port, slot, PAD_MODETABLE, i) != PAD_TYPE_DUALSHOCK) {
            i++;
        }

        if (i < modes) {
            padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
            waitPadReady(port, slot);
            padInfoPressMode(port, slot);
            waitPadReady(port, slot);
            padEnterPressMode(port, slot);
            waitPadReady(port, slot);
            actuators = padInfoAct(port, slot, -1, 0);

            if (actuators != 0) {
                memset(actAlign, 0xff, sizeof(actAlign));
                actAlign[0] = 0;
                actAlign[1] = 1;

                waitPadReady(port, slot);
                padSetActAlign(port, slot, actAlign);
            }

            waitPadReady(port, slot);
        }
    }

    return 1;
}

static void InitScreen() {
	gsGlobal = gsKit_init_global();

	gsGlobal->PrimAAEnable 	  = GS_SETTING_ON;
	gsGlobal->DoubleBuffering = GS_SETTING_OFF;
	gsGlobal->ZBuffering      = GS_SETTING_OFF;

	gsKit_init_screen(gsGlobal);

	gsKit_mode_switch(gsGlobal, GS_ONESHOT);

	gsFontM = gsKit_init_fontm();

	gsKit_fontm_upload(gsGlobal, gsFontM);

	gsFontM->Spacing = 0.95f;
}

static void InitPS2() {
    SifInitRpc(0);
    while(!SifIopReset("", 0)){};
    while(!SifIopSync()){};
    SifInitRpc(0);
    SifLoadFileInit();

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    padInit(0);
    padPortOpen(0, 0, padBuf);
}

int main() {
    struct padButtonStatus buttons;
    u32 new_pad;
    char* filename = "none";
    char* gamename = "none";
    int executing = 0;

    InitPS2();
    initializePad(0, 0);
    InitScreen();

    while (1) {
        gsKit_clear(gsGlobal, Black);

        if (executing == 1) {
            executing = 2;
            LoadELFFromFile(filename, 0, NULL);
        }

        char tempstr[64];
        sprintf(tempstr, "Selected: %s\n", gamename);
        gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 10, 1, 0.7f, WhiteFont, "Welcome to Sestain's bootleg loader\n\n");
        gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 30, 1, 0.7f, WhiteFont, tempstr);
        gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 50, 1, 0.7f, WhiteFont, "Press start to boot game\n");

        if (padRead(0, 0, &buttons) != 0) {
            new_pad = 0xffff ^ buttons.btns;

            if (new_pad & PAD_CROSS) {
                gamename = "Crash Twinsanity PAL"; // PAL
                filename = "cdrom0:\\SLES_525.68;1"; 
            }  
            else if (new_pad & PAD_CIRCLE) {
                gamename = "Crash Twinsanity NTSC-J"; // NTSC-J
                filename = "cdrom0:\\SLPM_658.01;1"; 
            }
            else if (new_pad & PAD_SQUARE) {
                gamename = "Crash Twinsanity NTSC-U 1"; // NTSC-U
                filename = "cdrom0:\\SLUS_209.09;1"; 
            }
            else if (new_pad & PAD_TRIANGLE) {
                gamename = "Crash Twinsanity NTSC-U 2"; // NTSC-U 2.0
                filename = "cdrom0:\\SLUS_209.09_2;1"; 
            }
            if (new_pad & PAD_START) {
                if (strcmp(gamename, "none") != 0 && executing != 2)
                    executing = 1;
            }
        }

        if (executing > 0) {
            char tempstr2[64];
            sprintf(tempstr2, "Executing: %s", filename);
            gsKit_fontm_print_scaled(gsGlobal, gsFontM, 10, 70, 1, 0.7f, WhiteFont, tempstr2);
        }

        gsKit_sync_flip(gsGlobal);
        gsKit_queue_exec(gsGlobal);
    }

    return 0;
}