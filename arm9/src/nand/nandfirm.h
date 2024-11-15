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

void wait(int ticks);
void death(char *message, u8 *buffer);

int nfMain(void);

bool nandFirmRead(void);
bool nandFirmImport(bool sdmc);
bool nandPrintInfo(void);