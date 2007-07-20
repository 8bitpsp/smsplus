/** PSP helper library ***************************************/
/**                                                         **/
/**                          image.c                        **/
/**                                                         **/
/** This file contains image manipulation routines.         **/
/** Parts of the PNG loading/saving code are based on, and  **/
/** adapted from NesterJ v1.11 code by Ruka                 **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include <malloc.h>
#include <string.h>
#include <png.h>
#include <pspgu.h>

#include "video.h"
#include "image.h"

typedef unsigned char byte;

/* Creates an image in memory */
PspImage* pspImageCreate(int width, int height, int bpp)
{
  if (bpp != PSP_IMAGE_INDEXED && bpp != PSP_IMAGE_16BPP) return NULL;

  int size = width * height * (bpp / 8);
  void *pixels = memalign(16, size);

  if (!pixels) return NULL;

  PspImage *image = (PspImage*)malloc(sizeof(PspImage));

  if (!image)
  {
    free(pixels);
    return NULL;
  }

  memset(pixels, 0, size);

  image->Width = width;
  image->Height = height;
  image->Pixels = pixels;

  image->Viewport.X = 0;
  image->Viewport.Y = 0;
  image->Viewport.Width = width;
  image->Viewport.Height = height;

  int i;
  for (i = 1; i < width; i *= 2);
  image->PowerOfTwo = (i == width);
  image->BytesPerPixel = bpp / 8;
  image->FreeBuffer = 1;
  image->Depth = bpp;
  memset(image->Palette, 0, sizeof(image->Palette));

  switch (image->Depth)
  {
  case PSP_IMAGE_INDEXED:
    image->TextureFormat = GU_PSM_T8;
    break;
  case PSP_IMAGE_16BPP:
    image->TextureFormat = GU_PSM_5551;
    break;
  }

  return image;
}

/*
PspImage* pspImageCreate(int width, int height)
{
  int size = width * height * sizeof(unsigned short);
  unsigned short* pixels = (unsigned short*)memalign(16, size);

  if (!pixels) return NULL;

  PspImage *image = (PspImage*)malloc(sizeof(PspImage));

  if (!image)
  {
    free(pixels);
    return NULL;
  }

  memset(pixels, 0, size);

  image->Width = width;
  image->Height = height;
  image->Pixels = pixels;

  image->Viewport.X = 0;
  image->Viewport.Y = 0;
  image->Viewport.Width = width;
  image->Viewport.Height = height;
  image->FreeBuffer = 1;

  return image;
}
*/

/* Creates an image using portion of VRAM */
PspImage* pspImageCreateVram(int width, int height, int bpp)
{
  if (bpp != PSP_IMAGE_INDEXED && bpp != PSP_IMAGE_16BPP) return NULL;

  int size = width * height * (bpp / 8);
  void *pixels = pspVideoAllocateVramChunk(size);

  if (!pixels) return NULL;

  PspImage *image = (PspImage*)malloc(sizeof(PspImage));
  if (!image) return NULL;

  memset(pixels, 0, size);

  image->Width = width;
  image->Height = height;
  image->Pixels = pixels;

  image->Viewport.X = 0;
  image->Viewport.Y = 0;
  image->Viewport.Width = width;
  image->Viewport.Height = height;

  int i;
  for (i = 1; i < width; i *= 2);
  image->PowerOfTwo = (i == width);
  image->BytesPerPixel = bpp / 8;
  image->FreeBuffer = 0;
  image->Depth = bpp;
  memset(image->Palette, 0, sizeof(image->Palette));

  switch (image->Depth)
  {
  case PSP_IMAGE_INDEXED:
    image->TextureFormat = GU_PSM_T8;
    break;
  case PSP_IMAGE_16BPP:
    image->TextureFormat = GU_PSM_5551;
    break;
  }

  return image;
}

/* Destroys image */
void pspImageDestroy(PspImage *image)
{
  if (image->FreeBuffer) free(image->Pixels);
  free(image);
}

/* TODO: fix */
PspImage* pspImageCreateThumbnail(const PspImage *image)
{
  PspImage *thumb;
  int i, j, p;

  if (!(thumb = pspImageCreate(image->Viewport.Width / 2,
    image->Viewport.Height / 2, image->Depth)))
      return NULL;

  int dy = image->Viewport.Y + image->Viewport.Height;
  int dx = image->Viewport.X + image->Viewport.Width;

  for (i = image->Viewport.Y, p = 0; i < dy; i += 2)
    for (j = image->Viewport.X; j < dx; j += 2)
      ; //thumb->Pixels[p++] = image->Pixels[(image->Width * i) + j];
/* AKTODO!!: implement */
  return thumb;
}

/* Creates an exact copy of the image */
PspImage* pspImageCreateCopy(const PspImage *image)
{
  PspImage *copy;

  /* Create image */
  if (!(copy = pspImageCreate(image->Width, image->Height, image->Depth)))
    return NULL;

  /* Copy pixels */
  int size = image->Width * image->Height * image->BytesPerPixel;
  memcpy(copy->Pixels, image->Pixels, size);
  memcpy(&copy->Viewport, &image->Viewport, sizeof(PspViewport));

  return copy;
}

