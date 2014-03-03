/* layout.c */

#ifndef __LOCAL_LAYOUT_H__
#define __LOCAL_LAYOUT_H__

void addclient(Client *c, Bool focusme, Bool raiseme);
void delclient(Client *c);
void tookfocus(Client *c);
Bool isfloating(Client *c, Monitor *m);
Bool enterclient(XEvent *e, Client *c);
Bool configureclient(XEvent *e, Client *c, int gravity);
Bool configuremonitors(XEvent *e, Client *c);
Client *nextdockapp(Client *c, Monitor *m);
Client *prevdockapp(Client *c, Monitor *m);
Client *nexttiled(Client *c, Monitor *m);
Client *prevtiled(Client *c, Monitor *m);
void restack_client(Client *c, int stack_mode, Client *sibling);
void toggleabove(Client *c);
void togglebelow(Client *c);
void arrange(Monitor *m);
void setlayout(const char *arg);
void raisetiled(Client *c);
void lowertiled(Client *c);
void raiseclient(Client *c);
void lowerclient(Client *c);
void raiselower(Client *c);
void setmwfact(Monitor *m, View *v, double factor);
void setnmaster(Monitor *m, View *v, int n);
void decnmaster(Monitor *m, View *v, int n);
void incnmaster(Monitor *m, View *v, int n);
Bool mousemove(Client *c, XEvent *e, Bool toggle);
Bool mouseresize_from(Client *c, int from, XEvent *e, Bool toggle);
Bool mouseresize(Client *c, XEvent *e, Bool toggle);
void moveresizekb(Client *c, int dx, int dy, int dw, int dh, int gravity);
void moveto(Client *c, RelativeDirection position);
void moveby(Client *c, RelativeDirection direction, int amount);
void snapto(Client *c, RelativeDirection direction);
void edgeto(Client *c, int direction);
void rotateview(Client *c);
void unrotateview(Client *c);
void rotatezone(Client *c);
void unrotatezone(Client *c);
void rotatewins(Client *c);
void unrotatewins(Client *c);
void togglefloating(Client *c);
void togglefill(Client *c);
void togglefull(Client *c);
void togglemax(Client *c);
void togglemaxv(Client *c);
void togglemaxh(Client *c);
void toggleshade(Client *c);
void toggledectiled(Monitor *m, View *v);
void zoom(Client *c);
void zoomfloat(Client *c);

#endif				/* __LOCAL_LAYOUT_H__ */
