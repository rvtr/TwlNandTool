#include "main.h"
#include "menu.h"
#include "message.h"
#include "nand/nandio.h"
#include "storage.h"
#include "version.h"
#include <dirent.h>
#include <time.h>
#include "nand/filesystem.h"
#include "nand/nandfirm.h"
#include "video.h"
#include "nitrofs.h"
#include "font.h"
#include <stdlib.h>
#include <stdio.h>

/*

	TODO:
	- LESS NAND WRITES!!!!!
	- Write good NAND read routine
	- Detect debugger vs dev
	- Recover HWInfo
	- Back up/restore screen memory
	- System transfer (way later on)

	- Why doesn't unmounting NAND get reflected in the file test?

*/
extern bool nand_Startup();
extern bool sdio_Startup();
bool nandMounted = false;
bool sdMounted = false;
bool agingMode = false;
bool success = false;

bool programEnd = false;
bool sdnandMode = true;
bool unlaunchFound = false;
bool unlaunchPatches = false;
bool arm7Exiting = false;
bool charging = false;
u8 batteryLevel = 0;
u8 region = 0;
u32 consoleSign;
char consoleSignName[9];
char consoleType[9];

PrintConsole topScreen;
PrintConsole bottomScreen;

typedef enum {
  STARTMENU_FS_MENU,
  STARTMENU_NF_MENU,
  STARTMENU_NULL,
  STARTMENU_TEST,
  STARTMENU_TEST2,
  STARTMENU_TEST3,
  STARTMENU_EXIT
} MenuState;

static int _mainMenu(int cursor)
{
	clearScreen(cSUB);
 	clearScreen(cMAIN);

    //menu
    Menu* m = newMenu();
    setMenuHeader(m, "TwlNandTool");
    setListHeader(m, "START MENU");

    char modeStr[32];
    addMenuItem(m, "FileSystem Menu", NULL, 0, "Options such as repairing MBR\n and formatting twl_main/photo.");
    addMenuItem(m, "NandFirm menu", NULL, 0, "NandFirm (stage2) installers\n and version testing.");
    addMenuItem(m, "---------------", NULL, 0, "");
    addMenuItem(m, "Debug1", NULL, 0, "Font display.");
    addMenuItem(m, "Debug2", NULL, 0, "MBR corruption test.");
    addMenuItem(m, "Debug3", NULL, 0, "NAND AGING test.");
    addMenuItem(m, "Exit", NULL, 0, "Leave the program.");

    m->cursor = (cursor);

    //bottom screen
    printMenu(m, 0);

    while (!programEnd)
    {
        swiWaitForVBlank();
        scanKeys();

        if (moveCursor(m))
            printMenu(m, 0);

        if (keysDown() & KEY_A)
            break;
    }

    int result = m->cursor;
    freeMenu(m);

    return result;
}

void fifoHandlerPower(u32 value32, void* userdata)
{
	if (value32 == 0x54495845) // 'EXIT'
	{
		programEnd = true;
		arm7Exiting = true;
	}
}

void fifoHandlerBattery(u32 value32, void* userdata)
{
	batteryLevel = value32 & 0xF;
	charging = (value32 & BIT(7)) != 0;
}

