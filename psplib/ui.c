/** PSP helper library ***************************************/
/**                                                         **/
/**                           ui.c                          **/
/**                                                         **/
/** This file contains a simple GUI rendering library       **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <psprtc.h>

#include "psp.h"
#include "fileio.h"
#include "ctrl.h"
#include "ui.h"

#define MAX_DIR_LEN 255

#define  CONTROL_BUTTON_MASK \
  (PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_CROSS | PSP_CTRL_SQUARE | \
   PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT | PSP_CTRL_START)

static const char 
  *AlertDialogButtonTemplate   = "\026\001\020/\026\002\020 Close",
  *ConfirmDialogButtonTemplate = "\026\001\020 Confirm\t\026\002\020 Cancel",

  *BrowserCancelTemplate    = "\026\002\020 Cancel",
  *BrowserEnterDirTemplate  = "\026\001\020 Enter directory",
  *BrowserOpenTemplate      = "\026\001\020 Open",
  *BrowserParentDirTemplate = "\026\243\020 Parent directory",

  *SplashStatusBarTemplate  = "\026\255\020/\026\256\020 Switch tabs",

  *OptionModeTemplate = 
    "\026\245\020/\026\246\020 Select\t\026\247\020/\026\002\020 Cancel\t\026\250\020/\026\001\020 Confirm";

struct UiPos
{
  int Index;
  int Offset;
  const PspMenuItem *Top;
};

void _pspUiReplaceIcons(char *string);

void _pspUiReplaceIcons(char *string)
{
  char *ch;

  for (ch = string; *ch; ch++)
  {
    switch(*ch)
    {
    case '\001': *ch = pspUiGetButtonIcon(UiMetric.OkButton); break;
    case '\002': *ch = pspUiGetButtonIcon(UiMetric.CancelButton); break;
    }
  }
}

char pspUiGetButtonIcon(u32 button_mask)
{
  switch (button_mask)
  {
  case PSP_CTRL_CROSS:    return *PSP_FONT_CROSS;
  case PSP_CTRL_CIRCLE:   return *PSP_FONT_CIRCLE;
  case PSP_CTRL_TRIANGLE: return *PSP_FONT_TRIANGLE;
  case PSP_CTRL_SQUARE:   return *PSP_FONT_SQUARE;
  default:                return '?';
  }
}

void pspUiAlert(const char *message)
{
  int sx, sy, dx, dy, th, fh, mw, cw, w, h;
  char *instr = strdup(AlertDialogButtonTemplate);
  _pspUiReplaceIcons(instr);

  mw = pspFontGetTextWidth(UiMetric.Font, message);
  cw = pspFontGetTextWidth(UiMetric.Font, instr);
  fh = pspFontGetLineHeight(UiMetric.Font);
  th = pspFontGetTextHeight(UiMetric.Font, message);

  w = ((mw > cw) ? mw : cw) + 50;
  h = th + fh * 3;
  sx = SCR_WIDTH / 2 - w / 2;
  sy = SCR_HEIGHT / 2 - h / 2;
  dx = sx + w;
  dy = sy + h;

  pspVideoBegin();

  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, UiMetric.DialogFogColor);

  pspVideoFillRect(sx, sy, dx, dy, UiMetric.DialogBgColor);
  pspVideoDrawRect(sx + 1, sy + 1, dx - 1, dy - 1, UiMetric.DialogBorderColor);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - mw / 2, sy + fh * 0.5, message, 
    UiMetric.TextColor);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - cw / 2, dy - fh * 1.5, instr, 
    UiMetric.TextColor);

  free(instr);

  pspVideoEnd();

  /* Swap buffers */
  sceKernelDcacheWritebackAll();
  pspVideoWaitVSync();
  pspVideoSwapBuffers();

  SceCtrlData pad;

  /* Loop until X or O is pressed */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    if (pad.Buttons & UiMetric.OkButton || pad.Buttons & UiMetric.CancelButton)
      break;
  }
}

int pspUiConfirm(const char *message)
{
  int sx, sy, dx, dy, fh, mw, cw, w, h;
  char *instr = strdup(ConfirmDialogButtonTemplate);
  _pspUiReplaceIcons(instr);

  mw = pspFontGetTextWidth(UiMetric.Font, message);
  cw = pspFontGetTextWidth(UiMetric.Font, instr);
  fh = pspFontGetLineHeight(UiMetric.Font);

  w = ((mw > cw) ? mw : cw) + 50;
  h = fh * 4;
  sx = SCR_WIDTH / 2 - w / 2;
  sy = SCR_HEIGHT / 2 - h / 2;
  dx = sx + w;
  dy = sy + h;

  pspVideoBegin();

  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, UiMetric.DialogFogColor);

  pspVideoFillRect(sx, sy, dx, dy, UiMetric.DialogBgColor);
  pspVideoDrawRect(sx + 1, sy + 1, dx - 2, dy - 2, UiMetric.DialogBorderColor);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - mw / 2, sy + fh * 0.5, message, UiMetric.TextColor);

  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - cw / 2, sy + fh * 2.5, instr, UiMetric.TextColor);
  free(instr);

  pspVideoEnd();

  /* Swap buffers */
  sceKernelDcacheWritebackAll();
  pspVideoWaitVSync();
  pspVideoSwapBuffers();

  SceCtrlData pad;

  /* Loop until X or O is pressed */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    if (pad.Buttons & UiMetric.OkButton || pad.Buttons & UiMetric.CancelButton)
      break;
  }

  return pad.Buttons & UiMetric.OkButton;
}

