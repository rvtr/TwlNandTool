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
size_t i;

void death(char *message, u8 *buffer){
	iprintf("\n%s\n", message);
	free(buffer);
	while(1)swiWaitForVBlank();
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


/*

	I'd actually like to make a good RAW NAND R/W routine.
	nandRead(byteAddress, amount, infile)


	I want to find the sector (626) and sector offset (80) for the hex address below:

		Offset:      320592
		Sector size: 512


	byteOffset = byteAddress % sectorSize and sector = byteAddress / sectorSize

*/