int main(int argc, char **argv)
{

	fifoWaitValue32(FIFO_USER_01);
    consoleSign = fifoGetValue32(FIFO_USER_01);

    if (consoleSign == 0x00) {
        strcpy(consoleSignName, "prod");
    } else {
        strcpy(consoleSignName, "dev");
    }

    if (consoleSign == 0x00) {
        strcpy(consoleType, "Retail");
    } else if (consoleSign == 0x02) {
        strcpy(consoleType, "Panda");
    }/*

	Do some check here to determine retail, panda, and debugger.
	Then block debuggers due to different FW and a higher chance of messing stuff up without an easy fix.

    else if (consoleSign == 0x02 || ) {
		strcpy consoleType, "Debugger"
    }
    */

    videoInit();

    srand(time(0));
    keysSetRepeat(25, 5);
    //_setupScreens();

    fifoSetValue32Handler(FIFO_USER_01, fifoHandlerPower, NULL);
    fifoSetValue32Handler(FIFO_USER_03, fifoHandlerBattery, NULL);

    iprintf("\x1B[30m");

	if (!isDSiMode()) {
		messageBox("\x1B[31mError:\x1B[30m This app is only for DSi.");
		return 0;
	}

	agingMode = true;

	if (!sdio_Startup())
	{
		messageBox("\n\x1B[31mERROR: \x1B[30mFailed to mount SD!\n\nSome features will not work.");
	}

	if (!nand_Startup())
	{
		messageBox("\n\x1B[31mFATAL:\x1B[30mStart NAND failed!\nPlease make an issue on GitHub.\n\nThe program will end soon.");
	}

	agingMode = true;
	mountMain();
	mountNitroFS();
	agingMode = false;

	clearScreen(cSUB);
 	clearScreen(cMAIN);

    int cursor = 0;

    while (!programEnd)
    {
        cursor = _mainMenu(cursor);

        switch (cursor)
        {

            case STARTMENU_FS_MENU:
                fsMain();
                break;

            case STARTMENU_NF_MENU:
                nfMain();
                break;

            case STARTMENU_NULL:
                break;

            case STARTMENU_TEST:
                debug1();
                break;

            case STARTMENU_TEST2:
                debug2();
                break;

            case STARTMENU_TEST3:
                debug3();
                break;

            case STARTMENU_EXIT:
                programEnd = true;
                break;
        }
    }

    clearScreen(cSUB);
    printf("Unmounting NAND...\n");
	if(nandMounted) {
		fatUnmount("nand");
	}
	// I think this was some safety thing but it wants to re-write the entire NAND...
	// I'm the one person always saying "NANDs aren't that weak", so you know it's excessive when I comment it out.

    //printf("Merging stages...\n");
    //nandio_shutdown();

    fifoSendValue32(FIFO_USER_02, 0x54495845); // 'EXIT'

    while (arm7Exiting)
        swiWaitForVBlank();

    return 0;
}

int debug1(void) {

	clearScreen(cSUB);

	iprintf("\n>> Debug1");
	iprintf("\n Show font                      ");
	iprintf("\n--------------------------------");

    for (int i = 0; i <= 200; i++) {
        printf("%c ", (char)i);
    }

	exitFunction();
}

int debug2(void) {
	clearScreen(cSUB);

	iprintf("\n>> Corrupt MBR                  ");
	iprintf("\n--------------------------------");	
	memset(sector_buf, 0, 0x200);
	iprintf("\nWriting new MBR...");
	nand_WriteSectors(0, 1, sector_buf);
    iprintf("\nTesting new MBR...");
	nand_ReadSectors(0, 1, sector_buf);
	dsi_nand_crypt(sector_buf, sector_buf, 0, SECTOR_SIZE / AES_BLOCK_SIZE);
    if(!parse_mbr(sector_buf, is3DS)) {
    	iprintf("\n\n    \x1B[31mERROR!\x1B[30m Failed to break MBR.");
    } else {
    	iprintf("\n\x1B[32mMBR corrupted okay!\x1B[30m");
    }

	exitFunction();
}

int debug3(void) {
	success = true;
	agingMode = true;
	clearScreen(cSUB);

	iprintf("\n>> NAND AGING tester            ");
	iprintf("\n--------------------------------");

	nandPrintInfo();
	if (!nandFirmImport(true)) {
        success = false;
    }
    nandFirmRead();
	if (!repairMbr(true)) {
        success = false;
    }
    readMbr();

	if (success == true && !mountMain()) {
		if (!formatMain() && !formatPhoto() && !mountMain()) {
			iprintf("\nNAND mount failed!");
			success = false;
		} else {
			nandMounted = true;
		}
	} else if (nandMounted == true) {
		iprintf("\nNAND already mounted.");
	} else {
		nandMounted = true;
	}
	filetestMain();
	unmountMain();
	filetestNitro();

	agingMode = false;

	iprintf("\n>> NAND AGING tester result     ");
	iprintf("\n--------------------------------");

	if (success == true) {
		iprintf("\nAll tests passed okay.");
	} else {
		iprintf("\nTests failed!");
	}

	exitFunction();
	return 1;
}

void clearScreen(enum console c)
{
	consoleSet(c);
	iprintf("\x1b[2J");
}
