#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <nds/arm9/nand.h>
#include "f_xy.h"
#include "twltool/dsi.h"
#include "nandio.h"
#include "nandfirm.h"
#include "sector0.h"
#include "crypto.h"
#include "filesystem.h"
#include "../message.h"
#include "../main.h"
#include "../video.h"
#include "../menu.h"

// Master Boot Records
u8 mbrSamsung[68] = {
	0x00, 0x00, 0x00, 0x03, 0x18, 0x04, 0x06, 0x0F, 0xE0, 0x3B, 0x77, 0x08,
	0x00, 0x00, 0x89, 0x6F, 0x06, 0x00, 0x00, 0x02, 0xCE, 0x3C, 0x06, 0x0F,
	0xE0, 0xBE, 0x4D, 0x78, 0x06, 0x00, 0xB3, 0x05, 0x01, 0x00, 0x00, 0x02,
	0xDE, 0xBF, 0x01, 0x0F, 0xE0, 0xBF, 0x5D, 0x7E, 0x07, 0x00, 0xA3, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
};
u8 mbrSt[68] = {
	0x00, 0x00, 0x00, 0x03, 0x18, 0x04, 0x06, 0x0F, 0xE0, 0x3B, 0x77, 0x08,
	0x00, 0x00, 0x89, 0x6F, 0x06, 0x00, 0x00, 0x02, 0xCE, 0x3C, 0x06, 0x0F,
	0xE0, 0xBE, 0x4D, 0x78, 0x06, 0x00, 0xB3, 0x05, 0x01, 0x00, 0x00, 0x02,
	0xDC, 0xBF, 0x01, 0x0F, 0xE0, 0xD5, 0x5B, 0x7E, 0x07, 0x00, 0xA5, 0x2D,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
};

// Volume Boot Records
u8 vbrMain[54] = {
	0xE9, 0x00, 0x00, 0x54, 0x57, 0x4C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x02, 0x20, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0xF8, 0x34, 0x00,
	0x20, 0x00, 0x10, 0x00, 0x77, 0x08, 0x00, 0x00, 0x89, 0x6F, 0x06, 0x00,
	0x00, 0x00, 0x29, 0x78, 0x56, 0x34, 0x12, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};
