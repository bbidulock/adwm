/* config.c */

#ifndef __LOCAL_CONFIG_H__
#define __LOCAL_CONFIG_H__

typedef struct {
	Bool attachaside;
	Bool dectiled;
	Bool decmax;
	Bool hidebastards;
	Bool autoroll;
	int focus;
	int snap;
	char command[255];
	DockPosition dockpos;
	DockOrient dockori;
	unsigned dragdist;
} Options;

void initconfig(void);
extern Options options;

#endif				/* __LOCAL_CONFIG_H__ */
