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
#include "../menu.h"

int fsMain(void);
bool readMbr(void);
bool repairMbr(bool autofix);
bool formatMain(void);
bool formatPhoto(void);
bool mountMain(void);
bool unmountMain(void);
bool mountNitroFS(void);

bool filetestMain(void);
bool filetestNitro(void);