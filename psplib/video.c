/** PSP helper library ***************************************/
/**                                                         **/
/**                          video.c                        **/
/**                                                         **/
/** This file contains the video rendering library          **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/* TODO: move ScratchBuffer into VRAM */

#include <pspgu.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "video.h"

#define SLICE_SIZE 64

#define VRAM_START 0x04000000
#define VRAM_SIZE  0x00200000

const unsigned int PspFontColor[] = 
{
  0, /* Restore */
  PSP_VIDEO_BLACK,
  PSP_VIDEO_RED,
  PSP_VIDEO_GREEN,
  PSP_VIDEO_BLUE,
  PSP_VIDEO_GRAY,
  PSP_VIDEO_YELLOW,
  PSP_VIDEO_MAGENTA,
  PSP_VIDEO_WHITE
};

struct TexVertex
{
  unsigned short u, v;
  unsigned short color;
  short x, y, z;
};

static void *DisplayBuffer;
static void *DrawBuffer;
static int   PixelFormat;
static int   TexColor;
static void *VramOffset;
static void *VramChunkOffset;
//static unsigned short __attribute__((aligned(16))) ScratchBuffer[BUF_WIDTH * SCR_HEIGHT];
static void *ScratchBuffer;
static int ScratchBufferSize;
static unsigned int VramBufferOffset;
static unsigned int __attribute__((aligned(16))) List[262144]; /* TODO: ? */

static void* GetBuffer(const PspImage *image);
int _pspVideoPutChar(const PspFont *font, int sx, int sy, unsigned char sym, int color);

void pspVideoInit()
{
  PixelFormat = GU_PSM_5551;
  TexColor = GU_COLOR_5551;
  VramBufferOffset = 0;
  VramOffset = 0;
  VramChunkOffset = (void*)0x44088000;
  ScratchBufferSize = sizeof(unsigned short) * BUF_WIDTH * SCR_HEIGHT;
  ScratchBuffer = memalign(16, ScratchBufferSize);

  int size;

  // Initialize draw buffer

  size = 2 * BUF_WIDTH * SCR_HEIGHT;
  DrawBuffer = (void*)VramBufferOffset;
  VramBufferOffset += size;

  // Initialize display buffer

  size = 4 * BUF_WIDTH * SCR_HEIGHT;
  DisplayBuffer = (void*)VramBufferOffset;
  VramBufferOffset += size;

  // Initialize depth buffer

  size = 2 * BUF_WIDTH * SCR_HEIGHT;
  void *depth_buf = (void*)VramBufferOffset;
  VramBufferOffset += size;

  sceGuInit();
  sceGuStart(GU_DIRECT, List);
  sceGuDrawBuffer(PixelFormat, DrawBuffer, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, DisplayBuffer, BUF_WIDTH);
  sceGuDepthBuffer(depth_buf, BUF_WIDTH);
  sceGuDisable(GU_TEXTURE_2D);
  sceGuOffset(0, 0);
  sceGuViewport(SCR_WIDTH/2, SCR_HEIGHT/2, SCR_WIDTH, SCR_HEIGHT);
  sceGuDepthRange(0xc350, 0x2710);
  sceGuDisable(GU_ALPHA_TEST);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
  sceGuEnable(GU_BLEND);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuEnable(GU_CULL_FACE);
  sceGuDisable(GU_LIGHTING);
  sceGuFrontFace(GU_CW);
  sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
  sceGuAmbientColor(0xffffffff);
  sceGuFinish();
  sceGuSync(0,0);

  sceGuDisplay(GU_TRUE);
}

