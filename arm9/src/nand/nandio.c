
#include <nds.h>
#include <nds/disc_io.h>
#include <malloc.h>
#include <stdio.h>
#include "crypto.h"
#include "sector0.h"
#include "f_xy.h"
#include "nandio.h"
#include "../menu.h"
#include "u128_math.h"

/************************ Function Protoypes **********************************/

bool nandio_startup();
bool nandio_is_inserted();
bool nandio_read_sectors(sec_t offset, sec_t len, void *buffer);
bool nandio_write_sectors(sec_t offset, sec_t len, const void *buffer);
bool nandio_clear_status();
bool nandio_shutdown();

/************************ Constants / Defines *********************************/

u8 consoleID[8];
u8 CID[16];
u8 consoleIDfixed[8];

nandData nandInfo = {0};

const DISC_INTERFACE io_dsi_nand = {
	NAND_DEVICENAME,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
	nandio_startup,
	nandio_is_inserted,
	nandio_read_sectors,
	nandio_write_sectors,
	nandio_clear_status,
	nandio_shutdown
};

bool is3DS;

static bool writingLocked = true;
static bool nandWritten = false;

extern bool nand_Startup();

static u8* crypt_buf = 0;

static u32 fat_sig_fix_offset = 0;

static u32 sector_buf32[SECTOR_SIZE/sizeof(u32)];
extern u8 *sector_buf = (u8*)sector_buf32;
static u32 sector_buf232[SECTOR_SIZE/sizeof(u32)];
extern u8 *sector_buf2 = (u8*)sector_buf232;
static u32 file_buf32[BUFFER_SIZE/sizeof(u32)];
extern u8 *file_buf = (u8*)file_buf32;

void nandio_set_fat_sig_fix(u32 offset)
{
	fat_sig_fix_offset = offset;
}

void getCID(u8 *CID){
	memcpy(CID,(u8*)0x02FFD7BC,16); //arm9 location
}

void getConsoleID(u8 *consoleID)
{
	u8 *fifo=(u8*)0x02300000; //shared mem address that has our computed key3 stuff
	u8 key[16]; //key3 normalkey - keyslot 3 is used for DSi/twln NAND crypto
	u8 key_x[16];////key3_x - contains a DSi console id (which just happens to be the LFCS on 3ds)

	u8 empty_buff[8] = {0};

	memcpy(key, fifo, 16);  //receive the goods from arm7

	if(memcmp(key + 8, empty_buff, 8) == 0)
	{
		//we got the consoleid directly or nothing at all, don't treat this as key3 output
		memcpy(consoleID, key, 8);
		return;
	}
	F_XY_reverse(key, key_x); //work backwards from the normalkey to get key_x that has the consoleID

	u128_xor(key_x, DSi_NAND_KEY_Y);

	memcpy(&consoleID[0], &key_x[0], 4);
	memcpy(&consoleID[4], &key_x[0xC], 4);
}

void nandGetInfo(void) {

    // Copy over all NAND data from the CID
    nandInfo.NAND_MID = CID[14];
    strcpy(nandInfo.NAND_MID_NAME, "UNKNOWN");
    if (nandInfo.NAND_MID == 0x15) {
        strcpy(nandInfo.NAND_MID_NAME, "SAMSUNG");
    } else if (nandInfo.NAND_MID == 0xFE) {
        strcpy(nandInfo.NAND_MID_NAME, "ST");
    }
    nandInfo.NAND_OID[0] = CID[13];
    nandInfo.NAND_OID[1] = CID[12];
	nandInfo.NAND_PNM[0] = CID[11];
	nandInfo.NAND_PNM[1] = CID[10];
	nandInfo.NAND_PNM[2] = CID[9];
	nandInfo.NAND_PNM[3] = CID[8];
	nandInfo.NAND_PNM[4] = CID[7];
	nandInfo.NAND_PNM[5] = CID[6];
	nandInfo.NAND_PNM[6] = '\0';
    nandInfo.NAND_PRV = CID[5];
    memcpy(nandInfo.NAND_PSN, &CID[4], 4);
    nandInfo.NAND_MDT = CID[0];
    nandInfo.NAND_MDT_MONTH = (CID[0] & 0xF0) >> 4;
    nandInfo.NAND_MDT_YEAR = (CID[0] & 0x0F) - 3;

    return;
}

