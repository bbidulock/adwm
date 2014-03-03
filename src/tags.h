/* tags.c */
void inittags(void);
void initlayouts(void);
void settags(unsigned numtags);
void appendtag(void);
void rmlasttag(void);

void tag(Client *c, int index);
void taketo(Client *c, int index);

void togglesticky(Client *c);
void toggletag(Client *c, int index);
void toggleview(Monitor *cm, int index);
void focusview(Monitor *m, int index);

void view(int index);
void viewlefttag(void);
void viewprevtag(void);
void viewrighttag(void);
