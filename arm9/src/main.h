#ifndef MAIN_H
#define MAIN_H

#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include "nand/nandfirm.h"
#include "nand/nandio.h"
#include "nand/filesystem.h"
#include "video.h"
#include "nitrofs.h"

extern bool programEnd;
extern bool sdnandMode;
extern bool unlaunchFound;
extern bool unlaunchPatches;
extern bool charging;
extern u8 batteryLevel;
extern u8 region;
extern u32 consoleSign;
extern char consoleSignName[9];

void installMenu();
void titleMenu();
void backupMenu();
void testMenu();
int debug1();

extern PrintConsole topScreen;
extern PrintConsole bottomScreen;

void clearScreen(enum console c);

#define abs(X) ( (X) < 0 ? -(X): (X) )
#define sign(X) ( ((X) > 0) - ((X) < 0) )
#define repeat(X) for (int _I_ = 0; _I_ < (X); _I_++)

#endif