bool nandio_startup()
{
	if (!nand_Startup())
	{
		return false;
	}

	nand_ReadSectors(0, 1, sector_buf);
	is3DS = parse_ncsd(sector_buf) == 0;
	//if (is3DS) return false;

	// Get ConsoleID
	getConsoleID(consoleID);
	getCID(CID);
	for (int i = 0; i < 8; i++)
	{
		consoleIDfixed[i] = consoleID[7-i];
	}
	nandGetInfo();

	// iprintf("sector 0 is %s\n", is3DS ? "3DS" : "DSi");
	dsi_crypt_init((const u8*)consoleIDfixed, (const u8*)0x2FFD7BC, is3DS);
	dsi_nand_crypt(sector_buf, sector_buf, 0, SECTOR_SIZE / AES_BLOCK_SIZE);

	parse_mbr(sector_buf, is3DS);

	mbr_t *mbr = (mbr_t*)sector_buf;

	nandio_set_fat_sig_fix(is3DS ? 0 : mbr->partitions[0].offset);

	if (crypt_buf == 0)
	{
		crypt_buf = (u8*)memalign(32, SECTOR_SIZE * CRYPT_BUF_LEN);
	}

	return crypt_buf != 0;
}

bool nandio_is_inserted()
{
	return true;
}

// len is guaranteed <= CRYPT_BUF_LEN
static bool read_sectors(sec_t start, sec_t len, void *buffer)
{
	if (nand_ReadSectors(start, len, crypt_buf))
	{
		dsi_nand_crypt(buffer, crypt_buf, start * SECTOR_SIZE / AES_BLOCK_SIZE, len * SECTOR_SIZE / AES_BLOCK_SIZE);
		if (fat_sig_fix_offset &&
			start == fat_sig_fix_offset
			&& ((u8*)buffer)[0x36] == 0
			&& ((u8*)buffer)[0x37] == 0
			&& ((u8*)buffer)[0x38] == 0)
		{
			((u8*)buffer)[0x36] = 'F';
			((u8*)buffer)[0x37] = 'A';
			((u8*)buffer)[0x38] = 'T';
		}
		return true;
	}
	else
	{
		return false;
	}
}

