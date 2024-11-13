#ifndef MENU_H
#define MENU_H

#include <nds/ndstypes.h>

#define ITEMS_PER_PAGE 20

typedef struct {
	bool directory;
	char* label;
	char* value;
	char* help;
} Item;

typedef struct {
	int cursor;
	int page;
	int itemCount;
	bool nextPage;
	int changePage;
	char header[32];
	char lheader[32];
	Item items[ITEMS_PER_PAGE];
} Menu;

Menu* newMenu();
void freeMenu(Menu* m);

void addMenuItem(Menu* m, char const* label, char const* value, bool directory, char const* help);
void sortMenuItems(Menu* m);
void setMenuHeader(Menu* m, char* str);
void setListHeader(Menu* m, char* str);

void resetMenu(Menu* m);
void clearMenu(Menu* m);
void clearHelpMenu(Menu* m);
void printHelpMenu(Menu* m);
void printMenu(Menu* m, int level);

bool moveCursor(Menu* m);

char downloadPlayLoading(int number);

#endif