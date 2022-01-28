/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "draw.h"
#include "buttons.h" /* verification */

typedef Bool (*ActionType)(Client *, XEvent *);

typedef struct {
	char *name;
	Bool (*action)(Client *,XEvent *);
} ActionDef;

static ActionDef ActionDefs[] = {
	{ "spawn3",	m_spawn3    },
	{ "spawn2",	m_spawn2    },
	{ "spawn",	m_spawn	    },
	{ "prevtag",	m_prevtag   },
	{ "nexttag",	m_nexttag   },
	{ "resize",	m_resize    },
	{ "move",	m_move	    },
	{ "shade",	m_shade	    },
	{ "unshade",	m_unshade   },
	{ "zoom",	m_zoom	    },
	{ "restack",	m_restack   },
	{ "none",	NULL	    },
	{ "",		NULL	    },
	{ NULL,		NULL	    }
};

static ActionDef ButtonDefs[] = {
	{ "menu",	b_menu	    },
	{ "iconify",	b_iconify   },
	{ "hide",	b_hide	    },
	{ "withdraw",	b_withdraw  },
	{ "max",	b_maxb	    },
	{ "maxv",	b_maxv	    },
	{ "maxh",	b_maxh	    },
	{ "close",	b_close	    },
	{ "kill",	b_kill	    },
	{ "xkill",	b_xkill	    },
	{ "reshade",	b_reshade   },
	{ "shade",	b_shade	    },
	{ "unshade",	b_unshade   },
	{ "restick",	b_restick   },
	{ "stick",	b_stick	    },
	{ "unstick",	b_unstick   },
	{ "relhalf",	b_relhalf   },
	{ "lhalf",	b_lhalf	    },
	{ "unlhalf",	b_unlhalf   },
	{ "rerhalf",	b_rerhalf   },
	{ "rhalf",	b_rhalf	    },
	{ "unrhalf",	b_unrhalf   },
	{ "refill",	b_refill    },
	{ "fill",	b_fill	    },
	{ "unfill",	b_unfill    },
	{ "refloat",	b_refloat   },
	{ "float",	b_float	    },
	{ "tile",	b_tile	    },
	{ "resize",	b_resize    },
	{ "move",	b_move	    },
	{ "zoom",	b_zoom	    },
	{ "restack",	b_restack   },
	{ "none",	NULL	    },
	{ "",		NULL	    },
	{ NULL,		NULL	    }
};

static void *
findaction(const char *act, ActionDef *def)
{
	if (act) {
		for (; def->name; def++) {
			if (!strcmp(def->name, act)) {
				return (def->action);
			}
		}
	}
	return (NULL);
}

static const char *
findfunction(void *action, ActionDef * def)
{
	for(; def->name; def++) {
		if (action == def->action) {
			return (def->name);
		}
	}
	return (NULL);
}

typedef struct {
	const char *name;
	char *action[Button5-Button1+1][2];
} ActionItem;

static ActionItem ActionItems[] = {
	[OnClientTitle] = {
		"client.title",	    {
			{   "move",	"restack"	},
			{   "move",	"zoom"		},
			{   "resize",	"restack"	},
			{   "shade",	 NULL		},
			{   "unshade",	 NULL		}
		}
	},
	[OnClientGrips] = {
		"client.grips",	    {
			{   "resize",	"restack"	},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		}
		}
	},
	[OnClientFrame] = {
		"client.frame",	    {
			{   "resize",	"restack"	},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		},
			{   "shade",	 NULL		},
			{   "unshade",	 NULL		}
		}
	},
	[OnClientDock] = {
		"client.dock",	    {
			{   "resize",	"restack"	},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		}
		}
	},
	[OnClientWindow] = {
		"client.window",    {
			{   "move",	"restack"	},
			{   "move",	"zoom"		},
			{   "resize",	"restack"	},
			{   "shade",	 NULL		},
			{   "unshade",	 NULL		}
		}
	},
	[OnClientIcon] = {
		"client.icon",	    {
			{   "move",	"restack"	},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		},
			{    NULL,	 NULL		}
		}
	},
	[OnRoot] = {
		"root",		    {
			{   "spawn3",	 NULL		},
			{   "spawn2",	 NULL		},
			{   "spawn",	 NULL		},
			{   "prevtag",	 NULL		},
			{   "nexttag",	 NULL		}
		}
	}
};

