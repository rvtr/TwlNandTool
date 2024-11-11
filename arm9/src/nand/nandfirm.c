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

u32 done=0;

void death(char *message, u8 *buffer){
	iprintf("\n%s\n", message);
	free(buffer);
	while(1)swiWaitForVBlank();
}

static size_t i;

enum {
	MENUSTATE_CHECK_NF_VER,
	MENUSTATE_IMPORT_NF,
	MENUSTATE_IMPORT_NF_SDMC,
	MENUSTATE_READ_CID,
	BACK
};

static int _nfMenu(int cursor)
{
	//top screen
	clearScreen(cMAIN);

	printf("\n\x1B[40mTwlNandTool Ver0.0");
	printf("\n\nNAND repair tool by RMC/RVTR");
	printf("\n\nMode: NandFirm");

	//menu
	Menu* m = newMenu();
	setMenuHeader(m, "TwlNandTool");

	char modeStr[32];
	addMenuItem(m, "Check NandFirm", NULL, 0);
	addMenuItem(m, "Import NandFirm", NULL, 0);
	addMenuItem(m, "Import NandFirm (SDMC)", NULL, 0);
	addMenuItem(m, "CID Info", NULL, 0);
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

int nfMain(void)
{

	int cursor = 0;

	while (!programEnd)
	{
		cursor = _nfMenu(cursor);

		switch (cursor)
		{

			case MENUSTATE_CHECK_NF_VER:
				nandFirmRead();
				break;

			case MENUSTATE_IMPORT_NF:
				nandFirmImport(false);
				break;

			case MENUSTATE_IMPORT_NF_SDMC:
				nandFirmImport(true);
				break;

			case MENUSTATE_READ_CID:
				nandPrintInfo();
				break;

			case BACK:
				programEnd = true;
				break;
		}
	}

	programEnd = false;

	return 0;
}

int nandFirmRead(void) {

	clearScreen(cSUB);

	iprintf("\n>> NandFirm Version Checker     ");
	iprintf("\n--------------------------------");

	int fail=0;
		
	// Sectors are 0x200 each, this is sector 626 OR offset 0x4E400
	// nand_ReadSectors(<start at sector>, <number of sectors to read>, <save sectors to>)
	if(nand_ReadSectors(626, 1, sector_buf) == false){
		iprintf("Nand read error");
	}
	
	printf("\n        Ver: ");
    for (i = 0; i < 9; i++) {

        if (sector_buf[i] == 0x0A) {
            printf("-");
        } else {
            printf("%c", sector_buf[i]);
        }
    }
    printf("\n");

	iprintf("\n\n\n  Please Push Select To Return  ");


	while (true)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_SELECT )
			break;
	}
}

int nandFirmImport(bool sdmc) {

	clearScreen(cSUB);

	iprintf("\n>> Debug1");
	iprintf("\n NandFirm write                 ");
	iprintf("\n--------------------------------");

    printf("Opening NandFirm...\n");

    char file_path[100];
    snprintf(file_path, 100, "nitro:/import/%s/%s-launcher.nand", consoleSignName, sdmc ? "sdmc" : "menu");
    FILE *file = fopen(file_path, "r");

    printf("%s\n", file_path);

    if(file) {
    	// First 0x200 of NandFirm is reserved for MBR
        fseek(file, 0, SEEK_END);
        int file_length = ftell(file);
        fseek(file, 0x200, SEEK_SET);
    	printf("Importing...\n");
        good_nandio_write_file(0x200, file_length - ftell(file), file, false);
        printf("Done!\n");
        fclose(file);
    }

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_SELECT )
			break;
	}
}

int nandPrintInfo(void) {

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
	iprintf("\nManufacturing date : %02X(%d 20%d)",nandInfo.NAND_MDT, nandInfo.NAND_MDT_MONTH, nandInfo.NAND_MDT_YEAR);
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
	iprintf("\n\n\n  Please Push Select To Return  ");

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_SELECT )
			break;
	}
}