void pspUiFlashMessage(const char *message)
{
  int sx, sy, dx, dy, fh, mw, mh, w, h;

  mw = pspFontGetTextWidth(UiMetric.Font, message);
  fh = pspFontGetLineHeight(UiMetric.Font);
  mh = pspFontGetTextHeight(UiMetric.Font, message);

  w = mw + 50;
  h = mh + fh * 2;
  sx = SCR_WIDTH / 2 - w / 2;
  sy = SCR_HEIGHT / 2 - h / 2;
  dx = sx + w;
  dy = sy + h;

  pspVideoBegin();

  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, UiMetric.DialogFogColor);
  pspVideoFillRect(sx, sy, dx, dy, UiMetric.DialogBgColor);
  pspVideoDrawRect(sx + 1, sy + 1, dx - 1, dy - 1, UiMetric.DialogBorderColor);
  pspVideoPrintCenter(UiMetric.Font,
    sx, sy + fh, dx, message, UiMetric.TextColor);

  pspVideoEnd();

  /* Swap buffers */
  sceKernelDcacheWritebackAll();
  pspVideoWaitVSync();
  pspVideoSwapBuffers();
}

void pspUiOpenBrowser(PspUiFileBrowser *browser, const char *start_path)
{
  PspMenu *menu;
  PspFile *file;
  PspFileList *list;
  const PspMenuItem *sel;
  const PspMenuItem *item;
  SceCtrlData pad;
  char *instr;

  if (!start_path)
    start_path = pspGetAppDirectory();

  char *cur_path = pspFileIoGetParentDirectory(start_path);
  const char *cur_file = pspFileIoGetFilename(start_path);
  struct UiPos pos;
  int lnmax, lnhalf, instr_len;
  int sby, sbh, i, j, h, w, fh = pspFontGetLineHeight(UiMetric.Font);
  int sx, sy, dx, dy;
  int hasparent, is_dir;

  sx = UiMetric.Left;
  sy = UiMetric.Top + fh + UiMetric.TitlePadding;
  dx = UiMetric.Right;
  dy = UiMetric.Bottom;
  w = dx - sx - UiMetric.ScrollbarWidth;
  h = dy - sy;

  menu = pspMenuCreate();

  /* Compute max. space needed for instructions */
  instr_len = strlen(BrowserCancelTemplate) + 1;
  instr_len += 1 
    + ((strlen(BrowserEnterDirTemplate) > strlen(BrowserOpenTemplate)) 
      ? strlen(BrowserEnterDirTemplate) : strlen(BrowserOpenTemplate));
  instr_len += 1 + strlen(BrowserParentDirTemplate);
  instr = (char*)malloc(sizeof(char) * instr_len);


  u32 ticks_per_sec, ticks_per_upd;
  u64 current_tick, last_tick;

  /* Begin browsing (outer) loop */
  while (!ExitPSP)
  {
    sel = NULL;
    pos.Top = NULL;
    pspMenuClear(menu);

    /* Load list of files for the selected path */
    if ((list = pspFileIoGetFileList(cur_path, browser->Filter)))
    {
      /* Check for a parent path, prepend .. if necessary */
      if ((hasparent=!pspFileIoIsRootDirectory(cur_path)))
        pspMenuAppendItem(menu, "..", (void*)PSP_FILEIO_DIR);

      /* Add a menu item for each file */
      for (file = list->First; file; file = file->Next)
      {
        /* Skip files that begin with '.' */
        if (file->Name && file->Name[0] == '.')
          continue;

        item = pspMenuAppendItem(menu, file->Name, (void*)file->Attrs);
        if (cur_file && strcmp(file->Name, cur_file) == 0)
          sel = item;
      }

      cur_file = NULL;

      /* Destroy the file list */
      pspFileIoDestroyFileList(list);
    }
    else
    {
      /* Check for a parent path, prepend .. if necessary */
      if ((hasparent=!pspFileIoIsRootDirectory(cur_path)))
        pspMenuAppendItem(menu, "..", (void*)PSP_FILEIO_DIR);
    }

    /* Initialize variables */
    lnmax = (dy - sy) / fh;
    lnhalf = lnmax >> 1;
    sbh = (menu->Count > lnmax) ? (int)((float)h * ((float)lnmax / (float)menu->Count)) : 0;

    pos.Index = pos.Offset = 0;

    if (!sel) 
    { 
      /* Select the first file/dir in the directory */
      if (menu->First && menu->First->Next)
        sel=menu->First->Next;
      else if (menu->First)
        sel=menu->First;
    }

    /* Compute index and offset of selected file */
    if (sel)
    {
      pos.Top=menu->First;
      for (item=menu->First; item != sel; item=item->Next)
        if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top=pos.Top->Next; } else pos.Index++;
    }

    pspVideoWaitVSync();

    /* Compute update frequency */
    ticks_per_sec = sceRtcGetTickResolution();
    sceRtcGetCurrentTick(&last_tick);
    ticks_per_upd = ticks_per_sec / UiMetric.MenuFps;

    /* Begin navigation (inner) loop */
    while (!ExitPSP)
    {
      if (!pspCtrlPollControls(&pad))
        continue;

      /* Check the directional buttons */
      if (sel)
      {
        if ((pad.Buttons & PSP_CTRL_DOWN || pad.Buttons & PSP_CTRL_ANALDOWN) && sel->Next)
        {
          if (pos.Index+1 >= lnmax) { pos.Offset++; pos.Top=pos.Top->Next; } 
          else pos.Index++;
          sel=sel->Next;
        }
        else if ((pad.Buttons & PSP_CTRL_UP || pad.Buttons & PSP_CTRL_ANALUP) && sel->Prev)
        {
          if (pos.Index - 1 < 0) { pos.Offset--; pos.Top=pos.Top->Prev; }
          else pos.Index--;
          sel = sel->Prev;
        }
        else if (pad.Buttons & PSP_CTRL_LEFT)
        {
          for (i=0; sel->Prev && i < lnhalf; i++)
          {
            if (pos.Index-1 < 0) { pos.Offset--; pos.Top=pos.Top->Prev; }
            else pos.Index--;
            sel=sel->Prev;
          }
        }
        else if (pad.Buttons & PSP_CTRL_RIGHT)
        {
          for (i=0; sel->Next && i < lnhalf; i++)
          {
            if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top=pos.Top->Next; }
            else pos.Index++;
            sel=sel->Next;
          }
        }

        /* File/dir selection */
        if (pad.Buttons & UiMetric.OkButton)
        {
          if (((unsigned int)sel->Userdata & PSP_FILEIO_DIR))
          {
            /* Selected a directory, descend */
            pspFileIoEnterDirectory(&cur_path, sel->Caption);
            break;
          }
          else
          {
            int exit = 1;

            /* Selected a file */
            if (browser->OnOk)
            {
              char *file = malloc((strlen(cur_path) + strlen(sel->Caption) + 1) * sizeof(char));
              sprintf(file, "%s%s", cur_path, sel->Caption);
              exit = browser->OnOk(browser, file);
              free(file);
            }

            if (exit) goto exit_browser;
            else continue;
          }
        }
      }

      if (pad.Buttons & PSP_CTRL_TRIANGLE)
      {
        if (!pspFileIoIsRootDirectory(cur_path))
        {
          pspFileIoEnterDirectory(&cur_path, "..");
          break;
        }
      }
      else if (pad.Buttons & UiMetric.CancelButton)
      {
        if (browser->OnCancel)
          browser->OnCancel(browser, cur_path);
        goto exit_browser;
      }
      else if ((pad.Buttons & CONTROL_BUTTON_MASK) && browser->OnButtonPress)
      {
        char *file = NULL;
        int exit;

        if (sel)
        {
          file = malloc((strlen(cur_path) + strlen(sel->Caption) + 1) * sizeof(char));
          sprintf(file, "%s%s", cur_path, sel->Caption);
        }

        exit = browser->OnButtonPress(browser, 
          file, pad.Buttons & CONTROL_BUTTON_MASK);

        if (file) free(file);
        if (exit) goto exit_browser;
      }

      is_dir = (unsigned int)sel->Userdata & PSP_FILEIO_DIR;

      pspVideoBegin();

      /* Clear screen */
      if (UiMetric.Background) 
        pspVideoPutImage(UiMetric.Background, 0, 0, UiMetric.Background->Width, 
          UiMetric.Background->Height);
      else pspVideoClearScreen();

      /* Draw current path */
      pspVideoPrint(UiMetric.Font, sx, UiMetric.Top, cur_path, 
        UiMetric.TitleColor);
      pspVideoDrawLine(UiMetric.Left, UiMetric.Top + fh - 1, UiMetric.Left + w, 
        UiMetric.Top + fh - 1, UiMetric.TitleColor);

      /* Draw instructions */
      strcpy(instr, BrowserCancelTemplate);
      if (sel)
      {
        strcat(instr, "\t");
        strcat(instr, (is_dir) ? BrowserEnterDirTemplate : BrowserOpenTemplate);
      }
      if (hasparent)
      {
        strcat(instr, "\t");
        strcat(instr, BrowserParentDirTemplate);
      }
      _pspUiReplaceIcons(instr);
      pspVideoPrintCenter(UiMetric.Font, 
        sx, SCR_HEIGHT - fh, dx, instr, UiMetric.StatusBarColor);

      /* Draw scrollbar */
      if (sbh > 0)
      {
        sby = sy + (int)((float)(h - sbh) * ((float)(pos.Offset + pos.Index) / (float)menu->Count));
        pspVideoFillRect(dx - UiMetric.ScrollbarWidth, sy, dx, dy, UiMetric.ScrollbarBgColor);
        pspVideoFillRect(dx - UiMetric.ScrollbarWidth, sby, dx, sby + sbh, UiMetric.ScrollbarColor);
      }

      /* Render the files */
      for (item = pos.Top, i = 0, j = sy; item && i < lnmax; item = item->Next, j += fh, i++)
      {
        if (item == sel)
          pspVideoFillRect(sx, j, sx + w, j + fh, UiMetric.SelectedBgColor);

        pspVideoPrintClipped(UiMetric.Font, sx + 10, j, item->Caption, w - 10, "...", 
          (item == sel) ? UiMetric.SelectedColor :
            ((unsigned int)item->Userdata & PSP_FILEIO_DIR) 
              ? UiMetric.BrowserDirectoryColor : UiMetric.BrowserFileColor);
      }

      /* Perform any custom drawing */
      if (browser->OnRender)
        browser->OnRender(browser, "not implemented");

      pspVideoEnd();

      /* Wait if needed */
      do { sceRtcGetCurrentTick(&current_tick); }
      while (current_tick - last_tick < ticks_per_upd);
      last_tick = current_tick;

      /* Swap buffers */
      sceKernelDcacheWritebackAll();
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
    }
  }

