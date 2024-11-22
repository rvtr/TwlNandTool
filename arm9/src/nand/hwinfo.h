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
#include "../message.h"
#include "../main.h"
#include "../video.h"

// Language code
// Differences in display text between Europe and North America are determined by combining region and language code
typedef enum TWL_LANG
{
    LANG_JAPANESE      = 0,            // Japanese
    LANG_ENGLISH       = 1,            // English
    LANG_FRENCH        = 2,            // French
    LANG_GERMAN        = 3,            // German
    LANG_ITALIAN       = 4,            // Italian
    LANG_SPANISH       = 5,            // Spanish
    LANG_SIMP_CHINESE  = 6,            // Simplified Chinese
    LANG_KOREAN        = 7,            // Korean
    LANG_CODE_MAX
} TWL_LANG;

#define LANG_BITMAP_JAPAN      ( ( 0x0001 << LANG_JAPANESE ) )
#define LANG_BITMAP_AMERICA    ( ( 0x0001 << LANG_ENGLISH ) | \
                                          ( 0x0001 << LANG_FRENCH  ) | \
                                          ( 0x0001 << LANG_SPANISH  ) )
#define LANG_BITMAP_EUROPE     ( ( 0x0001 << LANG_ENGLISH ) | \
                                          ( 0x0001 << LANG_FRENCH  ) | \
                                          ( 0x0001 << LANG_GERMAN  ) | \
                                          ( 0x0001 << LANG_ITALIAN  ) | \
                                          ( 0x0001 << LANG_SPANISH  ) )
#define LANG_BITMAP_AUSTRALIA  ( ( 0x0001 << LANG_ENGLISH  ) )
#define LANG_BITMAP_CHINA      ( ( 0x0001 << LANG_SIMP_CHINESE ) )
#define LANG_BITMAP_KOREA      ( ( 0x0001 << LANG_KOREAN ) )

#define HWINFO_FILE_LENGTH             ( 16 * 1024 )
#define HWN_PATH             "nand:/sys/HWINFO_N.dat"
#define HWS_PATH             "nand:/sys/HWINFO_S.dat"
#define HWS_PATH_SD          "sd:/TwlNandTool/HWINFO_S.dat"

#define HWS_OFFSET_BOX       0x784000
#define HWS_OFFSET_HANDHELD  0x790000
#define HWS_OFFSET_OTHER     0x794000

#define HWN_VERSION          1           // HW information format version (start no.:1)
#define HWS_VERSION          1           // HW information format version (start no.:1)

// I think these are for HWInfo Normal? This is from the TwlSDK Secure7 private package
//#define HWS_MOVABLE_UNIQUE_ID_LEN   16          // Unique ID Transferable between bodies
//#define HWS_MOVABLE_UNIQUE_ID_MASK  0x9865f16bd375414c  // Unique ID shall be XOR of this value with serial no.

typedef struct {
    u32 HWS_SIG[0x80];
    u32 HWS_HEADER;            // Always 0x01000000
    u32 HWS_SIZE;              // Always 0x1C000000
    u32 HWS_LANG;
    u32 HWS_PAD;
    u8 HWS_REGION;
    char HWS_SERIAL[15];            // Last 3 bytes are zerofilled.
                                    // USA serials are 11, and others are 12. 
                                    // Kinda neat, seems like this padding was in case they needed more serials?
    u32 HWS_TID;
} hwsFormat;

extern hwsFormat hwsData;

bool clearHWInfoStruct();
bool loadHWInfoStruct();
bool recoverHWInfo(bool simple);
bool recoverHWInfoOffset(int address);
bool saveHWInfoSDMC(void);