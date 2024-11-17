#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <nds/arm9/nand.h>
#include <nds/disc_io.h>
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
#include "../lib/libfat/include/fat.h"

extern bool nand_Startup();

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
	FSMENU_READ_MBR,
	FSMENU_REPAIR_MBR,
	FSMENU_FORMAT_MAIN,
	FSMENU_FORMAT_PHOTO,
  	FSMENU_NULL,
	FSMENU_MOUNT_MAIN,
	FSMENU_UNMOUNT_MAIN,
	FSMENU_MOUNT_NITRO,
	FSMENU_NULL2,
	FSMENU_FILETEST_MAIN,
	FSMENU_FILETEST_NITRO
};

static int _fsMenu(int cursor)
{

    Menu* m = newMenu();
    setMenuHeader(m, "TwlNandTool");
    setListHeader(m, "FileSystem");

	addMenuItem(m, "Read MBR", NULL, 0, "Test the Master Boot Record.");
	addMenuItem(m, "Repair MBR", NULL, 0, "Repair the Master Boot Record.");
	addMenuItem(m, "Format TWL_MAIN", NULL, 0, "Format the partition where the\n firmware, apps, and saves are\n installed.\n\n THIS WILL ERASE EVERYTHING.");
	addMenuItem(m, "Format TWL_PHOTO", NULL, 0, "Format the partition where\n photos are stored.\n\n THIS WILL ERASE EVERYTHING.");
	addMenuItem(m, "------------------", NULL, 0, "");
	addMenuItem(m, "Mount TWL_MAIN", NULL, 0, "Mount the TWL_MAIN partition.");
	addMenuItem(m, "Unmount TWL_MAIN", NULL, 0, "Unmount the TWL_MAIN partition");
	addMenuItem(m, "Mount NitroFS", NULL, 0, "Mount the ROM filesystem.");
	addMenuItem(m, "------------------", NULL, 0, "");
	addMenuItem(m, "File test TWL_MAIN", NULL, 0, "Attempt to make/delete dummy\n file on NAND.");
	addMenuItem(m, "File test NitroFS", NULL, 0, "Attempt to read file in\n NitroFS.");

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

int fsMain(void)
{

	int cursor = 0;

	while (!programEnd)
	{
		cursor = _fsMenu(cursor);

		if (programEnd == false) {
			switch (cursor)
			{

				case FSMENU_READ_MBR:
					readMbr();
					break;

				case FSMENU_REPAIR_MBR:
					repairMbr(false);
					break;

				case FSMENU_FORMAT_MAIN:
					formatMain();
					break;

				case FSMENU_FORMAT_PHOTO:
					formatPhoto();
					break;

				case FSMENU_NULL:
					break;

				case FSMENU_MOUNT_MAIN:
					mountMain();
					break;

				case FSMENU_UNMOUNT_MAIN:
					unmountMain();
					break;

				case FSMENU_MOUNT_NITRO:
					mountNitroFS();
					break;

				case FSMENU_NULL2:
					break;

				case FSMENU_FILETEST_MAIN:
					filetestMain();
					break;

				case FSMENU_FILETEST_NITRO:
					filetestNitro();
					break;
			}
		}
	}

	programEnd = false;

	return 0;
}

bool readMbr(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> MBR (Master Boot Record)     ");
	iprintf("\n--------------------------------");

    if(nand_ReadSectors(0, 1, sector_buf) == false) {
    	iprintf("\nCouldn't read NAND!\n");
    	success = false;
    }

    dsi_nand_crypt(sector_buf, sector_buf, 0, SECTOR_SIZE / AES_BLOCK_SIZE);

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
    	iprintf("\n\n    \x1B[31mERROR!\x1B[30m MBR is corrupted.");
    	success = false;
    }

	exitFunction();
	return success;
}

