/** PSP helper library ***************************************/
/**                                                         **/
/**                          video.h                        **/
/**                                                         **/
/** This file contains declarations for the video rendering **/
/** library                                                 **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PSP_VIDEO_H
#define _PSP_VIDEO_H

#include <psptypes.h>

#include "font.h"
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_VIDEO_WHITE	   (u32)0xffffffff
#define PSP_VIDEO_BLACK	   (u32)0xff000000
#define PSP_VIDEO_GRAY	   (u32)0xffcccccc
#define PSP_VIDEO_DARKGRAY (u32)0xff777777
#define PSP_VIDEO_RED	     (u32)0xff0000ff
#define PSP_VIDEO_GREEN	   (u32)0xff00ff00
#define PSP_VIDEO_BLUE	   (u32)0xffff0000
#define PSP_VIDEO_YELLOW   (u32)0xff00ffff
#define PSP_VIDEO_MAGENTA  (u32)0xffff00ff

#define PSP_VIDEO_FC_RESTORE 020
#define PSP_VIDEO_FC_BLACK   021
#define PSP_VIDEO_FC_RED     022
#define PSP_VIDEO_FC_GREEN   023
#define PSP_VIDEO_FC_BLUE    024
#define PSP_VIDEO_FC_GRAY    025
#define PSP_VIDEO_FC_YELLOW  026
#define PSP_VIDEO_FC_MAGENTA 027
#define PSP_VIDEO_FC_WHITE   030

#define PSP_VIDEO_UNSCALED    0
#define PSP_VIDEO_FIT_HEIGHT  1
#define PSP_VIDEO_FILL_SCREEN 2

#define BUF_WIDTH 512
#define SCR_WIDTH 480
#define SCR_HEIGHT 272

#define RGB(r,g,b) (((((b) >> 3) & 0x1F) << 10) | ((((g) >> 3) & 0x1F) << 5) | (((r) >> 3) & 0x1F) | 0x8000)
#define RED(pixel) ((((pixel)) & 0x1f) * 0xff / 0x1f)
#define GREEN(pixel) ((((pixel) >> 5)  & 0x1f) * 0xff / 0x1f)
#define BLUE(pixel) ((((pixel) >> 10) & 0x1f) * 0xff / 0x1f)

#define COLOR(r,g,b,a) (((int)(r)) | ((int)(g) << 8) | ((int)(b) << 16) | ((int)(a) << 24))

extern const unsigned int PspFontColor[];

typedef struct PspVertex
{
  unsigned int color;
  short x, y, z;
} PspVertex;


void pspVideoInit();
void pspVideoShutdown();
void pspVideoClearScreen();
void pspVideoWaitVSync();
void pspVideoSwapBuffers();

void pspVideoBegin();
void pspVideoEnd();

void pspVideoDrawLine(int sx, int sy, int dx, int dy, u32 color);
void pspVideoDrawRect(int sx, int sy, int dx, int dy, u32 color);
void pspVideoFillRect(int sx, int sy, int dx, int dy, u32 color);

int pspVideoPrint(const PspFont *font, int sx, int sy, const char *string, u32 color);
int pspVideoPrintCenter(const PspFont *font, int sx, int sy, int dx, const char *string, u32 color);
int pspVideoPrintN(const PspFont *font, int sx, int sy, const char *string, int count, u32 color);
int pspVideoPrintClipped(const PspFont *font, int sx, int sy, const char* string, int max_w, char* clip, u32 color);
int pspVideoPrintNRaw(const PspFont *font, int sx, int sy, const char *string, int count, u32 color);
int pspVideoPrintRaw(const PspFont *font, int sx, int sy, const char *string, u32 color);

void pspVideoPutImage(const PspImage *image, int dx, int dy, int dw, int dh);
void pspVideoPutImageDirect(const PspImage *image, int dx, int dy, int dw, int dh);

void pspVideoGlowRect(int sx, int sy, int dx, int dy, u32 color, int radius);
void pspVideoShadowRect(int sx, int sy, int dx, int dy, u32 color, int depth);

PspImage* pspVideoGetVramBufferCopy();

void pspVideoCallList(const void *list);

#ifdef __cplusplus
}
#endif

#endif  // _PSP_VIDEO_H
