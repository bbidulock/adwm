/* image.c */

#ifndef __LOCAL_IMAGE_H__
#define __LOCAL_IMAGE_H__

void renderimage(AScreen *ds, const ARGB *argb, const unsigned width, const unsigned height);
void initimage(Bool reload);

#endif				/* __LOCAL_IMAGE_H__ */