exit_browser:

  pspMenuDestroy(menu);

  free(cur_path);
  free(instr);
}

void pspUiOpenGallery(const PspUiGallery *gallery, const char *title)
{
  PspMenu *menu = gallery->Menu;
  const PspMenuItem *top, *item;
  SceCtrlData pad;
  PspMenuItem *sel = menu->Selected;

  int sx, sy, dx, dy, x,
    orig_w = 272, orig_h = 228, // defaults
    fh, c, i, j,
    sbh, sby,
    w, h, 
    icon_w, icon_h, 
    grid_w, grid_h,
    icon_idx, icon_off,
    rows, vis_v, vis_s,
    icons;

  /* Find first icon and save its width/height */
  for (item = menu->First; item; item = item->Next)
  {
    if (item->Icon)
    {
      orig_w = ((PspImage*)item->Icon)->Width;
      orig_h = ((PspImage*)item->Icon)->Height;
      break;
    }
  }

  fh = pspFontGetLineHeight(UiMetric.Font);
  sx = UiMetric.Left;
  sy = UiMetric.Top + ((title) ? fh + UiMetric.TitlePadding : 0);
  dx = UiMetric.Right;
  dy = UiMetric.Bottom;
  w = (dx - sx) - UiMetric.ScrollbarWidth; // visible width
  h = dy - sy; // visible height
  icon_w = (w - UiMetric.GalleryIconMarginWidth 
    * (UiMetric.GalleryIconsPerRow - 1)) / UiMetric.GalleryIconsPerRow; // icon width
  icon_h = (int)((float)icon_w 
    / ((float)orig_w / (float)orig_h)); // icon height
  grid_w = icon_w + UiMetric.GalleryIconMarginWidth; // width of the grid
  grid_h = icon_h + (fh * 2); // half-space for margin + 1 line of text 
  icons = menu->Count; // number of icons total
  rows = ceil((float)icons / (float)UiMetric.GalleryIconsPerRow); // number of rows total
  vis_v = h / grid_h; // number of rows visible at any time
  vis_s = UiMetric.GalleryIconsPerRow * vis_v; // max. number of icons visible on screen at any time

  icon_idx = 0;
  icon_off = 0;
  top = menu->First;

  if (!sel)
  {
    /* Select the first icon */
    sel = menu->First;
  }
  else
  {
    /* Find the selected icon */
    for (item = menu->First; item; item = item->Next)
    {
      if (item == sel) 
        break;

      if (++icon_idx >= vis_s)
      {
        icon_idx=0;
        icon_off += vis_s;
        top = item;
      }
    }
    
    if (item != sel)
    {
      /* Icon not found; reset to first icon */
      sel = menu->First;
      top = menu->First;
      icon_idx = 0;
      icon_off = 0;
    }
  }

  /* Compute height of scrollbar */
  sbh = ((float)vis_v / (float)(rows + (rows % vis_v))) * (float)h;

  int glow = 2, glow_dir = 1;

  pspVideoWaitVSync();

  /* Compute update frequency */
  u32 ticks_per_sec, ticks_per_upd;
  u64 current_tick, last_tick;

  ticks_per_sec = sceRtcGetTickResolution();
  sceRtcGetCurrentTick(&last_tick);
  ticks_per_upd = ticks_per_sec / UiMetric.MenuFps;

  /* Begin navigation loop */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    /* Check the directional buttons */
    if (sel)
    {
      if (pad.Buttons & PSP_CTRL_RIGHT && sel->Next)
      {
        sel = sel->Next;
        if (++icon_idx >= vis_s)
        {
          icon_idx = 0;
          icon_off += vis_s;
          top = sel;
        }
      }
      else if (pad.Buttons & PSP_CTRL_LEFT && sel->Prev)
      {
        sel = sel->Prev;

        if (--icon_idx < 0)
        {
          icon_idx = vis_s-1;
          icon_off -= vis_s;
          for (i = 0; i < vis_s && top; i++) top = top->Prev;
        }
      }
      else if (pad.Buttons & PSP_CTRL_DOWN)
      {
        for (i = 0; sel->Next && i < UiMetric.GalleryIconsPerRow; i++)
        {
          sel = sel->Next;

          if (++icon_idx >= vis_s)
          {
            icon_idx = 0;
            icon_off += vis_s;
            top = sel;
          }
        }
      }
      else if (pad.Buttons & PSP_CTRL_UP)
      {
        for (i = 0; sel->Prev && i < UiMetric.GalleryIconsPerRow; i++)
        {
          sel = sel->Prev;

          if (--icon_idx < 0)
          {
            icon_idx = vis_s-1;
            icon_off -= vis_s;
            for (j = 0; j < vis_s && top; j++) top = top->Prev;
          }
        }
      }

      if (pad.Buttons & UiMetric.OkButton)
      {
        pad.Buttons &= ~UiMetric.OkButton;
        if (!gallery->OnOk || gallery->OnOk(gallery, sel))
          break;
      }
    }

    if (pad.Buttons & UiMetric.CancelButton)
    {
      pad.Buttons &= ~UiMetric.CancelButton;
      if (gallery->OnCancel)
        gallery->OnCancel(gallery, sel);
      break;
    }

    if ((pad.Buttons & CONTROL_BUTTON_MASK) && gallery->OnButtonPress)
    {
      if (gallery->OnButtonPress(gallery, sel, pad.Buttons & CONTROL_BUTTON_MASK))
          break;
    }

    pspVideoBegin();

    /* Clear screen */
    if (UiMetric.Background) 
      pspVideoPutImage(UiMetric.Background, 0, 0, 
        UiMetric.Background->Width, UiMetric.Background->Height);
    else 
      pspVideoClearScreen();

    /* Draw title */
    if (title)
    {
      pspVideoPrint(UiMetric.Font, UiMetric.Left, UiMetric.Top, 
        title, UiMetric.TitleColor);
      pspVideoDrawLine(UiMetric.Left, UiMetric.Top + fh - 1, UiMetric.Left + w, 
        UiMetric.Top + fh - 1, UiMetric.TitleColor);
    }

    /* Draw scrollbar */
    if (sbh < h)
    {
      sby = sy + (((float)icon_off / (float)UiMetric.GalleryIconsPerRow) 
        / (float)(rows + (rows % vis_v))) * (float)h;
      pspVideoFillRect(dx - UiMetric.ScrollbarWidth, 
        sy, dx, dy, UiMetric.ScrollbarBgColor);
      pspVideoFillRect(dx - UiMetric.ScrollbarWidth, 
        sby, dx, sby+sbh, UiMetric.ScrollbarColor);
    }

    /* Draw instructions */
    if (sel && sel->HelpText)
    {
      static char help_copy[MAX_DIR_LEN];
      strncpy(help_copy, sel->HelpText, MAX_DIR_LEN);
      help_copy[MAX_DIR_LEN - 1] = '\0';
      _pspUiReplaceIcons(help_copy);

      pspVideoPrintCenter(UiMetric.Font, 
        0, SCR_HEIGHT - fh, SCR_WIDTH, help_copy, UiMetric.StatusBarColor);
    }

    /* Render the menu items */
    for (i = sy, item = top; item && i + grid_h < dy; i += grid_h)
    {
      for (j = sx, c = 0; item && c < UiMetric.GalleryIconsPerRow; j += grid_w, c++, item = item->Next)
      {
        if (item->Icon)
        {
          if (item == sel)
            pspVideoPutImage((PspImage*)item->Icon, j - 4, i - 4, icon_w + 8, icon_h + 8);
          else
            pspVideoPutImage((PspImage*)item->Icon, j, i, icon_w, icon_h);
        }

        if (item == sel)
        {
          pspVideoDrawRect(j - 4, i - 4, 
            j + icon_w + 4, i + icon_h + 4, UiMetric.TextColor);
          glow += glow_dir;
          if (glow >= 5 || glow <= 2) glow_dir *= -1;
          pspVideoGlowRect(j - 4, i - 4, 
            j + icon_w + 4, i + icon_h + 4, UiMetric.SelectedColor, glow);
        }
        else 
        {
          pspVideoShadowRect(j, i, j + icon_w, i + icon_h, PSP_VIDEO_BLACK, 3);
          pspVideoDrawRect(j, i, j + icon_w, i + icon_h, UiMetric.TextColor);
        }

        if (item->Caption)
        {
          x = j + icon_w / 2 
            - pspFontGetTextWidth(UiMetric.Font, item->Caption) / 2;
          pspVideoPrint(UiMetric.Font, x, 
            i + icon_h + (fh / 2), item->Caption, 
            (item == sel) ? UiMetric.SelectedColor : UiMetric.TextColor);
        }
      }
    }

    /* Perform any custom drawing */
    if (gallery->OnRender)
      gallery->OnRender(gallery, sel);

    pspVideoEnd();

    /* Wait if needed */
    do { sceRtcGetCurrentTick(&current_tick); }
    while (current_tick - last_tick < ticks_per_upd);
    last_tick = current_tick;

    /* Swap buffers */
    sceKernelDcacheWritebackAll();
    pspVideoWaitVSync();
    pspVideoSwapBuffers();
  }

  menu->Selected = sel;
}

