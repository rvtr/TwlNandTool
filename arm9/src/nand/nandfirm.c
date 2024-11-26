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
#include "chipinfo.h"
#include "sector0.h"
#include "crypto.h"
#include "../message.h"
#include "../main.h"
#include "../video.h"

static size_t i;

enum {
	NFMENU_CHECK_VER,
	NFMENU_IMPORT,
	NFMENU_IMPORT_SDMC
};

static int _nfMenu(int cursor)
{

    Menu* m = newMenu();
    setMenuHeader(m, "TwlNandTool");
    setListHeader(m, "NandFirm");

	addMenuItem(m, "Check NandFirm", NULL, 0, "Check the stage2 (bootloader)\n version and type.");
	addMenuItem(m, "Import NandFirm", NULL, 0, "Install the standard stage2\n (bootloader).\n\n This stage2 works normally,\n but it is an updated version:\n - v2265-9336 (prod)\n - v2725-9336 (dev)");
	addMenuItem(m, "Import NandFirm (SDMC)", NULL, 0, "Install the SDMC Launcher\n stage2 (bootloader).\n\n SDMC will remove access to\n the firmware and SHOULD NOT\n BE USED unless otherwise\n told to do so.");

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

		if (keysDown() & KEY_B)
			programEnd = true;
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

		if (programEnd == false) {
			switch (cursor)
			{

				case NFMENU_CHECK_VER:
					readNandFirm();
					break;

				case NFMENU_IMPORT:
					importNandFirm(false);
					break;

				case NFMENU_IMPORT_SDMC:
					importNandFirm(true);
					break;
			}
		}
	}

	programEnd = false;

	return 0;
}

bool readNandFirm(void) {
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

bool importNandFirm(bool sdmc) {
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