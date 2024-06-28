#ifndef _GAME_FONTS_H
#define _GAME_FONTS_H

typedef unsigned long uint64;

extern int FontPrintNormal(int xpos, int ypos, uint64 rgba, char *s, int len);
extern int FontPrintSmall(int xpos, int ypos, uint64 rgba, char *s, int len);
extern int FontPrintLarge(int xpos, int ypos, uint64 rgba, char *s, int len);

extern int FontPrintCenter(int xpos, int ypos, uint64 rgba, char *s, int len);
extern int FontPrintCenterSmall(int xpos, int ypos, uint64 rgba, char *s, int len);
extern int FontPrintCenterLarge(int xpos, int ypos, uint64 rgba, char *s, int len);

extern void FontPrintRight(int xpos, int ypos, uint64 rgba, char *s, int len);
extern void FontPrintRightSmall(int xpos, int ypos, uint64 rgba, char *s, int len);
extern void FontPrintRightLarge(int xpos, int ypos, uint64 rgba, char *s, int len);

#endif