// len is guaranteed <= CRYPT_BUF_LEN
static bool write_sectors(sec_t start, sec_t len, const void *buffer)
{
	static u8 writeCopy[SECTOR_SIZE*16];
	memcpy(writeCopy, buffer, len * SECTOR_SIZE);

	dsi_nand_crypt(crypt_buf, writeCopy, start * SECTOR_SIZE / AES_BLOCK_SIZE, len * SECTOR_SIZE / AES_BLOCK_SIZE);
	if (nand_WriteSectors(start, len, crypt_buf))
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool nandio_read_sectors(sec_t offset, sec_t len, void *buffer)
{
	while (len >= CRYPT_BUF_LEN)
	{
		if (!read_sectors(offset, CRYPT_BUF_LEN, buffer))
		{
			return false;
		}
		offset += CRYPT_BUF_LEN;
		len -= CRYPT_BUF_LEN;
		buffer = ((u8*)buffer) + SECTOR_SIZE * CRYPT_BUF_LEN;
	}
	if (len > 0)
	{
		return read_sectors(offset, len, buffer);
	} else
	{
		return true;
	}
}

bool nandio_write_sectors(sec_t offset, sec_t len, const void *buffer)
{
	if (writingLocked)
		return false;

	nandWritten = true;

	while (len >= CRYPT_BUF_LEN)
	{
		if (!write_sectors(offset, CRYPT_BUF_LEN, buffer))
		{
			return false;
		}
		offset += CRYPT_BUF_LEN;
		len -= CRYPT_BUF_LEN;
		buffer = ((u8*)buffer) + SECTOR_SIZE * CRYPT_BUF_LEN;
	}
	if (len > 0)
	{
		return write_sectors(offset, len, buffer);
	} else
	{
		return true;
	}
}

bool good_nandio_write(int inputAddress, int inputLength, u8 *buffer, bool crypt) {
	// Sorry lol I just don't want to deal with sector calculation
	int byteOffset = inputAddress % SECTOR_SIZE;
	int sectorNum = inputAddress / SECTOR_SIZE;
	int byteEndOffset = (inputAddress+inputLength) % SECTOR_SIZE;
	int sectorEndNum = (inputAddress+inputLength) / SECTOR_SIZE;
	int i;
	if (inputLength <= SECTOR_SIZE) {
		// Handle a single sector write differently since it is unpredictable
		nand_ReadSectors(sectorNum, 1, sector_buf);
		memcpy(sector_buf, buffer, inputLength);
		if (crypt == true) {
			dsi_nand_crypt(sector_buf, sector_buf, sectorNum * SECTOR_SIZE / AES_BLOCK_SIZE, SECTOR_SIZE / AES_BLOCK_SIZE);
		}
		nand_WriteSectors(sectorNum, 1, sector_buf);
		//iprintf("\n%02X to %02X of %02X", byteOffset, inputLength, sectorNum);
		//iprintf("\n%02X to %02X of buffer", 0, inputLength);
	} else {
		iprintf("\n ");
		for (i = sectorNum; i < sectorEndNum + 1;) {
			char currentPicto = downloadPlayLoading(i);
			if (i % (sectorEndNum / 15) == 0) {
				printf("\b%c", currentPicto);
			}
			// Back up sector
		    nand_ReadSectors(i, 1, sector_buf);
			if (i == sectorNum) {
				// Handle the first sector differently since we'll only be writing a partial amount of data
				memcpy(sector_buf + byteOffset, buffer, SECTOR_SIZE - byteOffset);
				//iprintf("\n%02X to %02X of %02X", byteOffset, (SECTOR_SIZE), i);
				//iprintf("\n0 to %02X of buffer", (SECTOR_SIZE - byteOffset), i);
			} else if (i == sectorEndNum) {
				// Handle the last sector differently since we'll only be writing a partial amount of data
				memcpy(sector_buf, buffer + (((i  - sectorNum) * SECTOR_SIZE) - byteOffset), byteEndOffset);
				//iprintf("\n0 to %02X of %02X (end)", byteEndOffset, i);
				//iprintf("\n%02X to %02X of buffer", ((i  - sectorNum) * SECTOR_SIZE) - byteOffset, (((i  - sectorNum) * SECTOR_SIZE) + byteEndOffset) - byteOffset);
			} else {
				// Handle the middle sectors the same because they'll always be the full sector
				memcpy(sector_buf, buffer + (((i  - sectorNum) * SECTOR_SIZE) - byteOffset), SECTOR_SIZE);
				//iprintf("\n0 to %02X of %02X", SECTOR_SIZE, i);
				//iprintf("\n%02X to %02X of buffer", ((i  - sectorNum) * SECTOR_SIZE) - byteOffset, (((i  - sectorNum) * SECTOR_SIZE) - byteOffset) + SECTOR_SIZE);
			}
			// I need to do a cmp here t0 save NAND writes
			// Write sector
			if (crypt == true) {
				// offset * SECTOR_SIZE / AES_BLOCK_SIZE
				dsi_crypt_init((const u8*)consoleIDfixed, (const u8*)0x2FFD7BC, is3DS);
				dsi_nand_crypt(sector_buf, sector_buf, i * SECTOR_SIZE / AES_BLOCK_SIZE, SECTOR_SIZE / AES_BLOCK_SIZE);
				// Okay so the below one encrypted every other sector, failing the 1st, 3rd, 5th, etc.
				//dsi_nand_crypt(sector_buf, sector_buf, inputAddress * SECTOR_SIZE / AES_BLOCK_SIZE, SECTOR_SIZE / AES_BLOCK_SIZE);
			}
			nand_WriteSectors(i, 1, sector_buf);
			i++;
			// Do some check to make sure it is not outside of NAND range.
		}
		iprintf("\b\x1B[1A");
	}
	return true;
}

bool good_nandio_write_file(int inputAddress, int inputLength, FILE *fp, bool crypt) {
	int byteOffset = inputAddress % SECTOR_SIZE;
	int sectorNum = inputAddress / SECTOR_SIZE;
	int byteEndOffset = (inputAddress+inputLength) % SECTOR_SIZE;
	int sectorEndNum = (inputAddress+inputLength) / SECTOR_SIZE;
	int i;
    u8 buffer[SECTOR_SIZE];
	if (inputLength <= SECTOR_SIZE) {
		// Handle a single sector write differently since it is unpredictable
		nand_ReadSectors(sectorNum, 1, sector_buf);
		fread(buffer, 1, inputLength, fp);
		memcpy(sector_buf, buffer, inputLength);
		if (crypt == true) {
			dsi_nand_crypt(sector_buf, sector_buf, sectorNum * SECTOR_SIZE / AES_BLOCK_SIZE, SECTOR_SIZE / AES_BLOCK_SIZE);
		}
		nand_WriteSectors(sectorNum, 1, sector_buf);
	} else {
		iprintf("\n ");
		for (i = sectorNum; i < sectorEndNum + 1;) {
			char currentPicto = downloadPlayLoading(i);
			if (i % (sectorEndNum / 15) == 0) {
				printf("\b%c", currentPicto);
			}
			// Back up sector
		    nand_ReadSectors(i, 1, sector_buf);

			if (i == sectorNum) {
				// Handle the first sector differently since we'll only be writing a partial amount of data
				fread(buffer, 1, SECTOR_SIZE, fp);
				memcpy(sector_buf + byteOffset, buffer, SECTOR_SIZE - byteOffset);
			} else if (i == sectorEndNum) {
				// Handle the last sector differently since we'll only be writing a partial amount of data
				fread(buffer, 1, byteEndOffset, fp);
				memcpy(sector_buf, buffer - byteOffset, byteEndOffset);
			} else {
				// Handle the middle sectors the same because they'll always be the full sector
				fread(buffer, 1, SECTOR_SIZE, fp);
				memcpy(sector_buf, buffer, SECTOR_SIZE);
			}
			// I need to do a cmp here t0 save NAND writes
			// Write sector
			if (crypt == true) {
				dsi_nand_crypt(sector_buf, sector_buf, i * SECTOR_SIZE / AES_BLOCK_SIZE, SECTOR_SIZE / AES_BLOCK_SIZE);
			}
			nand_WriteSectors(i, 1, sector_buf);
			i++;
			// Do some check to make sure it is not outside of NAND range.
		}
		iprintf("\b\x1B[1A");
	}
	return true;
}

bool nandio_clear_status()
{
	return true;
}

bool nandio_shutdown()
{
	if (nandWritten)
	{
		// at cleanup we synchronize the FAT statgings
		// A FatFS might have multiple copies of the FAT.
		// we will get them back synchonized as we just worked on the first copy
		// this allows us to revert changes in the FAT if we did not properly finish
		// and did not push the changes to the other copies
		// to do this we read the first partition sector
		nandio_read_sectors(fat_sig_fix_offset, 1, sector_buf);
		u8 stagingLevels = sector_buf[0x10];
		u8 reservedSectors = sector_buf[0x0E];
		u16 sectorsPerFatCopy = sector_buf[0x16] | ((u16)sector_buf[0x17] << 8);
	/*
		iprintf("[i] Staging for %i FAT copies\n",stagingLevels);
		iprintf("[i] Stages starting at %i\n",reservedSectors);
		iprintf("[i] %i sectors per stage\n",sectorsPerFatCopy);
	*/
		if (stagingLevels > 1)
		{
			for (u32 sector = 0;sector < sectorsPerFatCopy; sector++)
			{
				// read fat sector
				nandio_read_sectors(fat_sig_fix_offset + reservedSectors + sector, 1, sector_buf);
				// write to each copy, except the source copy
				writingLocked = false;
				for (int stage = 1;stage < stagingLevels;stage++)
				{
					nandio_write_sectors(fat_sig_fix_offset + reservedSectors + sector + (stage *sectorsPerFatCopy), 1, sector_buf);
				}
				writingLocked = true;
			}
		}
		nandWritten = false;
	}
	free(crypt_buf);
	crypt_buf = 0;
	return true;
}

bool nandio_lock_writing()
{
	writingLocked = true;

	return writingLocked;
}

bool nandio_unlock_writing()
{
	writingLocked = false;

	return !writingLocked;
}

bool nandio_force_fat_fix()
{
	if (!writingLocked)
		nandWritten = true;

	return true;
}