void pspUiOpenMenu(const PspUiMenu *uimenu, const char *title)
{
  struct UiPos pos;
  PspMenu *menu = uimenu->Menu;
  const PspMenuItem *item;
  SceCtrlData pad;
  const PspMenuOption *temp_option;
  int lnmax;
  int sby, sbh, i, j, k, h, w, fh = pspFontGetLineHeight(UiMetric.Font);
  int sx, sy, dx, dy;
  int max_item_w = 0, item_w;
  int option_mode, max_option_w = 0;
  int arrow_w = pspFontGetTextWidth(UiMetric.Font, "\272");
  int anim_frame = 0, anim_incr = 1;
  PspMenuItem *sel = menu->Selected;

  sx = UiMetric.Left;
  sy = UiMetric.Top + ((title) ? (fh + UiMetric.TitlePadding) : 0);
  dx = UiMetric.Right;
  dy = UiMetric.Bottom;
  w = dx - sx - UiMetric.ScrollbarWidth;
  h = dy - sy;

  /* Determine width of the longest caption */
  for (item = menu->First; item; item = item->Next)
  {
    if (item->Caption)
    {
      item_w = pspFontGetTextWidth(UiMetric.Font, item->Caption);
      if (item_w > max_item_w) 
        max_item_w = item_w;
    }
  }

  /* Initialize variables */
  lnmax = (dy - sy) / fh;
  sbh = (menu->Count > lnmax) ? (int)((float)h * ((float)lnmax / (float)menu->Count)) : 0;

  pos.Index = 0;
  pos.Offset = 0;
  pos.Top = NULL;
  option_mode = 0;
  temp_option = NULL;

  /* Find first selectable item */
  if (!sel)
  {
    for (sel = menu->First; sel; sel = sel->Next)
      if (sel->Caption && sel->Caption[0] != '\t')
        break;
  }

  /* Compute index and offset of selected file */
  pos.Top = menu->First;
  for (item = menu->First; item != sel; item = item->Next)
  {
    if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top = pos.Top->Next; } 
    else pos.Index++;
  }

  pspVideoWaitVSync();
  PspMenuItem *last;
  struct UiPos last_valid;

  /* Compute update frequency */
  u32 ticks_per_sec, ticks_per_upd;
  u64 current_tick, last_tick;

  ticks_per_sec = sceRtcGetTickResolution();
  sceRtcGetCurrentTick(&last_tick);
  ticks_per_upd = ticks_per_sec / UiMetric.MenuFps;

  /* Begin navigation loop */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    anim_frame += anim_incr;
    if (anim_frame > 2 || anim_frame < 0) 
      anim_incr *= -1;

    /* Check the directional buttons */
    if (sel)
    {
      if (pad.Buttons & PSP_CTRL_DOWN || pad.Buttons & PSP_CTRL_ANALDOWN)
      {
        if (option_mode)
        {
          if (temp_option->Next)
            temp_option = temp_option->Next;
        }
        else
        {
          if (sel->Next)
          {
            last = sel;
            last_valid = pos;

            for (;;)
            {
              if (pos.Index + 1 >= lnmax)
              {
                pos.Offset++;
                pos.Top = pos.Top->Next;
              }
              else pos.Index++;

              sel = sel->Next;

              if (!sel)
              {
                sel = last;
                pos = last_valid;
                break;
              }

              if (sel->Caption && sel->Caption[0] != '\t')
                break;
            }
          }
        }
      }
      else if (pad.Buttons & PSP_CTRL_UP || pad.Buttons & PSP_CTRL_ANALUP)
      {
        if (option_mode)
        {
          if (temp_option->Prev)
            temp_option = temp_option->Prev;
        }
        else
        {
          if (sel->Prev)
          {
            last = sel;
            last_valid = pos;

            for (;;)
            {
              if (pos.Index - 1 < 0)
              {
                pos.Offset--;
                pos.Top = pos.Top->Prev;
              }
              else pos.Index--;

              sel = sel->Prev;

              if (!sel)
              {
                sel = last;
                pos = last_valid;
                break;
              }

              if (sel->Caption && sel->Caption[0] != '\t')
                break;
            }
          }
        }
      }

      if (option_mode)
      {
        if (pad.Buttons & PSP_CTRL_RIGHT || pad.Buttons & UiMetric.OkButton)
        {
          option_mode = 0;

          /* If the callback function refuses the change, restore selection */
          if (!uimenu->OnItemChanged || uimenu->OnItemChanged(uimenu, sel, temp_option))
            sel->Selected = temp_option;
        }
        else if (pad.Buttons & PSP_CTRL_LEFT  || pad.Buttons & UiMetric.CancelButton)
        {
          option_mode = 0;
          if (pad.Buttons & UiMetric.CancelButton)
            pad.Buttons &= ~UiMetric.CancelButton;
        }
      }
      else
      {
        if (pad.Buttons & PSP_CTRL_RIGHT)
        {
          if (sel->Options && sel->Options->Next)
          {
            option_mode = 1;
            max_option_w = 0;
            int width;
            PspMenuOption *option;

            /* Find the longest option caption */
            for (option = sel->Options; option; option = option->Next)
              if (option->Text && (width = pspFontGetTextWidth(UiMetric.Font, option->Text)) > max_option_w) 
                max_option_w = width;

            temp_option = (sel->Selected) ? sel->Selected : sel->Options;
          }
        }
        else if (pad.Buttons & UiMetric.OkButton)
        {
          if (!uimenu->OnOk || uimenu->OnOk(uimenu, sel))
            break;
        }
      }
    }

    if (!option_mode)
    {
      if (pad.Buttons & UiMetric.CancelButton)
      {
        if (uimenu->OnCancel) 
          uimenu->OnCancel(uimenu, sel);
        break;
      }

      if ((pad.Buttons & CONTROL_BUTTON_MASK) && uimenu->OnButtonPress)
      {
        if (uimenu->OnButtonPress(uimenu, sel, pad.Buttons & CONTROL_BUTTON_MASK))
            break;
      }
    }

    pspVideoBegin();

    /* Clear screen */
    if (UiMetric.Background) 
      pspVideoPutImage(UiMetric.Background, 0, 0, 
        UiMetric.Background->Width, UiMetric.Background->Height);
    else 
      pspVideoClearScreen();

    /* Draw instructions */
    if (sel)
    {
      const char *dirs = NULL;

      if (!option_mode && sel->HelpText)
      {
        static char help_copy[MAX_DIR_LEN];
        strncpy(help_copy, sel->HelpText, MAX_DIR_LEN);
        help_copy[MAX_DIR_LEN - 1] = '\0';
       _pspUiReplaceIcons(help_copy);

        dirs = help_copy;
      }
      else if (option_mode)
      {
        static char help_copy[MAX_DIR_LEN];
        strncpy(help_copy, OptionModeTemplate, MAX_DIR_LEN);
        help_copy[MAX_DIR_LEN - 1] = '\0';
       _pspUiReplaceIcons(help_copy);

        dirs = help_copy;
      }

      if (dirs) 
        pspVideoPrintCenter(UiMetric.Font, 
          0, SCR_HEIGHT - fh, SCR_WIDTH, dirs, UiMetric.StatusBarColor);
    }

    /* Draw title */
    if (title)
    {
      pspVideoPrint(UiMetric.Font, UiMetric.Left, UiMetric.Top, 
        title, UiMetric.TitleColor);
      pspVideoDrawLine(UiMetric.Left, UiMetric.Top + fh - 1, UiMetric.Left + w, 
        UiMetric.Top + fh - 1, UiMetric.TitleColor);
    }

    /* Render the menu items */
    for (item = pos.Top, i = 0, j = sy; item && i < lnmax; item = item->Next, j += fh, i++)
    {
      if (item->Caption)
      {
  	    /* Section header */
  	    if (item->Caption[0] == '\t')
    		{
    		  // if (i != 0) j += fh / 2;
          pspVideoPrint(UiMetric.Font, sx, j, item->Caption + 1, UiMetric.TitleColor);
          pspVideoDrawLine(sx, j + fh - 1, sx + w, j + fh - 1, UiMetric.TitleColor);
    		  continue;
    		}

        /* Selected item background */
        if (item == sel)
          pspVideoFillRect(sx, j, sx + w, j + fh, UiMetric.SelectedBgColor);

        /* Item caption */
        pspVideoPrint(UiMetric.Font, 
          sx + 10, j, 
          item->Caption, 
          (item == sel) ? UiMetric.SelectedColor : UiMetric.TextColor);

        /* Selected option for the item */
        if (item->Selected)
        {
          k = sx + max_item_w + UiMetric.MenuItemMargin + 10;
          k += pspVideoPrint(UiMetric.Font, 
            k, j, 
            item->Selected->Text, 
            (item == sel) ? UiMetric.SelectedColor : UiMetric.TextColor);

          if (!option_mode && item == sel)
          {
            if (sel->Options && sel->Options->Next)
              pspVideoPrint(UiMetric.Font, k + anim_frame, j, " >", UiMetric.MenuDecorColor);
          }
        }
      }
    }

    /* Render menu options */
    if (option_mode)
    {
      /* Render selected item + previous items */
      i = sy + pos.Index * fh;
      k = sx + max_item_w + UiMetric.MenuItemMargin + 10;
      int arrow_x = k - UiMetric.MenuItemMargin + (UiMetric.MenuItemMargin / 2 - arrow_w / 2);
      const PspMenuOption *option;

      for (option = temp_option; option && i >= sy; option = option->Prev, i -= fh)
      {
        pspVideoFillRect(k - UiMetric.MenuItemMargin, i, k + max_option_w + UiMetric.MenuItemMargin, i + fh, 
          (option == temp_option) ? UiMetric.MenuSelOptionBg : UiMetric.MenuOptionBoxBg);
        pspVideoPrint(UiMetric.Font, k, i, option->Text, 
          (option == temp_option) ? UiMetric.SelectedColor : UiMetric.MenuOptionBoxColor);
      }
      pspVideoFillRect(k - UiMetric.MenuItemMargin, 
        i + fh - 8, 
        k + max_option_w + UiMetric.MenuItemMargin, 
        i + fh, UiMetric.MenuOptionBoxBg);

      /* Up arrow */
      if (option)
        pspVideoPrint(UiMetric.Font, 
          arrow_x, i + fh + anim_frame, "\272", UiMetric.MenuDecorColor);

      /* Render following items */
      i = sy + (pos.Index  + 1) * fh;
      for (option = temp_option->Next; option && i < dy; option = option->Next, i += fh)
      {
        pspVideoFillRect(k - UiMetric.MenuItemMargin, i, 
          k + max_option_w + UiMetric.MenuItemMargin, i + fh,
          UiMetric.MenuOptionBoxBg);
        pspVideoPrint(UiMetric.Font, k, i, option->Text, UiMetric.MenuOptionBoxColor);
      }

      pspVideoFillRect(k - UiMetric.MenuItemMargin, i, 
        k + max_option_w + UiMetric.MenuItemMargin, i + 8, 
        UiMetric.MenuOptionBoxBg);

      /* Down arrow */
      if (option)
        pspVideoPrint(UiMetric.Font, 
          arrow_x, i - fh - anim_frame, 
          "\273", UiMetric.MenuDecorColor);
    }

    /* Perform any custom drawing */
    if (uimenu->OnRender)
      uimenu->OnRender(uimenu, sel);

    /* Draw scrollbar */
    if (sbh > 0)
    {
      sby = sy + (int)((float)(h - sbh) * ((float)(pos.Offset + pos.Index) / (float)menu->Count));
      pspVideoFillRect(dx - UiMetric.ScrollbarWidth, sy, dx, dy, UiMetric.ScrollbarBgColor);
      pspVideoFillRect(dx - UiMetric.ScrollbarWidth, sby, dx, sby + sbh, UiMetric.ScrollbarColor);
    }

    pspVideoEnd();

    /* Wait if needed */
    do { sceRtcGetCurrentTick(&current_tick); }
    while (current_tick - last_tick < ticks_per_upd);
    last_tick = current_tick;

    /* Swap buffers */
    sceKernelDcacheWritebackAll();
    pspVideoWaitVSync();
    pspVideoSwapBuffers();
  }

  menu->Selected = sel;
}

