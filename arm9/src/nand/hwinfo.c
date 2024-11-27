#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <nds/arm9/nand.h>
#include "f_xy.h"
#include "twltool/dsi.h"
#include "nandio.h"
#include "sector0.h"
#include "crypto.h"
#include "hwinfo.h"
#include "../message.h"
#include "../main.h"
#include "../video.h"
#include "../storage.h"

static size_t i;
bool hwinfofound = false;

hwsFormat hwsData = {0};

bool clearHWInfoStruct() {
    // Dummy values are JPN. This seems to be the usual default.
    for (int i = 0; i < 0x80; i++) {
        hwsData.HWS_SIG[i] = 0xFF;
    }
    hwsData.HWS_HEADER = 0x01000000;
    hwsData.HWS_SIZE = 0x1C000000;
    hwsData.HWS_LANG = (LANG_BITMAP_JAPAN << 24);
    hwsData.HWS_LANG &= 0xFF000000;
    //hwsData.HWS_LANG = 0x01000000;
    hwsData.HWS_PAD = 0x00000000;
    hwsData.HWS_REGION = LANG_JAPANESE;
    strcpy(hwsData.HWS_SERIAL, "AAAMP1234567"); // You're a real one if you understand this serial.
    hwsData.HWS_TID = 0x4A414E48;
    return true;
}

bool loadHWInfoStruct(u8 *hwinfo_buf) {
    // 0xA4 is the HWInfo size (trimmed)
    memcpy(&hwsData, hwinfo_buf, 0xA4);
}

bool recoverHWInfo(bool simple) {
    success = true;
    clearScreen(cSUB);

    iprintf("\n>> Recover HWInfo Secure        ");
    iprintf("\n--------------------------------");

    if (false){//nandMounted == true) {

        printf("\nChecking mounted TWL_MAIN...");
        if (false){//fileExists("nand:/sys/HWINFO_S.dat")) {
            printf("\nFound HWInfo.");
            if (copyFile("nand:/sys/HWINFO_S.dat", "sd:/HWINFO_S_BACKUP.dat") != 0) {
                success = false;
                printf("\nCouldn't copy HWInfo!");
            } else {
                printf("\nCopied okay.");
                // Read the file to file_buf
                // Some verification here
                // Dump the thing to the struct
                hwinfofound = true;
            }
        } else {
            printf("\nCouldn't find HWInfo!");
        }
    }

    if (simple == false) {
        if (!agingMode) {
            agingMode = true;
            if (recoverHWInfoOffset(HWS_OFFSET_OTHER)) {
                hwinfofound = true;
            } else if (recoverHWInfoOffset(HWS_OFFSET_BOX)) {
                hwinfofound = true;
            } else if (recoverHWInfoOffset(HWS_OFFSET_HANDHELD)) {
                hwinfofound = true;
            }
            agingMode = false;
        } else {
            if (recoverHWInfoOffset(HWS_OFFSET_OTHER)) {
                hwinfofound = true;
            } else if (recoverHWInfoOffset(HWS_OFFSET_BOX)) {
                hwinfofound = true;
            } else if (recoverHWInfoOffset(HWS_OFFSET_HANDHELD)) {
                hwinfofound = true;
            }
        }
    }

    clearScreen(cSUB);
    iprintf("\n>> Recover HWInfo Secure        ");
    iprintf("\n--------------------------------");

    if (hwinfofound == true) {
        iprintf("\nEverything was ok!");
    } else {
        iprintf("\nFailed to find HWInfo!");
    }
    exitFunction();
    return success;
}