u8 vbrPhoto[54] = {
	0xE9, 0x00, 0x00, 0x54, 0x57, 0x4C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x02, 0x20, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0xF8, 0x09, 0x00,
	0x20, 0x00, 0x10, 0x00, 0x4D, 0x78, 0x06, 0x00, 0xB3, 0x05, 0x01, 0x00,
	0x01, 0x00, 0x29, 0x78, 0x56, 0x34, 0x12, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

static size_t i;

enum {
	READ_MBR,
	REPAIR_MBR,
	FORMAT_MAIN,
	FORMAT_PHOTO,
	BACK
};

static int _fsMenu(int cursor)
{
	//top screen
	clearScreen(cMAIN);

	printf("\n\x1B[40mTwlNandTool Ver0.0");
	printf("\n\nNAND repair tool by RMC/RVTR");
	printf("\n\nMode: FileSystem");

	//menu
	Menu* m = newMenu();
	setMenuHeader(m, "TwlNandTool");

	char modeStr[32];
	addMenuItem(m, "Read MBR", NULL, 0);
	addMenuItem(m, "Repair MBR", NULL, 0);
	addMenuItem(m, "Format TWL_MAIN", NULL, 0);
	addMenuItem(m, "Format TWL_PHOTO", NULL, 0);
	addMenuItem(m, "Back", NULL, 0);

	m->cursor = cursor;

	//bottom screen
	printMenu(m);

	while (!programEnd)
	{
		swiWaitForVBlank();
		scanKeys();

		if (moveCursor(m))
			printMenu(m);

		if (keysDown() & KEY_A)
			break;
	}

	int result = m->cursor;
	freeMenu(m);

	return result;
}

int fsMain(void)
{

	int cursor = 0;

	while (!programEnd)
	{
		cursor = _fsMenu(cursor);

		switch (cursor)
		{

			case READ_MBR:
				readMbr();
				break;

			case REPAIR_MBR:
				repairMbr();
				break;

			case FORMAT_MAIN:
				formatMain();
				break;

			case FORMAT_PHOTO:
				repairMbr();
				break;

			case BACK:
				programEnd = true;
				break;
		}
	}

	programEnd = false;

	return 0;
}

int readMbr(void) {
	clearScreen(cSUB);

	nand_ReadSectors(0, 1, sector_buf);
	dsi_crypt_init((const u8*)consoleIDfixed, (const u8*)0x2FFD7BC, is3DS);
	dsi_nand_crypt(sector_buf, sector_buf, 0, SECTOR_SIZE / AES_BLOCK_SIZE);

	iprintf("\n>> MBR (Master Boot Record)     ");
	iprintf("\n--------------------------------");	
    printf("\n     ");
    for (i = 444; i < SECTOR_SIZE; i++) {
        printf("%02X", sector_buf[i]);
        if ((i + 1) % 2 == 0) {
            printf(" ");
        }
        if ((i - 443) % 8 == 0 && i != 444) {
            printf("\n     ");
        }
    }
    if(parse_mbr(sector_buf, is3DS)) {
    	iprintf("\n\n    \x1B[31mERROR!\x1B[40m MBR is corrupted.");
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

int repairMbr(void) {
	clearScreen(cSUB);

	iprintf("\n>> Repair Corrupted MBR         ");
	iprintf("\n--------------------------------");	

    if(!parse_mbr(sector_buf, is3DS)) {
		if (choicePrint("NAND does not look corrupt.\nRepair anyway?") == NO) {
			return 0;
		}
	}

    iprintf("\nNAND type : %s (0x%02X)", nandInfo.NAND_PNM, nandInfo.NAND_MID);

    memset(sector_buf, 0, 444);
    if (nandInfo.NAND_MID == 0x15) {
		memcpy(sector_buf + 444, mbrSamsung, sizeof(mbrSamsung));
    } else if (nandInfo.NAND_MID == 0xFE) {
		memcpy(sector_buf + 444, mbrSt, sizeof(mbrSt));
    }

    // Write new MBR, encrypt it, then save it.
    // Afterwards we read it back from NAND and decrypt to confirm it works.
    //
    // No need to do safety checks before writing since corrupted MBR is already broken.
    iprintf("\nWriting new MBR...");

	dsi_nand_crypt(sector_buf, sector_buf, 0, SECTOR_SIZE / AES_BLOCK_SIZE);
	nand_WriteSectors(0, 1, sector_buf);

    iprintf("\nTesting new MBR...");

	nand_ReadSectors(0, 1, sector_buf);
	dsi_nand_crypt(sector_buf, sector_buf, 0, SECTOR_SIZE / AES_BLOCK_SIZE);

    if(parse_mbr(sector_buf, is3DS)) {
    	iprintf("\n\n    \x1B[31mERROR!\x1B[40m Failed to fix MBR.");
    }

    iprintf("\n\x1B[32mThe new MBR passed!\x1B[40m");

	iprintf("\n\n  Please Push Select To Return  ");

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_SELECT )
			break;
	}
}

int formatMain(void) {
	clearScreen(cSUB);

	iprintf("\n>> Write TWL_MAIN VBR           ");
	iprintf("\n--------------------------------");

	memset(file_buf, 0, 0x200);
	// Write the first 54 bytes, then pad to 0x1FE. Finally write 0x55AA
	memcpy(file_buf, vbrMain, sizeof(vbrMain));
    memset(file_buf + sizeof(vbrMain), 0, (BUFFER_SIZE - sizeof(vbrMain) - 2));
    memset(file_buf + SECTOR_SIZE - 2, 0x55, 1);
    memset(file_buf + SECTOR_SIZE - 1, 0xAA, 1);
	good_nandio_write(0x10EE00, 0x200, file_buf, false);

	iprintf("\n\nDone! Please confirm VBR is correct.");
	iprintf("\n\n  Please Push Select To Return  ");

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_SELECT )
			break;
	}	
}