#ifndef _VIDEO_H
#define _VIDEO_H

extern enum console {
	cMAIN=	1<<0,
	cSUB=	1<<1
}consolescreens;

void	videoInit	();
void	consoleHide	(enum console c);
void	consoleShow	(enum console c);
void	consoleSet	(enum console c);
void	screenFill	(u16 *buf, u16 color);

#endif

