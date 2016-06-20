/* parse.c */

#ifndef __LOCAL_PARSE_H__
#define __LOCAL_PARSE_H__

void initrules(void);
void initkeys(void);
void freekeys(void);
void parsekeys(const char *s, Key *spec);
void addchain(Key *chain);
void freechain(Key *chain);
const char *showchain(Key *k);

#endif				/* __LOCAL_PARSE_H__ */
