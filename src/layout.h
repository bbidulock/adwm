/* layout.c */

#ifndef __LOCAL_LAYOUT_H__
#define __LOCAL_LAYOUT_H__

typedef enum {
	RotateAreaView,
	RotateAreaZone,
	RotateAreaWins
} RotateArea;

typedef enum {
	RotateDirectionCW,
	RotateDirectionCCW
} RotateDirection;

typedef enum {
	SetValueAbsolute,
	SetValueIncrement,
	SetValueDecrement
} SetValue;

typedef struct Arrangement Arrangement;
struct Arrangement {
	void *handle;
	const char *name;
	void (*initlayout) (View *v);
	void (*addclient) (Client *c, Bool focusme, Bool raisme);
	void (*delclient) (Client *c);
	void (*raise) (Client *c);
	void (*lower) (Client *c);
	void (*raiselower) (Client *c);
	Bool (*isfloating)(Client *c, View *v);
	void (*getdecor)(Client *c, View *v, ClientGeometry *g);
	void (*arrange)(View *v);
	void (*arrangedock)(View *v);
	void (*setnmaster)(View *v, SetValue how, int n);
	void (*setncolumns)(View *v, SetValue how, int n);
	void (*rotate)(Client *c, View *v, RotateDirection dir, RotateArea area);
	void (*zoom)(Client *c);
	void (*zoomfloat)(Client *c);
	void (*togglefloating)(Client *c);
	void (*togglefill)(Client *c);
	void (*togglefull)(Client *c);
	void (*togglemax)(Client *c);
	void (*togglemaxv)(Client *c);
	void (*togglemaxh)(Client *c);
	void (*togglelhalf)(Client *c);
	void (*togglerhalf)(Client *c);
	void (*toggleshade)(Client *c);
	void (*toggledectiled)(Client *c);
};

Bool isvisible(Client *c, View *v);
void addclient(Client *c, Bool focusme, Bool raiseme);
void delclient(Client *c);
void setfocused(Client *c);
void setselected(Client *c);
void tookfocus(Client *c);
Bool enterclient(XEvent *e, Client *c);
Bool configureclient(XEvent *e, Client *c, int gravity);
Bool configuremonitors(XEvent *e, Client *c);
Client *nextdockapp(Client *c, View *v);
Client *prevdockapp(Client *c, View *v);
Client *nexttiled(Client *c, View *v);
Client *prevtiled(Client *c, View *v);
void restack_client(Client *c, int stack_mode, Client *sibling);
void toggleabove(Client *c);
void togglebelow(Client *c);
void arrange(View *v);
void needarrange(View *v);
void arrangeneeded(void);
void setlayout(const char *arg);
void raisefloater(Client *c);
void raisetiled(Client *c);
void lowertiled(Client *c);
void raiseclient(Client *c);
void lowerclient(Client *c);
void raiselower(Client *c);
void setmwfact(View *v, double factor);
void setnmaster(View *v, int n);
void decnmaster(View *v, int n);
void incnmaster(View *v, int n);
void setncolumns(View *v, int n);
void decncolumns(View *v, int n);
void incncolumns(View *v, int n);
Bool mousemove(Client *c, XEvent *e, Bool toggle);
Bool mouseresize_from(Client *c, int from, XEvent *e, Bool toggle);
Bool mouseresize(Client *c, XEvent *e, Bool toggle);
void moveresizeclient(Client *c, int dx, int dy, int dw, int dh, int gravity);
void moveresizekb(Client *c, int dx, int dy, int dw, int dh, int gravity);
void moveto(Client *c, RelativeDirection position);
void moveby(Client *c, RelativeDirection direction, int amount);
void snapto(Client *c, RelativeDirection direction);
void edgeto(Client *c, int direction);
void flipview(Client *c);
void rotateview(Client *c);
void unrotateview(Client *c);
void flipzone(Client *c);
void rotatezone(Client *c);
void unrotatezone(Client *c);
void flipwins(Client *c);
void rotatewins(Client *c);
void unrotatewins(Client *c);
void togglefloating(Client *c);
void togglefill(Client *c);
void togglefull(Client *c);
void togglemax(Client *c);
void togglemaxv(Client *c);
void togglemaxh(Client *c);
void togglelhalf(Client *c);
void togglerhalf(Client *c);
void toggleshade(Client *c);
void toggledectiled(View *v);
void zoom(Client *c);
void zoomfloat(Client *c);

void delleaf(Leaf *l, Bool active);
void appleaf(Container *cp, Leaf *l, Bool active);
void insleaf(Container *cp, Leaf *l, Bool active);
void delnode(Container *cc);
void appnode(Container *cp, Container *cc);
void insnode(Container *cp, Container *cc);
Container *adddocknode(Container *t);

extern Layout layouts[];
extern Arrangement arrangements[];
extern unsigned narr;

#endif				/* __LOCAL_LAYOUT_H__ */