void pspUiSplashScreen(PspUiSplash *splash)
{
  SceCtrlData pad;
  int fh = pspFontGetLineHeight(UiMetric.Font);

  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    if (pad.Buttons & UiMetric.CancelButton)
    {
      if (splash->OnCancel) splash->OnCancel(splash, NULL);
      break;
    }

    if ((pad.Buttons & CONTROL_BUTTON_MASK) && splash->OnButtonPress)
    {
      if (splash->OnButtonPress(splash, pad.Buttons & CONTROL_BUTTON_MASK))
          break;
    }

    pspVideoBegin();

    /* Clear screen */
    if (UiMetric.Background) 
      pspVideoPutImage(UiMetric.Background, 0, 0, 
        UiMetric.Background->Width, UiMetric.Background->Height);
    else 
      pspVideoClearScreen();

    /* Draw instructions */
    const char *dirs = (splash->OnGetStatusBarText)
      ? splash->OnGetStatusBarText(splash)
      : SplashStatusBarTemplate;
    pspVideoPrintCenter(UiMetric.Font, UiMetric.Left,
      SCR_HEIGHT - fh, UiMetric.Right, dirs, UiMetric.StatusBarColor);

    /* Perform any custom drawing */
    if (splash->OnRender)
      splash->OnRender(splash, NULL);

    pspVideoEnd();

    /* Swap buffers */
    sceKernelDcacheWritebackAll();
    pspVideoWaitVSync();
    pspVideoSwapBuffers();
  }
}
