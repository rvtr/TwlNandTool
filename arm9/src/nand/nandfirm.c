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
#include "../message.h"
#include "../main.h"
#include "../video.h"

void death(char *message, u8 *buffer){
	iprintf("\n%s\n", message);
	free(buffer);
	while(1)swiWaitForVBlank();
}

static size_t i;

enum {
	NFMENU_CHECK_VER,
	NFMENU_IMPORT,
	NFMENU_IMPORT_SDMC,
	NFMENU_READ_CID,
	NFMENU_READ_CONSOLEID,
	NFMENU_BACK
};

static int _nfMenu(int cursor)
{

    Menu* m = newMenu();
    setMenuHeader(m, "TwlNandTool");
    setListHeader(m, "NandFirm");

	addMenuItem(m, "Check NandFirm", NULL, 0, "Check the stage2 (bootloader)\n version and type.");
	addMenuItem(m, "Import NandFirm", NULL, 0, "Install the standard stage2\n (bootloader).\n\n This stage2 works normally,\n but it is an updated version:\n - v2265-9336 (prod)\n - v2725-9336 (dev)");
	addMenuItem(m, "Import NandFirm (SDMC)", NULL, 0, "Install the SDMC Launcher\n stage2 (bootloader).\n\n SDMC will remove access to\n the firmware and SHOULD NOT\n BE USED unless otherwise\n told to do so.");
	addMenuItem(m, "CID Info", NULL, 0, "Get NAND chip information.");
	addMenuItem(m, "ConsoleID Info", NULL, 0, "Get CPU information.");
	addMenuItem(m, "Back", NULL, 0, "Leave the NandFirm menu.");

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
	}

	int result = m->cursor;
	freeMenu(m);

	return result;
}

int nfMain(void)
{

	int cursor = 0;

	while (!programEnd)
	{
		cursor = _nfMenu(cursor);

		switch (cursor)
		{

			case NFMENU_CHECK_VER:
				nandFirmRead();
				break;

			case NFMENU_IMPORT:
				nandFirmImport(false);
				break;

			case NFMENU_IMPORT_SDMC:
				nandFirmImport(true);
				break;

			case NFMENU_READ_CID:
				nandPrintInfo();
				break;

			case NFMENU_READ_CONSOLEID:
				cpuPrintInfo();
				break;

			case NFMENU_BACK:
				programEnd = true;
				break;
		}
	}

	programEnd = false;

	return 0;
}

bool nandFirmRead(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> NandFirm Version Checker     ");
	iprintf("\n--------------------------------");
		
	nand_ReadSectors(626, 1, sector_buf);
	
	printf("\n        Ver: ");
    for (i = 0; i < 10; i++) {

        if (sector_buf[i] == 0x0A) {
            printf("-");
        } else if (sector_buf[i] == 0x0D) {
        	// Print nothing
        } else {
            printf("%c", sector_buf[i]);
        }
    }
    printf("\n");

	exitFunction();
	return success;
}

bool nandFirmImport(bool sdmc) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Import NandFirm (%s)       ", sdmc ? "sdmc" : "menu");
	iprintf("\n--------------------------------");

    printf("\nOpening NandFirm...\n");

    char file_path[100];
    snprintf(file_path, 100, "nitro:/import/%s/%s-launcher.nand", consoleSignName, sdmc ? "sdmc" : "menu");
    FILE *file = fopen(file_path, "r");

    printf("\n%s\n\n", file_path);

    if(file) {
    	// First 0x200 of NandFirm is reserved for MBR
        fseek(file, 0, SEEK_END);
        int file_length = ftell(file);
        fseek(file, 0x200, SEEK_SET);
    	printf("Importing...\n");
        good_nandio_write_file(0x200, file_length - ftell(file), file, false);
        printf("Done!\n");
        fclose(file);
    } else {
    	success = false;
		iprintf("\nNandFirm import failed!");
    }

    exitFunction();
    return success;
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
	if (nandInfo.NAND_MDT_YEAR <= 9) {
		iprintf("\nManufacturing date : %02X(%d 200%d)",nandInfo.NAND_MDT, nandInfo.NAND_MDT_MONTH, nandInfo.NAND_MDT_YEAR);
	} else {
		iprintf("\nManufacturing date : %02X(%d 20%d)",nandInfo.NAND_MDT, nandInfo.NAND_MDT_MONTH, nandInfo.NAND_MDT_YEAR);
	}
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
    iprintf("\n\n     ");
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

	exitFunction();
	return success;
}