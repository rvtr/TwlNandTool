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
#include "sector0.h"
#include "crypto.h"
#include "../message.h"
#include "../main.h"
#include "../video.h"

static size_t i;

enum {
	SYSFILEMENU_RECOVER,
	SYSFILEMENU_RECOVER2,
	SYSFILEMENU_NULL,
	SYSFILEMENU_INIT_N,
	SYSFILEMENU_INIT_S,
	SYSFILEMENU_INIT_CERT,
	SYSFILEMENU_INIT_FONT
};

/*

	I just need to plan things out:
	- verifyHWInfo
		- Just test input HWInfo buffer
	- recoverHWInfoShallow
		- Check for easy to find file (in mounted TWL_MAIN, then by common HWInfo offsets)
	- recoverHWInfoDeep
		- Scrap the entire TWL_MAIN byte by byte to find HWInfo (last resort)
	- resignHWInfo

	Let's design this around edge cases- meet the TWL-CPU-X4 (prototype).
	The HWInfo is made up of one "real" HWInfo at the start, then a repeating secondary HWInfo to pad out to 16kb.

	40E34ABB 0E81922C 0B2B24E5 CD4864D4 \
	637F0199 D385B1AA A893F65E EDADE869  |
	DA66D05D 5B31A897 E4AAF339 83FDD161  | "Real" HWInfo signature
	128804B7 B7BA3C06 CD5304ED 9642181C  |
	A57B3B8B 695E1B37 6F7F220B 019C3932  |
	8D45C9AE 99550547 71E77ECD 0C442E98  |
	E2ECB154 BB4D7305 AF75D324 7231B36A  |
	A53677B1 EB4D0102 C8F31A43 EED93758 /
	01000000 1C000000 01000000 00000000 <-- Region and language info
	00414141 50503241 47303538 31000000 <-- Serial number
	4A414E48 <-- Launcher TID

	785E7A35 9A9B3C08 B9AAE1D5 02D5CD71	<-- No idea what this entire block is!
	E7CFDC89 607EC36A 7A680E45 D0B30B50
	BBD36599 99731FE3 91F61DDB 8788C2C1
	50B19D58 ... and so on for 0x36C bytes.

	64E1D65C 2649BBAA EDF4808A 3B5830A0 \
	2E0C2E9A 481A0487 E9DC32DB A49BDA8C  |
	901B5647 2E3473CB 7317122A 2F7ECCE3  | "Real" HWInfo signature
	C880187C 0EA2C230 DFFED67A D6AC7C54  |
	98676C25 4B1726FF 3EAA6EE4 39A718CA  |
	EA2ECF98 9B41BA5F 3E32A49F 8262DE7E  |
	201585B4 E4D27B32 B4D501EE 2B098032 /
	01000000 1C000000 01000000 00000000 <-- Region and language info
	002E2E02 2E2E022E 2E022E59 02595902 <-- Serial number (completely broken)
	4A414E48 <-- Launcher TID

	785E7A35 9A9B3C08 B9AAE1D5 02D5CD71	<-- This weird block appears again!
	E7CFDC89 607EC36A 7A680E45 D0B30B50
	BBD36599 99731FE3 91F61DDB 8788C2C1
	50B19D58 ... and so on for 0x36C bytes.

	I have no idea what testing this DSi went through. I can't say if this is unique to the X4 or not.
	
	Anyways, the "weird block" is repeated between every HWInfo chunk. We can check for...
	- 0x414E48: the common part of the launcher TID
	- 0x785E7A35 9A9B3C08 B9AAE1D5 02D5CD71: the "weird block" data after the TID

	Then to verify this is a HWInfo, check if the "weird block" is repeated 0x36C + size of HWInfo later.
	This will work on retail as well since the "weird block" here is 0xFF padding until 16kb, so it will always repeat.

	Never let down the prototype enjoyers, even if this makes the process way more annoying!

	Oh also here's another edge case!
	The factory HWInfo Secure created by PRE_IMPORT will be entirely 0xFF.
	It will only exist in uninitialized units (did not leave the factory).
	If you find 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF then do not proceed to recoverHWInfoDeep.

*/

static int _sysfileMenu(int cursor)
{

    Menu* m = newMenu();
    setMenuHeader(m, "TwlNandTool");
    setListHeader(m, "Sys File");

	addMenuItem(m, "Find HWINFO_S.dat", NULL, 0, "Search for HWInfo in TWL_MAIN\n then in common offsets.\n\n In most cases this will be\n enough to recover HWInfo.");
	addMenuItem(m, "Find HWINFO_S.dat (deep)", NULL, 0, "Do a byte by byte search for\n HWInfo in the NAND.\n\n This is rarely required and\n should be thought of as a last\n resort only.");
	addMenuItem(m, "------------------------", NULL, 0, "Leave the NandFirm menu.");
	addMenuItem(m, "Init HWINFO_N.dat", NULL, 0, "Create HWINFO_N.dat.\n\n This file is required to boot.");
	addMenuItem(m, "Init HWINFO_S.dat", NULL, 0, "Recover and make HWINFO_S.dat.\n\n If HWINFO_S.dat cannot be\n recovered, it cannot be\n recreated and unlaunch will be\n required to boot the launcher.");
	addMenuItem(m, "Init cert.sys", NULL, 0, "Create the certificate chain.");
	addMenuItem(m, "Init TWLFontTable", NULL, 0, "Create the font data.");

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
					break;

				case SYSFILEMENU_RECOVER2:
					break;

				case SYSFILEMENU_NULL:
					break;

				case SYSFILEMENU_INIT_S:
					break;

				case SYSFILEMENU_INIT_N:
					break;

				case SYSFILEMENU_INIT_CERT:
					break;

				case SYSFILEMENU_INIT_FONT:
					break;
			}
		}
	}

	programEnd = false;

	return 0;
}
