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

int sysfileMain();

bool recoverHWInfo();
bool recoverHWInfoDeep();
bool makeSystemFolders();
bool makeCertChain();
bool makeFontTable();