/* Clears an image */
void pspImageClear(PspImage *image, unsigned int color)
{
  if (image->BytesPerPixel == 1)
    memset(image->Pixels, color, image->Width * image->Height);
  else if (image->BytesPerPixel == 2)
  {
    int i;
    unsigned short *pixel = image->Pixels;
    for (i = image->Width * image->Height - 1; i >= 0; i--, pixel++)
      *pixel = color & 0xffff;
  }
}

/* Loads an image from a file */
PspImage* pspImageLoadPng(const char *path)
{
  FILE *fp = fopen(path,"rb");
  if(!fp) return NULL;

  PspImage *image = pspImageLoadPngOpen(fp);
  fclose(fp);

  return image;
}

int pspImageSavePng(const char *path, const PspImage* image)
{
  FILE *fp = fopen( path, "wb" );
	if (!fp) return 0;

  int stat = pspImageSavePngOpen(fp, image);
  fclose(fp);

  return stat;
}

/* Loads an image from an open file stream */
PspImage* pspImageLoadPngOpen(FILE *fp)
{
  const size_t nSigSize = 8;
  byte signature[nSigSize];
  if (fread(signature, sizeof(byte), nSigSize, fp) != nSigSize)
    return 0;

  if (!png_check_sig(signature, nSigSize))
    return 0;

  png_struct *pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!pPngStruct)
    return 0;

  png_info *pPngInfo = png_create_info_struct(pPngStruct);
  if(!pPngInfo)
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  if (setjmp(pPngStruct->jmpbuf))
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  png_init_io(pPngStruct, fp);
  png_set_sig_bytes(pPngStruct, nSigSize);
  png_read_png(pPngStruct, pPngInfo,
    PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
    PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR , NULL);

  png_uint_32 width = pPngInfo->width;
  png_uint_32 height = pPngInfo->height;
  int color_type = pPngInfo->color_type;

  PspImage *image;

  if (!(image = pspImageCreate(width, height, PSP_IMAGE_16BPP)))
  {
    png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
    return 0;
  }

  png_byte **pRowTable = pPngInfo->row_pointers;
  unsigned int x, y;
  byte r, g, b;
  unsigned short *out = image->Pixels;

  for (y=0; y<height; y++)
  {
    png_byte *pRow = pRowTable[y];
    for (x=0; x<width; x++)
    {
      switch(color_type)
      {
        case PNG_COLOR_TYPE_GRAY:
          r = g = b = *pRow++;
          break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
          r = g = b = pRow[0];
          pRow += 2;
          break;
        case PNG_COLOR_TYPE_RGB:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          pRow += 3;
          break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          pRow += 4;
          break;
        default:
          r = g = b = 0;
          break;
      }

      *out++=RGB(r,g,b);
    }
  }

  png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);

  return image;
}

/* TODO: fix */
int pspImageSavePngOpen(FILE *fp, const PspImage* image)
{
  unsigned char *bitmap;
  const unsigned short *pixel;
  int i, j, width, height;

  width = image->Viewport.Width;
  height = image->Viewport.Height;

  if (!(bitmap = (u8*)malloc(sizeof(u8) * width * height * 3)))
    return 0;

  pixel = image->Pixels + (image->Viewport.Y * image->Width);
  for (i = 0; i < height; i++)
  {
    /* Skip to the start of the viewport */
    pixel += image->Viewport.X;
    for (j = 0; j < width; j++, pixel++)
    {
      bitmap[i * width * 3 + j * 3 + 0] = RED(*pixel);
      bitmap[i * width * 3 + j * 3 + 1] = GREEN(*pixel);
      bitmap[i * width * 3 + j * 3 + 2] = BLUE(*pixel);
    }
    /* Skip to the end of the line */
    pixel += image->Width - (image->Viewport.X + width);
  }

  png_struct *pPngStruct = png_create_write_struct( PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL );

  if (!pPngStruct)
  {
    free(bitmap);
    return 0;
  }

  png_info *pPngInfo = png_create_info_struct( pPngStruct );
  if (!pPngInfo)
  {
    png_destroy_write_struct( &pPngStruct, NULL );
    free(bitmap);
    return 0;
  }

  png_byte **buf = (png_byte**)malloc(height * sizeof(png_byte*));
  if (!buf)
  {
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    free(bitmap);
    return 0;
  }

  unsigned int y;
  for (y = 0; y < height; y++)
    buf[y] = (byte*)&bitmap[y * width * 3];

  if (setjmp( pPngStruct->jmpbuf ))
  {
    free(buf);
    free(bitmap);
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    return 0;
  }

  png_init_io( pPngStruct, fp );
  png_set_IHDR( pPngStruct, pPngInfo, width, height, 8,
    PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT);
  png_write_info( pPngStruct, pPngInfo );
  png_write_image( pPngStruct, buf );
  png_write_end( pPngStruct, pPngInfo );

  png_destroy_write_struct( &pPngStruct, &pPngInfo );
  free(buf);
  free(bitmap);

  return 1;
}