/*

    Let's design recoverHWInfoOffset() around edge cases- meet the TWL-CPU-X4 (prototype).
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

    785E7A35 9A9B3C08 B9AAE1D5 02D5CD71 <-- No idea what this entire block is!
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
    002E2E02 2E2E022E 2E022E59 02595902 <-- Serial number (completely broken in copies)
    4A414E48 <-- Launcher TID

    785E7A35 9A9B3C08 B9AAE1D5 02D5CD71 <-- This weird block appears again!
    E7CFDC89 607EC36A 7A680E45 D0B30B50
    BBD36599 99731FE3 91F61DDB 8788C2C1
    50B19D58 ... and so on for 0x36C bytes.

    I have no idea what testing this DSi went through. I can't say if this is unique to the X4 or not.
    
    Anyways, the "weird block" is repeated between every HWInfo chunk. We can check for...
    - The common part of the launcher TID (0x414E48)
    - The first 13 bytes of the "weird block" data following the TID

    Then to quickly verify this is a HWInfo, check if the "weird block" is repeated 0x36C + size of HWInfo later.
    This will work on retail as well since the "weird block" here is 0xFF padding until 16kb, so it will always repeat.

    Never let down the prototype enjoyers, even if this makes the process way more annoying!

    Oh also here's another edge case!
    The factory HWInfo Secure created by PRE_IMPORT will be entirely 0xFF.
    It will only exist in units that did not leave the factory. This is less common, but there are hundreds of those DSis out there.
    If you find 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF then do not proceed to recoverHWInfoDeep().

*/
bool recoverHWInfoOffset(int address) {
    success = false;
    clearScreen(cSUB);
    iprintf("\n>> Recover HWInfo Secure Offset ");
    iprintf("\n--------------------------------");

    memset(sector_buf, 0, 512);
    good_nandio_read(address + 0x91, 32, file_buf, true);
    printf("\n     ");
    for (i = 0; i < 32; i++) {
        printf("%02X", file_buf[i]);
        if ((i + 1) % 2 == 0) {
            printf(" ");
        }
        if ((i - 31) % 8 == 0 && i != 32) {
            printf("\n     ");
        }
    }
    if (file_buf[16] == 0x41 && file_buf[17] == 0x4E && file_buf[18] == 0x48) {
        success = true;
        printf("\n\nLauncher TID found!");
        good_nandio_read(address + 0x400 + 0x91, 32, file_buf2, true);
        for (i = 19; i < 32; i++) {
            if (file_buf[i] != file_buf2[i]) {
                success = false;
                break;
            }
        }
        if (success == true) {
            iprintf("\nSecondary data matches!\nClearing buffer...");
            memset(sector_buf, 0, 0xA4);
            iprintf("\nLoading HWInfo...");
            good_nandio_read(address, 0xA4, file_buf, true);

            loadHWInfoStruct(file_buf);
            
            success = saveHWInfoSDMC();

            clearScreen(cSUB);
            iprintf("\n>> Recover HWInfo Secure Offset ");
            iprintf("\n--------------------------------");
            if (success == true) {
                iprintf("\nRecovery was ok!");
            } else {
                iprintf("\nFailed to back up HWInfo!");
            }
        }
        //    // Verify
        //    fopen fwrite fclose
    }
    exitFunction();
    return success;
}

// 0x10EE00 is the twl_main start
// 0xCDF1200 is the twl_main size

bool recoverHWInfoDeep(void) {
    success = false;
    clearScreen(cSUB);
    iprintf("\n>> Recover HWInfo Secure Deep   ");
    iprintf("\n--------------------------------");
    int j = 0;
    iprintf("\nSearching raw TWL_MAIN...\n\n ");
    for (i = 0; i < (0xCDF1200 / SECTOR_SIZE); i++) {
        good_nandio_read(0x10EE00 + (i * SECTOR_SIZE), SECTOR_SIZE, file_buf, true);

        if (j >= 20) {
            char currentPicto = downloadPlayLoading(69);
            printf("\b%c", currentPicto);
            j = 0;
        } else {
            j++;
        }

        if (file_buf[0 + 0xA1] == 0x41 && file_buf[1 + 0xA1] == 0x4E && file_buf[2 + 0xA1] == 0x48) {
            iprintf("\b\x1B[1A\nFound xANH");
            success = true;
            good_nandio_read(0x10EE00 + (i * SECTOR_SIZE), SECTOR_SIZE, file_buf, true);
            good_nandio_read(0x10EE00 + (i * SECTOR_SIZE) + 0x400, SECTOR_SIZE, file_buf2, true);
            for (int j = 0xA4; j < 0xB4; j++) {
                if (file_buf[j] != file_buf2[j]) {
                    success = false;
                    iprintf("\nExtra data didn't match.");
                }
            }
            iprintf("\nDone.");
            if (success == true) { // also if (verify)
                memset(file_buf, 0, SECTOR_SIZE);
                good_nandio_read(0x10EE00 + (i * SECTOR_SIZE), SECTOR_SIZE, file_buf, true);
                printf("\n%02X%02X%02X", file_buf[0 + 0xA1], file_buf[1 + 0xA1], file_buf[2 + 0xA1]);
                loadHWInfoStruct(file_buf);
                if (!agingMode) {
                    agingMode = true;
                    success = saveHWInfoSDMC();
                    agingMode = false;
                } else {
                    success = saveHWInfoSDMC();
                }
                clearScreen(cSUB);
                iprintf("\n>> Recover HWInfo Secure Deep   ");
                iprintf("\n--------------------------------");
                if (success == true) {
                    iprintf("\nRecovery was ok!");
                } else {
                    iprintf("\nFailed to back up HWInfo!");
                }
                exitFunction();
                return success;
            }
        }
    }
    exitFunction();
    return success;
}

bool saveHWInfoSDMC(void) {
    success = true;
    clearScreen(cSUB);

    iprintf("\n>> Save HWInfo Secure to SDMC   ");
    iprintf("\n--------------------------------");

    printf("\nClearing HWInfo buffer...");
    memset(file_buf, 0, 0x4000);



    // I need to do region checking (world/korea/china)
    // ^^^^ huh??? What for? Regions shouldn't be needed HWInfo R/W.
    printf("\nRemoving old HWInfo...");
    remove(HWS_PATH_SD);
    printf("\nWriting HWInfo...");

    FILE *file = fopen(HWS_PATH_SD, "wb");
    if(file) {
        fwrite(sector_buf, 1, 0xA4, file);
        fclose(file);
        iprintf("\nFile written.");
    } else {
        success = false;
        iprintf("\nFile failed to open!");
    }

    exitFunction();
    return success;
}