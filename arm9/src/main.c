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
#include "nand/chipinfo.h"
#include "nand/sysfile.h"
#include "nand/hwinfo.h"
#include "video.h"
#include "audio.h"
#include "nitrofs.h"
#include "font.h"
#include <stdlib.h>
#include <stdio.h>

/*

	TODO:
	- LESS NAND WRITES!!!!!
	- Detect debugger vs dev (less important currently)
	- HWInfo verify
	- HWInfo sign (can wait for a very long time)
	- Why doesn't unmounting NAND get reflected in the file test?
	- NandFirm hash checking <-- very very very important!
	- Make dummy TWLCFG, launcher saves, etc
	- product.log reading and writing
	- TAD stuff
		- Title verification (exception for HNAG)
		- PUB/PRV formatting and testing (cringe)
		- Getting title lists
		- Cut over 39 titles
	- Unlaunch stuff
		- Repair bricked TMD (depends on title verification)
		- Install unlaunch through the safe method

	COSMETIC OR OPTIONAL:
	- Back up/restore screen memory
	- System transfer (way later on)
	
*/
extern bool nand_Startup();
extern bool sdio_Startup();
bool legitHWInfo = false;
bool nandMounted = false;
bool nandPhotoMounted = false;
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
  STARTMENU_SYSFILE_MENU,
  STARTMENU_CHIP_MENU,
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

	addMenuItem(m, "FileSystem Menu", NULL, 0, "Options such as repairing MBR\n and formatting TWL_MAIN/PHOTO.");
	addMenuItem(m, "NandFirm Menu", NULL, 0, "NandFirm (stage2) installers\n and verification.");
	addMenuItem(m, "Sys File Menu", NULL, 0, "Create/recover system files\n like HWInfo, FontTable, and\n cert.sys");
	addMenuItem(m, "Chip Info Menu", NULL, 0, "Info on the NAND and CPU.");
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
	soundInit();

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
	mountNAND(false);
	mountNAND(true);
	mountNitroFS();
	agingMode = false;
	mkdir("sd:/TwlNandTool", 0777);

	// 
	swiWaitForVBlank();
	scanKeys();

	if (keysDown() & KEY_START || keysDown() & KEY_SELECT) {
	} else {
		//recoverHWInfoDeep();
		//debug3();
	}

	clearScreen(cSUB);
 	clearScreen(cMAIN);

 	soundPlayStartup();
 	wait(10);

	int cursor = 0;

	while (!programEnd)
	{
		cursor = _mainMenu(cursor);

		switch (cursor)
		{

			case STARTMENU_FS_MENU:
				soundPlaySelect();
				fsMain();
				break;

			case STARTMENU_NF_MENU:
				soundPlaySelect();
				nfMain();
				break;

			case STARTMENU_SYSFILE_MENU:
				soundPlaySelect();
				sysfileMain();
				break;

			case STARTMENU_CHIP_MENU:
				soundPlaySelect();
				chipMain();
				break;

			case STARTMENU_NULL:
				soundPlaySelect();
				break;

			case STARTMENU_TEST:
				soundPlaySelect();
				debug1();
				break;

			case STARTMENU_TEST2:
				soundPlaySelect();
				debug2();
				break;

			case STARTMENU_TEST3:
				soundPlaySelect();
				debug3();
				break;

			case STARTMENU_EXIT:
				soundPlaySelect();
				programEnd = true;
				break;
		}
	}

	clearScreen(cSUB);
	agingMode = true;
	unmountNAND(false);
	unmountNAND(true);
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
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Debug1");
	iprintf("\n Show font					  ");
	iprintf("\n--------------------------------");

	for (int i = 0; i <= 200; i++) {
		printf("%c ", (char)i);
	}

	exitFunction();
	return success;
}

int debug2(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Corrupt MBR				  ");
	iprintf("\n--------------------------------");	
	memset(sector_buf, 0, 0x200);
	iprintf("\nWriting new MBR...");
	nand_WriteSectors(0, 1, sector_buf);
	iprintf("\nTesting new MBR...");
	nand_ReadSectors(0, 1, sector_buf);
	dsi_nand_crypt(sector_buf, sector_buf, 0, SECTOR_SIZE / AES_BLOCK_SIZE);
	if(!parse_mbr(sector_buf, is3DS)) {
		iprintf("\n\n	\x1B[31mERROR!\x1B[30m Failed to break MBR.");
		success = false;
	} else {
		iprintf("\n\x1B[32mMBR corrupted okay!\x1B[30m");
	}

	exitFunction();
	return success;
}

int debug3(void) {
	bool agingSuccess = false;
	agingMode = true;
	clearScreen(cSUB);

	iprintf("\n>> NAND AGING tester			");
	iprintf("\n--------------------------------");

	if (cpuPrintInfo()) {
		if (nandPrintInfo()) {
			if (importNandFirm(1)) {
				if (readNandFirm()) {
					if (repairMbr(true)) {
						if (readMbr()) {
							if (mountNAND(false) || (formatMain() && mountNAND(false))) {
								if (mountNAND(true) || (formatPhoto() && mountNAND(true))) {
									if (recoverHWInfo(false) || recoverHWInfoDeep()) {
										if (filetestNAND(false)) {
											if (filetestNAND(true)) {
												if (makeSystemFolders()) {
													if (makeCertChain()) {
														if (makeFontTable()) {
															if (unmountNAND(false)) {
																if (unmountNAND(true)) {
																	if (filetestNitro()) {	
																		agingSuccess = true;
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	agingMode = false;
	clearScreen(cSUB);
	iprintf("\n>> NAND AGING tester result	 ");
	iprintf("\n--------------------------------");
	success = agingSuccess;
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
