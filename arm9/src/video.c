#include <stdlib.h>
#include <stdio.h>
#include <nds.h>
// From https://code.google.com/archive/p/sundevos/
// common headers
#include "video.h"
#include "font.h"

#define SUB ((u16 *)BG_BMP_RAM_SUB(2))
#define MAIN ((u16 *)BG_BMP_RAM(2))

PrintConsole console, consoleSub;

/*
 *	map base : 2Ko
 *	tile base : 16Ko
 *	bmp base : 16Ko
 */

void videoInit() {

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA (VRAM_A_MAIN_BG);
	vramSetBankC (VRAM_C_SUB_BG);

	bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 1, 0);
	bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 1, 0);

	consoleInit(&consoleSub, 3, BgType_Text4bpp, BgSize_T_256x256, 20, 0, false, false);
	consoleInit(&console, 3, BgType_Text4bpp, BgSize_T_256x256, 20, 0, true, false);

	ConsoleFont font;
	font.gfx = (u16*)fontTiles;
	font.pal = (u16*)fontPal;
	font.numChars = 200;
	font.numColors =  fontPalLen / 2;
	font.bpp = 4;
	font.asciiOffset = 32;
	font.convertSingleColor = true;
	consoleSetFont(&console, &font);
	consoleSetFont(&consoleSub, &font);

	setBackdropColor(0xFFFF);
	setBackdropColorSub(0xFFFF);

	lcdMainOnBottom();
	consoleSet(cSUB);
}

void consoleHide(enum console c) {
	if(c & cMAIN) bgHide(console.bgId);
	if(c & cSUB) bgHide(consoleSub.bgId);
}

void consoleShow(enum console c) {
	if(c & cMAIN) bgShow(console.bgId);
	if(c & cSUB) bgShow(consoleSub.bgId);
}

void consoleSet(enum console c) {
	if(c & cMAIN) consoleSelect(&console);
	if(c & cSUB) consoleSelect(&consoleSub);
}

void screenFill(u16 *buf, u16 color) {
	u16 i;
	for(i = 0; i < 256*192; i++) buf[i] = color;
}

