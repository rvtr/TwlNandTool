#include "menu.h"
#include "main.h"
#include "video.h"
#include "audio.h"
#include "version.h"
#include "nand/nandio.h"

Menu* newMenu()
{
	Menu* m = (Menu*)malloc(sizeof(Menu));

	m->cursor = 0;
	m->page = 0;
	m->itemCount = 0;
	m->nextPage = false;
	m->changePage = 0;
	m->header[0] = '\0';
	m->lheader[0] = '\0';

	for (int i = 0; i < ITEMS_PER_PAGE; i++)
	{
		m->items[i].directory = false;
		m->items[i].label = NULL;
		m->items[i].value = NULL;
		m->items[i].help = NULL;
	}

	return m;
}

void freeMenu(Menu* m)
{
	if (!m) return;

	clearMenu(m);

	free(m);
	m = NULL;
}

void addMenuItem(Menu* m, char const* label, char const* value, bool directory, char const* help)
{
	if (!m) return;

	int i = m->itemCount;
	if (i >= ITEMS_PER_PAGE) return;

	m->items[i].directory = directory;

	if (label)
	{
		m->items[i].label = (char*)malloc(32);
		sprintf(m->items[i].label, "%.31s", label);
	}

	if (value)
	{
		m->items[i].value = (char*)malloc(strlen(value)+1);
		sprintf(m->items[i].value, "%s", value);
	}

	if (help)
	{
		m->items[i].help = (char*)malloc(200);
		sprintf(m->items[i].help, "%s", help);
	}

	m->itemCount += 1;
}

static int alphabeticalCompare(const void* a, const void* b)
{
	const Item* itemA = (const Item*)a;
	const Item* itemB = (const Item*)b;

	if (itemA->directory && !itemB->directory)
		return -1;
	else if (!itemA->directory && itemB->directory)
		return 1;
	else
		return strcasecmp(itemA->label, itemB->label);
}

void sortMenuItems(Menu* m)
{
	qsort(m->items, m->itemCount, sizeof(Item), alphabeticalCompare);
}

void setMenuHeader(Menu* m, char* str)
{
	if (!m) return;

	if (!str)
	{
		m->header[0] = '\0';
		return;
	}

	char* strPtr = str;

	if (strlen(strPtr) > 30)
		strPtr = str + (strlen(strPtr) - 30);

	sprintf(m->header, "%.30s", strPtr);
}


void setListHeader(Menu* m, char* str)
{
	if (!m) return;

	if (!str)
	{
		m->lheader[0] = '\0';
		return;
	}

	char* strPtr = str;

	if (strlen(strPtr) > 30)
		strPtr = str + (strlen(strPtr) - 30);

	sprintf(m->lheader, "%.30s", strPtr);
}

void resetMenu(Menu* m)
{
	m->cursor = 0;
	m->page = 0;
	m->changePage = 0;
	m->nextPage = 0;
}

void clearMenu(Menu* m)
{
	if (!m) return;

	for (int i = 0; i < ITEMS_PER_PAGE; i++)
	{
		if (m->items[i].label)
		{
			free(m->items[i].label);
			m->items[i].label = NULL;
		}

		if (m->items[i].value)
		{
			free(m->items[i].value);
			m->items[i].value = NULL;
		}
	}

	m->itemCount = 0;
}

void clearHelpMenu(Menu* m)
{
	consoleSet(cMAIN);
	iprintf("\x1B[40m\x1b[0;0H\x1B[40m%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%cHelp%c%c%c%c%c", (char)138, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)134, (char)135, (char)136, (char)136, (char)136, (char)140);
	iprintf("\x1b[1;0H\x1b[K");
	iprintf("\x1b[2;0H\x1b[K");
	iprintf("\x1b[3;0H\x1b[K");
	iprintf("\x1b[4;0H\x1b[K");
	iprintf("\x1b[5;0H\x1b[K");
	iprintf("\x1b[6;0H\x1b[K");
	iprintf("\x1b[7;0H\x1b[K");
	iprintf("\x1b[8;0H\x1b[K");
	iprintf("\x1b[9;0H\x1b[K");
	iprintf("\x1b[10;0H\x1b[K");
	iprintf("\x1b[11;0H\x1b[K");
	iprintf("\x1b[12;0H\x1b[K");
	iprintf("\x1b[13;0H\x1b[K");
	iprintf("\x1b[14;0H%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", (char)139, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)141);
	iprintf("\x1b[15;0H\x1b[K");
	iprintf("\x1b[16;0H\x1B[40m%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%cInfo%c%c%c%c%c", (char)138, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)134, (char)135, (char)136, (char)136, (char)136, (char)140);
	iprintf("\x1B[30m\x1b[17;0H\x1b[K %s %s", m->header, VERSION);
	iprintf("\x1b[18;0H\x1b[K %s RMC/RVTR", BUILD_DATE);
	iprintf("\x1b[19;0H\x1b[K");
	iprintf("\x1b[20;0H\x1b[K Serial:  AAAMP1234567");
	iprintf("\x1b[21;0H\x1b[K Version: v0.1A (ALL)");
	iprintf("\x1b[22;0H\x1b[K Run on:  %s\x1B[40m", consoleType);
	iprintf("\x1b[23;0H%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\x1B[30m", (char)139, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)136, (char)141);
}

void printHelpMenu(Menu* m) 
{
	if (!m) return;
	consoleSet(cMAIN);
	iprintf("\x1B[30m\x1b[1;0H %s", m->items[m->cursor].help);
}

