#pragma once

#include <stdint.h>
#include <nds/disc_io.h>
#include "sector0.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************ Constants / Defines *********************************/

#define CRYPT_BUF_LEN         64
#define NAND_DEVICENAME       (('N' << 24) | ('A' << 16) | ('N' << 8) | 'D')

extern const DISC_INTERFACE   io_dsi_nand;

/************************ Function Protoypes **********************************/

void nandio_set_fat_sig_fix(uint32_t offset);

void getCID(u8 *CID);
void getConsoleID(uint8_t *consoleID);
void nandGetInfo(void);

typedef struct {
    uint8_t NAND_MID;
    char NAND_MID_NAME[10];
    uint8_t NAND_OID[2];
    uint8_t NAND_PNM[7];
    uint8_t NAND_PRV;
    uint8_t NAND_PSN[4];
    uint8_t NAND_MDT;
    uint8_t NAND_MDT_MONTH;
    uint8_t NAND_MDT_YEAR;
} nandData;

extern u8 *sector_buf;
extern u8 *file_buf;
extern bool is3DS;

extern nandData nandInfo;

extern u8 consoleID[8];
extern u8 CID[16];
extern u8 consoleIDfixed[8];

extern bool good_nandio_write(int inputAddress, int inputLength, u8 *buffer, bool crypt);

extern bool nandio_startup();
extern bool nandio_shutdown();

extern bool nandio_lock_writing();
extern bool nandio_unlock_writing();
extern bool nandio_force_fat_fix();

#ifdef __cplusplus
}
#endif
