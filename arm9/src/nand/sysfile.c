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
#include "sysfile.h"
#include "hwinfo.h"
#include "sector0.h"
#include "crypto.h"
#include "../message.h"
#include "../main.h"
#include "../video.h"
#include "../storage.h"
#include "../audio.h"

//static size_t i;

enum {
	SYSFILEMENU_RECOVER,
	SYSFILEMENU_RECOVER2,
	SYSFILEMENU_NULL,
	SYSFILEMENU_INIT_FOLDER,
	SYSFILEMENU_INIT_N,
	SYSFILEMENU_INIT_S,
	SYSFILEMENU_INIT_CERT,
	SYSFILEMENU_INIT_FONT
};

static int _sysfileMenu(int cursor)
{

    Menu* m = newMenu();
    setMenuHeader(m, "TwlNandTool");
    setListHeader(m, "Sys File");

	addMenuItem(m, "Find HWINFO_S.dat", NULL, 0, "Search for HWInfo in TWL_MAIN\n then in common offsets.\n\n In most cases this will be\n enough to recover HWInfo.");
	addMenuItem(m, "Find HWINFO_S.dat (deep)", NULL, 0, "Do a byte by byte search for\n HWInfo in the NAND.\n\n This is rarely required and\n should be thought of as a last\n resort only.");
	addMenuItem(m, "------------------------", NULL, 0, "");
	addMenuItem(m, "Write NAND folders", NULL, 0, "Create folders that store\n system files.");
	addMenuItem(m, "Write HWINFO_N.dat", NULL, 0, "Create HWINFO_N.dat.\n\n This file is required to boot.");
	addMenuItem(m, "Write HWINFO_S.dat", NULL, 0, "Recover and make HWINFO_S.dat.\n\n If HWINFO_S.dat cannot be\n recovered, it cannot be\n recreated and unlaunch will be\n required to boot the launcher.");
	addMenuItem(m, "Write cert.sys", NULL, 0, "Create the certificate chain.");
	addMenuItem(m, "Write TWLFontTable", NULL, 0, "Create the font data.");

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

int sysfileMain(void)
{

	int cursor = 0;

	while (!programEnd)
	{
		cursor = _sysfileMenu(cursor);

		if (programEnd == false) {
			switch (cursor)
			{

				case SYSFILEMENU_RECOVER:
					soundPlaySelect();
					recoverHWInfo(false);
					break;

				case SYSFILEMENU_RECOVER2:
					soundPlaySelect();
					recoverHWInfoDeep();
					break;

				case SYSFILEMENU_NULL:
					soundPlaySelect();
					break;

				case SYSFILEMENU_INIT_FOLDER:
					soundPlaySelect();
					makeSystemFolders();
					break;

				case SYSFILEMENU_INIT_S:
					soundPlaySelect();
					break;

				case SYSFILEMENU_INIT_N:
					soundPlaySelect();
					break;

				case SYSFILEMENU_INIT_CERT:
					soundPlaySelect();
					makeCertChain();
					break;

				case SYSFILEMENU_INIT_FONT:
					soundPlaySelect();
					makeFontTable();
					break;
			}
		}
	}

	programEnd = false;

	return 0;
}

bool makeSystemFolders(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Make NAND folders            ");
	iprintf("\n--------------------------------");

	if (nandMounted == true) {
	    printf("\nMaking /sys/...");
		mkdir("nand:/sys", 0777);
	    printf("\nMaking /sys/log/...");
		mkdir("nand:/sys/log", 0777);
	    printf("\nMaking /shared1/...");
		mkdir("nand:/shared1", 0777);
	    printf("\nMaking /shared2/...");
		mkdir("nand:/shared2", 0777);
	    printf("\nMaking /ticket/...");
		mkdir("nand:/ticket", 0777);
	    printf("\nMaking /title/...");
		mkdir("nand:/title", 0777);
	    printf("\nMaking /tmp/...");
		mkdir("nand:/tmp", 0777);
	    printf("\nDone!");
	} else {
		success = false;
		iprintf("\nTWL_MAIN is not mounted!");
	}

	exitFunction();
	return success;
}

bool makeCertChain(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Make cert.sys                ");
	iprintf("\n--------------------------------");

	if (nandMounted == true) {

	    printf("\nRemoving old cert.sys...");
	    char nitro_path[100];
	    snprintf(nitro_path, 100, "nitro:/import/%s/cert.sys", consoleSignName);
	    remove(CERT_PATH);
	    printf("\nWriting cert.sys...");
	    if (copyFile(nitro_path, CERT_PATH) != 0) {
	    	success = false;
	    } else {
	    	printf("\nDone!");
	    }
	} else {
		success = false;
		iprintf("\nTWL_MAIN is not mounted!");
	}

	exitFunction();
	return success;
}

bool makeFontTable(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Make TWLFontTable            ");
	iprintf("\n--------------------------------");

	if (nandMounted == true) {

		// I need to do region checking (world/korea/china)
	    printf("\nRemoving old TWLFontTable...");
	    char nitro_path[100];
	    snprintf(nitro_path, 100, "nitro:/import/common/TWLFontTable.dat");
	    remove(FONT_PATH);
	    printf("\nWriting TWLFontTable (world)...");
	    if (copyFile(nitro_path, FONT_PATH) != 0) {
	    	success = false;
	    } else {
	    	printf("\nDone!");
	    }
	} else {
		success = false;
		iprintf("\nTWL_MAIN is not mounted!");
	}

	exitFunction();
	return success;
}