#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <nds/arm9/nand.h>
#include "f_xy.h"
#include "twltool/dsi.h"
#include "nandio.h"
#include "chipinfo.h"
#include "sector0.h"
#include "crypto.h"
#include "../message.h"
#include "../main.h"
#include "../video.h"
#include "../audio.h"

static size_t i;

enum {
	CHIPMENU_NAND_INFO,
	CHIPMENU_CPU_INFO
};

static int _chipMenu(int cursor)
{

    Menu* m = newMenu();
    setMenuHeader(m, "TwlNandTool");
    setListHeader(m, "Chip Info");

	addMenuItem(m, "CID Info", NULL, 0, "Get NAND chip information.");
	addMenuItem(m, "ConsoleID Info", NULL, 0, "Get CPU information.");

	m->cursor = cursor;

	//bottom screen
	printMenu(m, 1);

	while (!programEnd)
	{
		swiWaitForVBlank();
		scanKeys();

		if (moveCursor(m))
			printMenu(m, 1);

		if (keysDown() & KEY_A)
			break;

		if (keysDown() & KEY_B) {
			soundPlayBack();
			programEnd = true;
		}
	}

	int result = m->cursor;
	freeMenu(m);

	return result;
}

int chipMain(void)
{

	int cursor = 0;

	while (!programEnd)
	{
		cursor = _chipMenu(cursor);

		if (programEnd == false) {
			switch (cursor)
			{

				case CHIPMENU_NAND_INFO:
					soundPlaySelect();
					nandPrintInfo();
					break;

				case CHIPMENU_CPU_INFO:
					soundPlaySelect();
					cpuPrintInfo();
					break;
			}
		}
	}

	programEnd = false;

	return 0;
}

bool nandPrintInfo(void) {
	success = true;
	extern nandData nandInfo;
	clearScreen(cSUB);
	iprintf("\n>> CID (Card IDentification)");
	iprintf("\n Brand : %s", nandInfo.NAND_MID_NAME);
	iprintf("\n--------------------------------");
	iprintf("\nManufacturer ID    : 0x%02X", nandInfo.NAND_MID);
	iprintf("\nOEM/Application ID : 0x%02X%02X", nandInfo.NAND_OID[0], nandInfo.NAND_OID[1]);
	iprintf("\nProduct name       : %s", nandInfo.NAND_PNM);
	iprintf("\nProduct revision   : %02X", nandInfo.NAND_PRV);
	iprintf("\nProduct S/N        : %02X%02X%02X%02X", nandInfo.NAND_PSN[0], nandInfo.NAND_PSN[1], nandInfo.NAND_PSN[2], nandInfo.NAND_PSN[3]);
	iprintf("\nManufacturing date : %02X(%02d 20%02d)",nandInfo.NAND_MDT, nandInfo.NAND_MDT_MONTH, nandInfo.NAND_MDT_YEAR);
    printf("\n\n     ");
    for (i = 16; i > 0;) {
    	i--;
        if ((i + 1) % 2 == 0) {
            printf(" ");
        }
        printf("%02X", CID[i]);
        if (i == 8) {
            printf("\n     ");
        }
    }

	exitFunction();
	return success;
}

bool cpuPrintInfo(void) {
	success = true;
	extern nandData nandInfo;
	clearScreen(cSUB);
	iprintf("\n>> ConsoleID (CPU ID)           ");
	iprintf("\n--------------------------------");
    iprintf("\n     ");
    for (i = 8; i > 0;) {
    	i--;
        if ((i + 1) % 2 == 0) {
            printf(" ");
        }
        printf("%02X", consoleID[i]);
        if (i == 8) {
            printf("\n     ");
        }
    }
    iprintf("\n\nManufacturing date range.\nThis is only an estimate!\n\n ");
    iprintf("20%02d/%02d to 20%02d/%02d\n", cpuInfo.CPU_START_YEAR, cpuInfo.CPU_START_MONTH, cpuInfo.CPU_END_YEAR, cpuInfo.CPU_END_MONTH);

	exitFunction();
	return success;
}