bool repairMbr(bool autofix) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Repair Corrupted MBR         ");
	iprintf("\n--------------------------------");	

	if (parse_mbr(sector_buf, is3DS) || nandMounted == false) {
	    if(!parse_mbr(sector_buf, is3DS) && autofix == false) {
			if (choicePrint("NAND does not look corrupt.\nRepair anyway?") == NO) {
				exitFunction();
				return success;
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
	    	iprintf("\n\n    \x1B[31mERROR!\x1B[30m Failed to fix MBR.");
	    	success = false;
	    } else {
	    	iprintf("\n\x1B[32mThe new MBR passed!\x1B[30m");
    	}
    } else {
		iprintf("\nNAND is currently mounted!\n\nSkipping MBR repair since\nthe FS had no issues.");
    }

	exitFunction();
	return success;
}

bool formatMain(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Format TWL_MAIN              ");
	iprintf("\n--------------------------------");

	if (nandMounted == false) {
		iprintf("\nWriting VBR...");
		memset(file_buf, 0, 0x200);
		// Write the first 54 bytes, then pad to 0x1FE. Finally write 0x55AA
		memcpy(file_buf, vbrMain, sizeof(vbrMain));
	    memset(file_buf + sizeof(vbrMain), 0, (BUFFER_SIZE - sizeof(vbrMain) - 2));
	    memset(file_buf + SECTOR_SIZE - 2, 0x55, 1);
	    memset(file_buf + SECTOR_SIZE - 1, 0xAA, 1);
		good_nandio_write(0x10EE00, 0x200, file_buf, true);
		iprintf("\nClearing file tables...");
		memset(file_buf, 0, 0x600);
		for (i = 0; i < 0x3d8; i++) {
			good_nandio_write(0x10EE00 + 0x200 + (0x600 * i), 0x600, file_buf, true);
		}
		iprintf("\nAll done!");
	} else {
		iprintf("\nNAND is currently mounted!\n\nSkipping TWL_MAIN format since\nthe FS had no issues.");
	}

	exitFunction();
	return success;
}

bool formatPhoto(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Format TWL_PHOTO             ");
	iprintf("\n--------------------------------");

	iprintf("\nWriting VBR...");
	memset(file_buf, 0, 0x200);
	// Write the first 54 bytes, then pad to 0x1FE. Finally write 0x55AA
	memcpy(file_buf, vbrPhoto, sizeof(vbrPhoto));
    memset(file_buf + sizeof(vbrPhoto), 0, (BUFFER_SIZE - sizeof(vbrPhoto) - 2));
    memset(file_buf + SECTOR_SIZE - 2, 0x55, 1);
    memset(file_buf + SECTOR_SIZE - 1, 0xAA, 1);
	good_nandio_write(0xCF09A00, 0x200, file_buf, true);
	iprintf("\nClearing file tables...");
	memset(file_buf, 0, 0x200);
	for (i = 0; i < 0x13A; i++) {
		good_nandio_write(0xCF09A00 + 0x200 + (0x200 * i), 0x200, file_buf, true);
	}

	iprintf("\nAll done!");

	exitFunction();
	return success;
}

bool mountMain(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Mount TWL_MAIN               ");
	iprintf("\n--------------------------------");

	if (nandMounted == true) {
		if (!agingMode) {
			agingMode = true;
			unmountMain();
			agingMode = false;
		} else {
			unmountMain();
		}
	}

	clearScreen(cSUB);
	iprintf("\n>> Mount TWL_MAIN               ");
	iprintf("\n--------------------------------");

	if (!nandio_startup()) {
		iprintf("\nFailed startup!");
		success = false;
	}
	if (success == true && !fatMountSimple("nand", &io_dsi_nand, true)) {
		iprintf("\n\x1B[31mMount TWL_MAIN failed!\n\x1B[30m\nNAND must be repaired. Try...\n - Fixing MBR\n - Formatting TWL_MAIN");
		success = false;
	} else if (success == true) {
		iprintf("\nTWL_MAIN mounted okay.");
		nandio_unlock_writing();
		nandMounted = true;
	}

	exitFunction();
	return success;
}