static ActionItem ButtonItems[] = {
	[MenuBtn] = {
		"button.menu", {
			{   NULL,	"menu"		},
			{   NULL,	"menu"		},
			{   NULL,	"menu"		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[IconifyBtn] = {
		"button.iconify", {
			{   NULL,	"iconify"	},
			{   NULL,	"hide"		},
			{   NULL,	"withdraw"	},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[MaximizeBtn] = {
		"button.maximize", {
			{   NULL,	"max"		},
			{   NULL,	"maxv"		},
			{   NULL,	"maxh"		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[CloseBtn] = {
		"button.close", {
			{   NULL,	"close"		},
			{   NULL,	"kill"		},
			{   NULL,	"xkill"		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[ShadeBtn] = {
		"button.shade", {
			{   NULL,	"reshade"	},
			{   NULL,	"shade"		},
			{   NULL,	"unshade"	},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[StickBtn] = {
		"button.stick", {
			{   NULL,	"restick"	},
			{   NULL,	"stick"		},
			{   NULL,	"unstick"	},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[LHalfBtn] = {
		"button.lhalf", {
			{   NULL,	"relhalf"	},
			{   NULL,	"lhalf"		},
			{   NULL,	"unlhalf"	},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[RHalfBtn] = {
		"button.rhalf", {
			{   NULL,	"rerhalf"	},
			{   NULL,	"rhalf"		},
			{   NULL,	"unrhalf"	},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[FillBtn] = {
		"button.fill", {
			{   NULL,	"refill"	},
			{   NULL,	"fill"		},
			{   NULL,	"unfill"	},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[FloatBtn] = {
		"button.float", {
			{   NULL,	"refloat"	},
			{   NULL,	"float"		},
			{   NULL,	"tile"		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[SizeBtn] = {
		"button.resize", {
			{  "resize",	 NULL		},
			{  "resize",	 NULL		},
			{  "resize",	 NULL		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[IconBtn] = {
		"button.icon", {
			{   NULL,	"menu"		},
			{   NULL,	"menu"		},
			{   NULL,	"menu"		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[TitleTags] = {
		"title.tags", {
			{   NULL,	 NULL		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	},
	[TitleName] = {
		"title.name", {
			{  "move",	"restack"	},
			{  "move",	"zoom"		},
			{  "resize",	"restack"	},
			{  "shade",	 NULL		},
			{  "unshade",	 NULL		}
		}
	},
	[TitleSep] = {
		"title.separator", {
			{   NULL,	 NULL		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		},
			{   NULL,	 NULL		}
		}
	}
};

char *directions[] = { "pressed", "release" };

void
initbuttons(Bool reload __attribute__((unused)))
{
	unsigned i, j, k;
	char line[256] = { 0, };

	for (i = 0; i < LENGTH(ActionItems); i++) {
		for (j = 0; j < 5; j++) {
			for (k = 0; k < 2; k++) {
				snprintf(line, sizeof(line), "%s.button%u.%s", ActionItems[i].name, (j+1), directions[k]);
				actions[i][j][k] = findaction(getsessionres(line, ActionItems[i].action[j][k]), ActionDefs);
			}
		}
	}
	for (i = 0; i < LENGTH(ButtonItems); i++) {
		for (j = 0; j < 5; j++) {
			for (k = 0; k < 2; k++) {
				snprintf(line, sizeof(line), "%s.button%u.%s", ButtonItems[i].name, (j+1), directions[k]);
				buttons[i][j][k] = findaction(getsessionres(line, ButtonItems[i].action[j][k]), ButtonDefs);
			}
		}
	}
}

void
savebuttons(Bool permanent __attribute__((unused)))
{
	unsigned i, j, k;
	char line[256] = { 0, };
	const char *act;
	Bool (*action)(Client *,XEvent *);

	for (i = 0; i < LENGTH(ActionItems); i++) {
		for (j = 0; j < 5; j++) {
			for (k = 0; k < 2; k++) {
				if (!!(action = actions[i][j][k]) && (act = findfunction(action, ActionDefs))) {
					snprintf(line, sizeof(line), "Adwm*%s.button%u.%s:\t\t%s\n", ActionItems[i].name, (j+1), directions[k], act);
					XrmPutLineResource(&srdb, line);
				}
			}
		}
	}
	for (i = 0; i < LENGTH(ButtonItems); i++) {
		for (j = 0; j < 5; j++) {
			for (k = 0; k < 2; k++) {
				if (!!(action = buttons[i][j][k]) && (act = findfunction(action, ButtonDefs))) {
					snprintf(line, sizeof(line), "Adwm*%s.button%u.%s:\t\t%s\n", ButtonItems[i].name, (j+1), directions[k], act);
					XrmPutLineResource(&srdb, line);
				}
			}
		}
	}
}

void
showbuttons(void)
{
	unsigned i, j, k;
	char line[256] = { 0, };
	const char *act;
	Bool (*action)(Client *,XEvent *);

	for (i = 0; i < LENGTH(ActionItems); i++) {
		for (j = 0; j < 5; j++) {
			for (k = 0; k < 2; k++) {
				if (!!(action = actions[i][j][k]) && (act = findfunction(action, ActionDefs))) {
					snprintf(line, sizeof(line), "Adwm*%s.button%u.%s:\t\t%s\n", ActionItems[i].name, (j+1), directions[k], act);
					fputs(line, stderr);
				}
			}
		}
	}
	for (i = 0; i < LENGTH(ButtonItems); i++) {
		for (j = 0; j < 5; j++) {
			for (k = 0; k < 2; k++) {
				if (!!(action = buttons[i][j][k]) && (act = findfunction(action, ButtonDefs))) {
					snprintf(line, sizeof(line), "Adwm*%s.button%u.%s:\t\t%s\n", ButtonItems[i].name, (j+1), directions[k], act);
					fputs(line, stderr);
				}
			}
		}
	}
}

