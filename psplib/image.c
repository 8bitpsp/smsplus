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

#include "video.h"
#include "image.h"

typedef unsigned char byte;

static void pspImageDrawHLine(PspImage *image, int sx, int dx, int y, unsigned short color);
static void pspImageDrawVLine(PspImage *image, int x, int sy, int dy, unsigned short color);

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

  return image;
}

void pspImageDestroy(PspImage *image)
{
  free(image->Pixels);
  free(image);
}

PspImage* pspImageCreateThumbnail(const PspImage *image)
{
  PspImage *thumb;
  int i, j, p;

  if (!(thumb = pspImageCreate(image->Width / 2, image->Height / 2)))
    return NULL;

  for (i = 0, p = 0; i < image->Height; i += 2)
    for (j = 0; j < image->Width; j += 2)
      thumb->Pixels[p++] = image->Pixels[(image->Width * i) + j];

  return thumb;  
}

PspImage* pspImageCreateCopy(const PspImage *image)
{
  PspImage *copy;

  /* Create image */
  if (!(copy = pspImageCreate(image->Width, image->Height)))
    return NULL;

  /* Copy pixels */
  int size = image->Width * image->Height * sizeof(unsigned short);
  memcpy(copy->Pixels, image->Pixels, size);

  return copy;
}

void pspImageClear(PspImage *image, unsigned short color)
{
  int i, size;
  size = image->Width * image->Height;

  for (i = 0; i < size; i++)
    image->Pixels[i] = color;
}

void pspImageDrawRect(PspImage *image, int sx, int sy, int dx, int dy, unsigned short color)
{
  pspImageDrawHLine(image, sx, dx, sy, color);
  pspImageDrawHLine(image, sx, dx, dy, color);
  pspImageDrawVLine(image, sx, sy, dy, color);
  pspImageDrawVLine(image, dx, sy, dy, color);
}

void pspImageFillRect(PspImage *image, int sx, int sy, int dx, int dy, unsigned short color)
{
  int minx, maxx, miny, maxy, i, j;
  unsigned short *pixel;

  /* Clamp coords */
  sx = (sx < 0) ? 0 : (sx >= image->Width) ? image->Width - 1 : sx;
  dx = (dx < 0) ? 0 : (dx >= image->Width) ? image->Width - 1 : dx;
  sy = (sy < 0) ? 0 : (sy >= image->Height) ? image->Height - 1 : sy;
  dy = (dy < 0) ? 0 : (dy >= image->Height) ? image->Height - 1 : dy;

  /* Determine smaller/larger */
  minx = (sx > dx) ? dx : sx;
  maxx = (sx < dx) ? dx : sx;
  miny = (sy > dy) ? dy : sy;
  maxy = (sy < dy) ? dy : sy;

  for (i = miny, pixel = image->Pixels + (miny * image->Width + minx); i <= maxy; i++, pixel += image->Width - (maxx - minx) - 1)
    for (j = minx; j <= maxx; j++, pixel++)
      *pixel = color;
}

static void pspImageDrawHLine(PspImage *image, int sx, int dx, int y, unsigned short color)
{
  int min, max, i;
  unsigned short *pixel;

  /* Clamp coords */
  sx = (sx < 0) ? 0 : (sx >= image->Width) ? image->Width - 1 : sx;
  dx = (dx < 0) ? 0 : (dx >= image->Width) ? image->Width - 1 : dx;
  y = (y < 0) ? 0 : (y >= image->Height) ? image->Height - 1 : y;

  /* Determine smaller/larger */
  min = (sx > dx) ? dx : sx;
  max = (sx < dx) ? dx : sx;

  for (i = min, pixel = image->Pixels + (y * image->Width + min); i <= max; i++, pixel++)
    *pixel = color;
}

static void pspImageDrawVLine(PspImage *image, int x, int sy, int dy, unsigned short color)
{
  int min, max, i;
  unsigned short *pixel;

  /* Clamp coords */
  sy = (sy < 0) ? 0 : (sy >= image->Height) ? image->Height - 1 : sy;
  dy = (dy < 0) ? 0 : (dy >= image->Height) ? image->Height - 1 : dy;
  x = (x < 0) ? 0 : (x >= image->Width) ? image->Width - 1 : x;

  /* Determine smaller/larger */
  min = (sy > dy) ? dy : sy;
  max = (sy < dy) ? dy : sy;

  for (i = min, pixel = image->Pixels + (min * image->Width + x); i <= max; i++, pixel += image->Width)
    *pixel = color;
}

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

  if (!(image = pspImageCreate(width, height)))
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

int pspImageSavePngOpen(FILE *fp, const PspImage* image)
{
  unsigned char *bitmap;
  const unsigned short *pixel;
  int i, j;

  if (!(bitmap = (unsigned char*)malloc(sizeof(unsigned char) * image->Width * image->Height * 3)))
    return 0;

  for (i = 0, pixel = image->Pixels; i < image->Height; i++)
  {
    for (j = 0; j < image->Width; j++, pixel++)
    {
      bitmap[i * image->Width * 3 + j * 3 + 0] = RED(*pixel);
      bitmap[i * image->Width * 3 + j * 3 + 1] = GREEN(*pixel);
      bitmap[i * image->Width * 3 + j * 3 + 2] = BLUE(*pixel);
    }
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

	png_byte **buf = (png_byte**)malloc(image->Height * sizeof(png_byte*));
	if (!buf)
	{
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
	  free(bitmap);
		return 0;
	}

	unsigned int y;
	for (y = 0; y < image->Height; y++)
		buf[y] = (byte*)&bitmap[y * image->Width * 3];

  if (setjmp( pPngStruct->jmpbuf ))
  {
    free(buf);
	  free(bitmap);
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    return 0;
  }

  png_init_io( pPngStruct, fp );
  png_set_IHDR( pPngStruct, pPngInfo, image->Width, image->Height, 8, 
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