void* GetBuffer(const PspImage *image)
{
  int i, j, w, h;
  static int last_w = -1, last_h = -1, last_d = -1;
  int x_offset, x_skip, x_buf_skip;

  w = (image->Viewport.Width > BUF_WIDTH)
    ? BUF_WIDTH : image->Viewport.Width;
  h = (image->Viewport.Height > SCR_HEIGHT)
    ? SCR_HEIGHT : image->Viewport.Height;

  if (w != last_w || h != last_h)
    memset(ScratchBuffer, 0, ScratchBufferSize);

  x_offset = image->Viewport.X;
  x_skip = image->Width - (image->Viewport.X + image->Viewport.Width);
  x_buf_skip = BUF_WIDTH - w;

  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    unsigned char *img_ptr = image->Pixels
      + image->Viewport.Y * image->BytesPerPixel;
    unsigned char *buf_ptr = ScratchBuffer;

    for (i = 0; i < h; i++)
    {
      img_ptr += x_offset;
      for (j = 0; j < w; j++, img_ptr++, buf_ptr++)
        *buf_ptr = *img_ptr;
      buf_ptr += x_buf_skip;
      img_ptr += x_skip;
    }
  }
  else if (image->Depth == PSP_IMAGE_16BPP)
  {
    unsigned short *img_ptr = image->Pixels
      + image->Viewport.Y * image->BytesPerPixel;
    unsigned short *buf_ptr = ScratchBuffer;

    for (i = 0; i < h; i++)
    {
      img_ptr += x_offset;
      for (j = 0; j < w; j++, img_ptr++, buf_ptr++)
        *buf_ptr = *img_ptr;
      buf_ptr += x_buf_skip;
      img_ptr += x_skip;
    }
  }

  last_w = w;
  last_h = h;
  last_d = image->Depth;

  sceKernelDcacheWritebackAll();

  return ScratchBuffer;
}

void pspVideoBegin()
{
  sceGuStart(GU_DIRECT, List);
}

void pspVideoEnd()
{
  sceGuFinish();
  sceGuSync(0, 0);
}

