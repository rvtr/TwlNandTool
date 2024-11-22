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

#define CERT_PATH             "nand:/sys/cert.sys"
#define DEVKP_PATH            "nand:/sys/dev.kp"
#define HWID_PATH             "nand:/sys/HWID.sgn"
#define FONT_PATH             "nand:/sys/TWLFontTable.dat"
#define FLOG_PATH             "nand:/sys/log/product.log"
#define CFG0_PATH             "nand:/shared1/TWLCFG0.dat"
#define CFG1_PATH             "nand:/shared1/TWLCFG1.dat"
#define WRAP_PATH             "nand:/shared2/launcher/wrap.bin"

void wait(int ticks);

int sysfileMain();

bool makeSystemFolders();
bool makeCertChain();
bool makeFontTable();