void printDebugIDs(Menu* m) 
{
	if (!m) return;
	consoleSet(cSUB);
	iprintf("\x1b[22;0H[");
	int i;
    for (i = 8; i > 0;) {
    	i--;
        printf("%02X", consoleID[i]);
    }
    iprintf("]\x1b[23;0H[");
    for (i = 15; i > 0;) {
    	i--;
        printf("%02X", CID[i]);
    }
    iprintf("]");
}

void printMenu(Menu* m, int level)
{

	clearHelpMenu(m);
	printHelpMenu(m);
	printDebugIDs(m);
	consoleSet(cSUB);

	if (!m) return;

	if (m->itemCount <= 0)
	{
		iprintf("Back - [B]\n");
		return;
	}

    int longestLength = 0;

    for (int i = 0; i < m->itemCount; i++) {
        if (m->items[i].label) {
            int currentLength = strlen(m->items[i].label);
            if (currentLength > longestLength) {
                longestLength = currentLength;
            }
        }
    }

/*

(char)134 = lbracket
(char)135 = rbracket

	Double line borders:
(char)130 = top lcorner
(char)128 = hpipe
(char)132 = top rcorner
(char)129 = vpipe
(char)131 = bot lcorner
(char)133 = bot rcorner

	Single line borders:

(char)138 = top lcorner
(char)136 = hpipe
(char)140 = top rcorner
(char)137 = vpipe
(char)139 = bot lcorner
(char)141 = bot rcorner
*/

iprintf("\x1B[30m\x1b[%d;%dH%c%c%c%s%c", 1 + level, level + 1, (char)138, (char)136, (char)134, m->lheader, (char)135);
for (int j = 0; j < longestLength - 3 - strlen(m->lheader); j++) {
    printf("%c", (char)136);
}
printf("%c\n", (char)140);
	//items
	for (int i = 0; i < m->itemCount; i++)
	{
		if (m->items[i].label)
		{
			if (m->items[i].directory) {
				iprintf("\x1b[%d;%dH%c[%.28s]", i + 2 + level, level + 1, (char)137, m->items[i].label);
			    for (int j = strlen(m->items[i].label); j < longestLength; j++) {
			        printf(" ");
			    }
			    printf("%c\n", (char)137);
			} else {
				iprintf("\x1b[%d;%dH%c%.30s", i + 2 + level, level + 1, (char)137, m->items[i].label);
			    for (int j = strlen(m->items[i].label); j < longestLength; j++) {
			        printf(" ");
			    }
			    printf("%c\n", (char)137);
			}
		}
		else {
			iprintf(" \n");
		}
	}

iprintf("\x1b[%d;%dH%c", m->itemCount + 2 + level, level + 1, (char)139);
for (int j = 0; j < longestLength; j++) {
    printf("%c", (char)136);
}
printf("%c\n", (char)141);

	//cursor
	iprintf("\x1b[%d;%dH%c\x1B[41m%s\x1B[30m\x1b[%dC%c", m->cursor + 2 + level, level + 1, (char)137, m->items[m->cursor].label, longestLength - strlen(m->items[m->cursor].label), (char)137);

	//scroll arrows
	if (m->page > 0)
		iprintf("\x1b[2;31H^");

	if (m->nextPage)
		iprintf("\x1b[21;31Hv");
}

static void _moveCursor(Menu* m, int dir)
{
	if (m->changePage != 0)
		return;

	m->cursor += sign(dir);

	if (m->cursor < 0)
	{
		if (m->page <= 0)
			m->cursor = 0;
		else
		{
			m->changePage = -1;
			m->cursor = ITEMS_PER_PAGE - 1;
		}
	}

	else if (m->cursor > m->itemCount-1)
	{
		if (m->nextPage && m->cursor >= ITEMS_PER_PAGE)
		{
			m->changePage = 1;
			m->cursor = 0;
		}
		else
		{
			m->cursor = m->itemCount-1;
		}
	}
}

bool moveCursor(Menu* m)
{
	if (!m) return false;

	m->changePage = 0;
	int lastCursor = m->cursor;

	u32 down = keysDownRepeat();

	if (down & KEY_DOWN) {
		soundPlaySelect();
		_moveCursor(m, 1);
	} else if (down & KEY_UP) {
		soundPlaySelect();
		_moveCursor(m, -1);
	}

	if (down & KEY_RIGHT)
	{
		soundPlaySelect();
		repeat(10)
			_moveCursor(m, 1);
	}

	else if (down & KEY_LEFT)
	{
		soundPlaySelect();
		repeat(10)
			_moveCursor(m, -1);
	}

	return !(lastCursor == m->cursor);
}


void wait(int ticks){
	while(ticks--)swiWaitForVBlank();
}

int loadingCounter = 0;
char downloadPlayLoading(int number) {
	char pictoload[] = {(char)143, (char)144, (char)145, (char)146, (char)147, (char)148, (char)149, (char)150};
	if (loadingCounter >= 7) {
		loadingCounter = 0;
	} else {
		loadingCounter++;
	}
	return pictoload[loadingCounter];
}

void exitFunction() {
	if (agingMode == false) {
		wait(25);
		if (success == true) {
			setBackdropColorSub(0x1FE0);
			soundPlayPass();
		} else {
			setBackdropColorSub(0x00FF);
			soundPlayFail();
		}
		iprintf("\n\n  Please Push Select To Return  ");
		while (true)
		{
			swiWaitForVBlank();
			scanKeys();

			if (keysDown() & KEY_SELECT ) {
				soundPlayBack();
				break;
			}
		}
	} else {
		wait(50);
	}
	clearScreen(cSUB);
	setBackdropColorSub(0xFFFF);
	// Restore screen memory here
}