void pspVideoPutImage(const PspImage *image, int dx, int dy, int dw, int dh)
{
  sceGuScissor(dx + 1, dy + 1, dx + dw - 2, dy + dh - 2);

  void *pixels;
  int width;

  if (image->PowerOfTwo)
  {
    pixels = image->Pixels;
    width = image->Width;
  }
  else
  {
    pixels = GetBuffer(image);
    width = BUF_WIDTH;
  }

  if (image->Depth != PSP_IMAGE_INDEXED &&
    dw == image->Viewport.Width && dh == image->Viewport.Height)
  {
    sceGuCopyImage(PixelFormat,
      image->Viewport.X, image->Viewport.Y,
      image->Viewport.Width, image->Viewport.Height,
      width, pixels, dx, dy,
      BUF_WIDTH, (void *)(VRAM_START + (u32)VramOffset));
  }
  else
  {
    sceGuEnable(GU_TEXTURE_2D);
    if (image->Depth == PSP_IMAGE_INDEXED)
    {
      sceGuClutMode(PixelFormat, 0, 0xff, 0);
      sceGuClutLoad(256 >> 3, image->Palette);
    }
    sceGuTexMode(image->TextureFormat, 0, 0, GU_FALSE);
    sceGuTexImage(0, width, width, width, pixels);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    struct TexVertex* vertices;
    int start, end, sc_end, slsz_scaled;
    slsz_scaled = ceil((float)dw * (float)SLICE_SIZE) / (float)image->Viewport.Width;

    start = image->Viewport.X;
    end = image->Viewport.X + image->Viewport.Width;
    sc_end = dx + dw;

    for (; start < end; start += SLICE_SIZE, dx += slsz_scaled)
    {
      vertices = (struct TexVertex*)sceGuGetMemory(2 * sizeof(struct TexVertex));

      vertices[0].u = start;
      vertices[0].v = image->Viewport.Y;
      vertices[1].u = start + SLICE_SIZE;
      vertices[1].v = image->Viewport.Height + image->Viewport.Y;

      vertices[0].x = dx; vertices[0].y = dy;
      vertices[1].x = dx + slsz_scaled; vertices[1].y = dy + dh;

      vertices[0].color
        = vertices[1].color
        = vertices[0].z 
        = vertices[1].z = 0;

      sceGuDrawArray(GU_SPRITES,
        GU_TEXTURE_16BIT | TexColor | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
        2, 0, vertices);
    }
    sceGuDisable(GU_TEXTURE_2D);
  }

  sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void pspVideoSwapBuffers()
{
  VramOffset = sceGuSwapBuffers();
}

void pspVideoShutdown()
{
  free(ScratchBuffer);
  sceGuTerm();
}

void pspVideoWaitVSync()
{
  sceDisplayWaitVblankStart();
}

void pspVideoDrawLine(int sx, int sy, int dx, int dy, u32 color)
{
  PspVertex *vert = (PspVertex*)sceGuGetMemory(sizeof(PspVertex) * 2);
  memset(vert, 0, sizeof(PspVertex) * 2);

  vert[0].x = sx; vert[0].y = sy; vert[0].color = color;
  vert[1].x = dx; vert[1].y = dy; vert[1].color = color;

  sceGuDrawArray(GU_LINES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, vert);
}

void pspVideoDrawRect(int sx, int sy, int dx, int dy, u32 color)
{
  PspVertex *vert = (PspVertex*)sceGuGetMemory(sizeof(PspVertex) * 5);
  memset(vert, 0, sizeof(PspVertex) * 5);

  vert[0].x=sx; vert[0].y=sy; vert[0].color = color;
  vert[1].x=sx; vert[1].y=dy; vert[1].color = color;
  vert[2].x=dx; vert[2].y=dy; vert[2].color = color;
  vert[3].x=dx; vert[3].y=sy; vert[3].color = color;
  vert[4].x=sx; vert[4].y=sy; vert[4].color = color;

  sceGuDrawArray(GU_LINE_STRIP, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 5, NULL, vert);
}

void pspVideoShadowRect(int sx, int sy, int dx, int dy, u32 color, int depth)
{
  int i;
  u32 alpha;
  color &= ~0xff000000;

  for (i = depth, alpha = 0x30000000; i > 0; i--, alpha += 0x20000000)
  {
    pspVideoDrawLine(sx + i, dy + i, dx + i, dy + i, color | alpha);
    pspVideoDrawLine(dx + i, sy + i, dx + i, dy + i + 1, color | alpha);
  }
}

void pspVideoGlowRect(int sx, int sy, int dx, int dy, u32 color, int radius)
{
  int i;
  u32 alpha;
  color &= ~0xff000000;

  for (i = radius, alpha = 0x30000000; i > 0; i--, alpha += 0x20000000)
    pspVideoDrawRect(sx - i, sy - i, dx + i, dy + i, color | alpha);
}

void pspVideoFillRect(int sx, int sy, int dx, int dy, u32 color)
{
  PspVertex *vert = (PspVertex*)sceGuGetMemory(4 * sizeof(PspVertex));
  memset(vert, 0, sizeof(PspVertex) * 4);

  vert[0].x = sx; vert[0].y = sy; vert[0].color = color;
  vert[1].x = dx; vert[1].y = sy; vert[1].color = color;
  vert[2].x = dx; vert[2].y = dy; vert[2].color = color;
  vert[3].x = sx; vert[3].y = dy; vert[3].color = color;

  sceGuDrawArray(GU_TRIANGLE_FAN, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 4, NULL, vert);
}

void pspVideoCallList(const void *list)
{
  sceGuCallList(list);
}

void pspVideoClearScreen()
{
  sceGuClear(GU_COLOR_BUFFER_BIT);
}

int _pspVideoPutChar(const PspFont *font, int sx, int sy, unsigned char sym, int color)
{
  /* Instead of a tab, skip 4 spaces */
  if (sym == (u8)'\t')
    return font->Chars[(int)' '].Width * 4;

  /* This function should be rewritten to write directly to VRAM, probably */
  int h, v, i, j, w, s;
  w = font->Chars[(int)sym].Width;
  h = font->Height;

  /* Allocate and clear enough memory to write the pixels of the char */
  s = sizeof(PspVertex) * w * h;
  PspVertex *vert = (PspVertex*)sceGuGetMemory(s);
  memset(vert, 0, s);

  /* Initialize pixel values */
  for (i = 0, v = 0; i < h; i++)
  {
    for (j = 0; j < w; j++)
    {
      if (font->Chars[(int)sym].Char[i] & (0x1 << (w-j)))
      {
        vert[v].x = sx + j; vert[v].y = sy + i; vert[v].color = color;
        v++;
      }
    }
  }

  /* Render the char as a set of pixels */
  sceGuDrawArray(GU_POINTS, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, v, NULL, vert);

  /* Return total width */
  return w;
}

int pspVideoPrint(const PspFont *font, int sx, int sy, const char *string, u32 color)
{
  return pspVideoPrintN(font, sx, sy, string, -1, color);
}

int pspVideoPrintCenter(const PspFont *font, int sx, int sy, int dx, const char *string, u32 color)
{
  const unsigned char *ch;
  int width, c = color, max;

  width = pspFontGetTextWidth(font, string);
  sx += (dx - sx) / 2 - width / 2;

  for (ch = (unsigned char*)string, width = 0, max = 0; *ch; ch++)
  {
    if (*ch < 32)
    {
      if (*ch >= PSP_VIDEO_FC_RESTORE && *ch <= PSP_VIDEO_FC_WHITE)
      {
        c = (*ch == PSP_VIDEO_FC_RESTORE) ? color : PspFontColor[(int)(*ch) - PSP_VIDEO_FC_RESTORE];
        continue;
      }
      else if (*ch == '\n')
      {
        sy += font->Height;
        width = 0;
        continue;
      }
    }

    width += _pspVideoPutChar(font, sx + width, sy, (u8)(*ch), c);
    if (width > max) max = width;
  }

  return max;
}

int pspVideoPrintN(const PspFont *font, int sx, int sy, const char *string, int count, u32 color)
{
  const unsigned char *ch;
  int width, i, c = color, max;

  for (ch = (unsigned char*)string, width = 0, i = 0, max = 0; *ch && (count < 0 || i < count); ch++, i++)
  {
    if (*ch < 32)
    {
      if (*ch >= PSP_VIDEO_FC_RESTORE && *ch <= PSP_VIDEO_FC_WHITE)
      {
        c = (*ch == PSP_VIDEO_FC_RESTORE) ? color : PspFontColor[(int)(*ch) - PSP_VIDEO_FC_RESTORE];
        continue;
      }
      else if (*ch == '\n')
      {
        sy += font->Height;
        width = 0;
        continue;
      }
    }

    width += _pspVideoPutChar(font, sx + width, sy, (u8)(*ch), c);
    if (width > max) max = width;
  }

  return max;
}

int pspVideoPrintClipped(const PspFont *font, int sx, int sy, const char* string, int max_w, char* clip, u32 color)
{
  int str_w = pspFontGetTextWidth(font, string);

  if (str_w <= max_w)
    return pspVideoPrint(font, sx, sy, string, color);

  int w, len;
  const char *ch;
  int clip_w = pspFontGetTextWidth(font, clip);

  for (ch=string, w=0, len=0; *ch && (w + clip_w < max_w); ch++, len++)
  {
    if (*ch == '\t') w += font->Chars[(u8)' '].Width * 4; 
    else w += font->Chars[(u8)(*ch)].Width;
  }

  w = pspVideoPrintN(font, sx, sy, string, len - 1, color);
  pspVideoPrint(font, sx + w, sy, clip, color);

  return w + clip_w;
}

PspImage* pspVideoGetVramBufferCopy()
{
  u16 *vram_addr = (u16*)((u8*)VRAM_START + 0x40000000);
  PspImage *image;

  if (!(image = pspImageCreate(SCR_WIDTH, SCR_HEIGHT, PSP_IMAGE_16BPP)))
    return NULL;

  int i, j;
  unsigned short *pixel;
  for (i = 0; i < image->Height; i++)
  {
    for (j = 0; j < image->Width; j++)
    {
      pixel = (unsigned short*)(image->Pixels + (i * image->Width + j));
      *pixel = *(vram_addr + (i * BUF_WIDTH + j));
    }
  }

  return image;
}

void* pspVideoAllocateVramChunk(unsigned int bytes)
{
  void *ptr = VramChunkOffset;
  VramChunkOffset += bytes;

  return ptr;
}
