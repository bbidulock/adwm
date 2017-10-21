/* parse.c */

#ifndef __LOCAL_PARSE_H__
#define __LOCAL_PARSE_H__

void initrules(Bool reload);
void initkeys(Bool reload);
void freekeys(void);
void parsekeys(const char *s, Key *spec);
void addchain(Key *chain);
void freechain(Key *chain);
const char *showchain(Key *k);

#endif				/* __LOCAL_PARSE_H__ */