bool unmountMain(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Unmount TWL_MAIN             ");
	iprintf("\n--------------------------------");

	if (nandMounted == true) {
		iprintf("\nUnmounting NAND...");
		fatUnmount("nand");
		nandMounted = false;
		sdMounted = false;

		// I just don't want to interrupt the startup aging test
		if (!agingMode) {
			agingMode = true;
			success = mountNitroFS();
			agingMode = false;
		} else {
			success = mountNitroFS();
		}

		clearScreen(cSUB);
		iprintf("\n>> Umount TWL_MAIN              ");
		iprintf("\n--------------------------------");

		if (success) {
			iprintf("\nTWL_MAIN unmounted okay.");
		} else {
			iprintf("\nFailed to mount TWL_MAIN!");
		}

	} else {
		iprintf("\nNAND not mounted!\nDoing nothing.");
	}

	exitFunction();
	return success;
}

bool mountNitroFS(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Mount NitroFS                ");
	iprintf("\n--------------------------------");

	if (sdMounted == false && !fatInitDefault()) {
		iprintf("\nSD card not mounted!\n");
		success = false;
	} else {
		iprintf("\nSD card mounted okay.\n");
		sdMounted = true;
	}

	if(!nitroFSInit("TwlNandTool.prod.srl") || !nitroFSGood()) {
		if(!nitroFSInit("TwlNandTool.dev.srl") || !nitroFSGood()) {
			if(!nitroFSInit("ntrboot.nds") || !nitroFSGood()) {
				messageBox("\n\x1B[31mFailed to mount NitroFS!\n\x1B[30m\nSome features will not work.\n\nTry placing the SRL for your DSiat one of these locations:\n\nSDMC:/TwlNandTool.prod.srl\nSDMC:/TwlNandTool.dev.srl\nSDMC:/ntrboot.nds\n");
				success = false;
			}
		}
	}

	if (success) {
		iprintf("\nNitroFS mounted okay.");
	}

	exitFunction();
	return success;
}

bool filetestMain(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Read TWL_MAIN file           ");
	iprintf("\n--------------------------------");

	if (nandMounted == true) {
	    printf("\nMaking temp folder...");
		mkdir("nand:/tmp", 0777);

	    printf("\nOpening test file...\n");

	    char file_path[100];
	    snprintf(file_path, 100, "nand:/tmp/test.bin");
	    FILE *file = fopen(file_path, "wb");
	    printf("\n%s\n", file_path);
	    if(file) {
		    iprintf("\nWrite test...");
		    memset(sector_buf, 0, SECTOR_SIZE);
			memcpy(sector_buf + 444, mbrSamsung, sizeof(mbrSamsung));
			fwrite(sector_buf, 1, SECTOR_SIZE, file);
	        fclose(file);
		    iprintf("\nFile closed.");
	    } else {
	    	success = false;
			iprintf("\nFile failed to open!");
	    }

	    memset(sector_buf, 0, SECTOR_SIZE);

		iprintf("\nOpening test file...");
	    FILE *file2 = fopen(file_path, "rb");
	    if(file2) {
		    iprintf("\nRead test...");
	        fread(sector_buf, 1, SECTOR_SIZE, file2);
		    if(parse_mbr(sector_buf, is3DS)) {
		    	iprintf("\n\n    \x1B[31mERROR!\x1B[30m File did not save!");
		    	success = false;
		    } else {
		    	iprintf("\n\x1B[32mThe file is okay!\x1B[30m");
	    	}
	        fclose(file2);
		    iprintf("\nFile closed.");
	    } else {
	    	success = false;
			iprintf("\nFile failed to open!");
	    }

	    remove(file_path);
	} else {
		success = false;
		iprintf("\nTWL_MAIN is not mounted!");
	}

	exitFunction();
	return success;
}

bool filetestNitro(void) {
	success = true;
	clearScreen(cSUB);

	iprintf("\n>> Read NitroFS file            ");
	iprintf("\n--------------------------------");

    printf("\nOpening test file...\n");

    char file_path[100];
    snprintf(file_path, 100, "nitro:/version_twlnandtool");
    FILE *file = fopen(file_path, "r");
    printf("\n%s\n", file_path);
    if(file) {
        fseek(file, 0, SEEK_END);
        int length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *version = (char *)malloc((length) * sizeof(char));
        version[length] = '\0'; 

        fread(version, 1, length, file);

        printf("\n%s\n", version);
        fclose(file);

		iprintf("\nFile opened okay.");
    } else {
    	success = false;
		iprintf("\nFile failed to open!");
    }
	exitFunction();
	return success;
}