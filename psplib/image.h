/** PSP helper library ***************************************/
/**                                                         **/
/**                         image.h                         **/
/**                                                         **/
/** This file contains declarations for image manipulation  **/
/** routines                                                **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PSP_IMAGE_H
#define _PSP_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

struct PspImage
{
  int Width;
  int Height;
  unsigned short* Pixels;
};

typedef struct PspImage PspImage;

PspImage* pspImageCreate(int width, int height);
void      pspImageDestroy(PspImage *image);
PspImage* pspImageCreateThumbnail(const PspImage *image);
PspImage* pspImageCreateCopy(const PspImage *image);
void      pspImageClear(PspImage *image, unsigned short color);

void pspImageDrawRect(PspImage *image, int sx, int sy, int dx, int dy, unsigned short color);
void pspImageFillRect(PspImage *image, int sx, int sy, int dx, int dy, unsigned short color);

PspImage* pspImageLoadPng(const char *path);
int       pspImageSavePng(const char *path, const PspImage* image);
PspImage* pspImageLoadPngOpen(FILE *fp);
int       pspImageSavePngOpen(FILE *fp, const PspImage* image);

#ifdef __cplusplus
}
#endif

#endif  // _PSP_IMAGE_H
