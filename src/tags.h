/* tags.c */

#ifndef __LOCAL_TAGS_H__
#define __LOCAL_TAGS_H__

void settags(unsigned numtags);
void appendtag(void);
void rmlasttag(void);

void tag(Client *c, int index);
void taketo(Client *c, int index);

void togglesticky(Client *c);
void toggletag(Client *c, int index);
void toggleview(View *v, int index);
void focusview(View *v, int index);

void view(View *ov, int index);
void viewlefttag(void);
void viewprevtag(void);
void viewrighttag(void);

#endif				/* __LOCAL_TAGS_H__ */
