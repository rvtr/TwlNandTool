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
	- Write good NAND read routine
	- Detect debugger vs dev
	- Create FAT

		Okay so this might be a bad idea in my hands lol
		Find an FS lib? No. Fuck no.

		Paste this (encrypted) into sector 0x877 (0x10EE00).

		unsigned char twlMain[56] = {
			0xE9, 0x00, 0x00, 0x54, 0x57, 0x4C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
			0x02, 0x20, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0xF8, 0x34, 0x00,
			0x20, 0x00, 0x10, 0x00, 0x77, 0x08, 0x00, 0x00, 0x89, 0x6F, 0x06, 0x00,
			0x00, 0x00, 0x29, 0x78, 0x56, 0x34, 0x12, 0x20, 0x20, 0x20, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00
		};

		Pad to 0x200b and make sure to have 0x55AA (CRINGE I HATE MBR I HATE MBR I HATE MBR) <-- I don't even remember writing this.
		Now fill 0x200*B88b (total 0x171000) afterwards with zerobytes.

		Congrats. This is a formatted file system. Please don't hurt me.

		Okay but what about twl_photo? Uhhh nobody likes that.... oh fine.

		Paste this (encrypted) into sector 0x6784D (0xCF09A00)

		unsigned char twlPhoto[56] = {
			0xE9, 0x00, 0x00, 0x54, 0x57, 0x4C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
			0x02, 0x20, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0xF8, 0x09, 0x00,
			0x20, 0x00, 0x10, 0x00, 0x4D, 0x78, 0x06, 0x00, 0xB3, 0x05, 0x01, 0x00,
			0x01, 0x00, 0x29, 0x78, 0x56, 0x34, 0x12, 0x20, 0x20, 0x20, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00
		};

		Same thing but fill 0x200*13Ab (total 0x27400) afterwards with zerobytes.

		Okay lol how do I do the VBR padding to 0x200
		- Dump VBR start into a variable
		- for loop to add the rest of the zero bytes (0x1C8b)
		- end with 0x55AA

		Awesome!

	- Fancy windows like TWL EVA
	- System transfer (way later on)
*/

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
  MENUSTATE_FS_MENU,
  MENUSTATE_NF_MENU,
  MENUSTATE_NULL,
  MENUSTATE_TEST,
  MENUSTATE_TEST2,
  MENUSTATE_EXIT
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
    addMenuItem(m, "Debug1", NULL, 0, "Testing area");
    addMenuItem(m, "Debug2", NULL, 0, "Testing area");
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

	//DSi check
	if (!isDSiMode()) {
		messageBox("\x1B[31mError:\x1B[33m This app is only for DSi.");
		return 0;
	}

	//setup sd card access
	if (!fatInitDefault()) {
		messageBox("fatInitDefault()...\x1B[31mFailed\n\x1B[30m\n\nSome features will not work.");
		//return 0;
	}

	//setup sd card access
	if(!nitroFSInit(argv[0])) {
		if(!nitroFSInit("TwlNandTool.prod.srl") || !nitroFSGood()) {
			if(!nitroFSInit("TwlNandTool.dev.srl") || !nitroFSGood()) {
				if(!nitroFSInit("ntrboot.nds") || !nitroFSGood()) {
					while (true)
					{
						swiWaitForVBlank();
						scanKeys();

						if (keysDown() & KEY_SELECT )
							break;
					}
					messageBox("nitroFSInit()...\x1B[31mFailed\n\x1B[30m\nSome features will not work.\n\nTry placing the SRL for your DSion your SD card root like this:\n\nSDMC:/TwlNandTool.prod.srl\nSDMC:/TwlNandTool.dev.srl\nSDMC:/ntrboot.nds\n");
				}
			}
		}
	}

	//setup nand access
	if (!fatMountSimple("nand", &io_dsi_nand)) {
		messageBox("nand init \x1B[31mfailed\n\x1B[30m\n\nNAND must be repaired.");
	}

	clearScreen(cSUB);
 	clearScreen(cMAIN);

    int cursor = 0;

    while (!programEnd)
    {
        cursor = _mainMenu(cursor);

        switch (cursor)
        {

            case MENUSTATE_FS_MENU:
                fsMain();
                break;

            case MENUSTATE_NF_MENU:
                nfMain();
                break;

            case MENUSTATE_NULL:
                break;

            case MENUSTATE_TEST:
                debug1();
                break;

            case MENUSTATE_TEST2:
                debug2();
                break;

            case MENUSTATE_EXIT:
                programEnd = true;
                break;
        }
    }

    clearScreen(cSUB);
    printf("Unmounting NAND...\n");
    fatUnmount("nand:");
    printf("Merging stages...\n");
    nandio_shutdown();

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

	iprintf("\n\n  Please Push Select To Return  ");

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_SELECT )
			break;
	}
}

int debug2(void) {

	clearScreen(cSUB);

	iprintf("\n>> Debug1");
	iprintf("\n Stupid AES-CTR BS              ");
	iprintf("\n--------------------------------");

	nand_ReadSectors(877, 1, sector_buf);
	dsi_nand_crypt(sector_buf, sector_buf, 877, SECTOR_SIZE / AES_BLOCK_SIZE);

    printf("\n     ");
    for (int i = 452; i < SECTOR_SIZE; i++) {
        printf("%02X", sector_buf[i]);
        if ((i + 1) % 2 == 0) {
            printf(" ");
        }
        if ((i - 443) % 8 == 0 && i != 444) {
            printf("\n     ");
        }
    }

	iprintf("\n\n  Read in");

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_SELECT )
			break;
	}

	dsi_nand_crypt(sector_buf, sector_buf, 877, SECTOR_SIZE / AES_BLOCK_SIZE);

    printf("\n     ");
    for (int i = 444; i < SECTOR_SIZE; i++) {
        printf("%02X", sector_buf[i]);
        if ((i + 1) % 2 == 0) {
            printf(" ");
        }
        if ((i - 443) % 8 == 0 && i != 444) {
            printf("\n     ");
        }
    }

	iprintf("\n\n  Read out");

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_SELECT )
			break;
	}
}

void clearScreen(enum console c)
{
	consoleSet(c);
	iprintf("\x1b[2J");
}
