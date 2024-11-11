#include <stdlib.h>
#include <stdio.h>
#include <nds.h>
// From https://code.google.com/archive/p/sundevos/
// common headers
#include "video.h"

#define SUB ((u16 *)BG_BMP_RAM_SUB(2))
#define MAIN ((u16 *)BG_BMP_RAM(2))

PrintConsole console, consoleSub;

/*
 *	map base : 2Ko
 *	tile base : 16Ko
 *	bmp base : 16Ko
 */

void videoInit() {
	consoleDebugInit(DebugDevice_CONSOLE);	// debug to console

	videoSetMode(MODE_5_3D);
		vramSetBankA(VRAM_A_MAIN_BG_0x06000000);	// allocate 128Ko at 0x06000000

		// <-- console (debug console for binaries loaded)
		consoleInit(
			&console,	// the console to be initted
			1,	// bgLayer
			BgType_Text4bpp,	// bg type
			BgSize_T_256x256,	// bg size
			4,	// map base
			0,	// tile base
			true,	// main display
			true	// load graphics
		);
		iprintf("\x1b[32;1m");	// Set the color to green
		// --> <-- 2D (96Ko at 0x06008000)
		bgInit(
			3,	// bg layer
			BgType_Bmp16,	// bg type
			BgSize_B16_256x256,	// bg size
			2,	// map base (here it's the bmp base)
			0	// tile base (here it's useless)
		);
		screenFill(MAIN, RGB15(31, 31, 31)|BIT(15));
		// --> <-- 3D
		/* <-- old
		glInit();
		glClearColor(0,0,0,0);				// make the BG transparent
		glClearDepth(0x7FFF);
		glViewport(0,0,255,191);			// Set our viewport to be the same size as the screen
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(70, 256.0 / 192.0, 0.1, 100);
		--> */

		#define MIN_X (0.0f)
		#define MAX_X (4.0f)
		#define MIN_Y (0.0f)
		#define MAX_Y (3.0f)
		glInit();
		glClearColor(0,0,0,0); // BG must be opaque for AA to work
		//glClearPolyID(63); // BG must have a unique polygon ID for AA to work
		glClearDepth(0x7FFF);
		glViewport(0,0,255,191);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrthof32(floattof32(MIN_X), floattof32(MAX_X), floattof32(MIN_Y), floattof32(MAX_Y), floattof32(0.1), floattof32(10));
		glMatrixMode(GL_MODELVIEW);
		glMaterialf(GL_AMBIENT, RGB15(31,31,31));
		glMaterialf(GL_DIFFUSE, RGB15(31,31,31));
		glMaterialf(GL_SPECULAR, BIT(15) | RGB15(31,31,31));
		glMaterialf(GL_EMISSION, RGB15(31,31,31));
		glMaterialShinyness();
		glMatrixMode(GL_MODELVIEW);
		// -->

	videoSetModeSub(MODE_5_2D);
		vramSetBankC(VRAM_C_SUB_BG_0x06200000);	// allocate 128Ko at 0x06200000

		// <-- console (debug console for SunOS)
		consoleInit(
			&consoleSub,	// the console to be initted
			1,	// bgLayer
			BgType_Text4bpp,	// bg type
			BgSize_T_256x256,	// bg size
			4,	// map base
			0,	// tile base
			false,	// main display
			true	// load graphics
		);
		iprintf("\x1b[31;1m");	// Set the color to red
		// --> <-- 2D (96Ko at 0x06208000)
		bgInitSub(
			3,	// bg layer
			BgType_Bmp16,	// bg type
			BgSize_B16_256x256,	// bg size
			2,	// map base (here it's the bmp base)
			0	// tile base (here it's useless)
		);
		screenFill(SUB, RGB15(31, 31, 31)|BIT(15));
		// -->

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

