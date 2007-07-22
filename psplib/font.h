/** PSP helper library ***************************************/
/**                                                         **/
/**                          font.h                         **/
/**                                                         **/
/** This file contains declarations for font manipulation   **/
/** routines                                                **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PSP_FONT_H
#define _PSP_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_FONT_ANALUP    "\251"
#define PSP_FONT_ANALDOWN  "\252"
#define PSP_FONT_ANALLEFT  "\253"
#define PSP_FONT_ANALRIGHT "\254"
#define PSP_FONT_UP        "\245"
#define PSP_FONT_DOWN      "\246"
#define PSP_FONT_LEFT      "\247"
#define PSP_FONT_RIGHT     "\250"
#define PSP_FONT_SQUARE    "\244"
#define PSP_FONT_CROSS     "\241"
#define PSP_FONT_CIRCLE    "\242"
#define PSP_FONT_TRIANGLE  "\243"
#define PSP_FONT_LTRIGGER  "\255"
#define PSP_FONT_RTRIGGER  "\256"
#define PSP_FONT_SELECT    "\257\260"
#define PSP_FONT_START     "\261\262"

#define PSP_FONT_RESTORE 020
#define PSP_FONT_BLACK   021
#define PSP_FONT_RED     022
#define PSP_FONT_GREEN   023
#define PSP_FONT_BLUE    024
#define PSP_FONT_GRAY    025
#define PSP_FONT_YELLOW  026
#define PSP_FONT_MAGENTA 027
#define PSP_FONT_WHITE   030

struct PspFont
{
  unsigned char Height;
  unsigned char Ascent;
  struct
  {
    unsigned char Width;
    unsigned short *Char;
  } Chars[256];
};

typedef struct PspFont PspFont;

extern const PspFont PspStockFont;

int pspFontGetLineHeight(const PspFont *font);
int pspFontGetTextWidth(const PspFont *font, const char *string);
int pspFontGetTextHeight(const PspFont *font, const char *string);


#ifdef __cplusplus
}
#endif

#endif  // _PSP_FONT_H
