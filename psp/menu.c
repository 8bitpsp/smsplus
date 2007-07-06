#include "menu.h"

#include <time.h>
#include <psptypes.h>
#include <psprtc.h>
#include <malloc.h>
#include <string.h>

#include "emumain.h"

#include "types.h"
#include "loadrom.h"
#include "system.h"

#include "image.h"
#include "ui.h"
#include "menu.h"
#include "ctrl.h"
#include "psp.h"
#include "util.h"

#define TAB_QUICKLOAD 0
#define TAB_OPTION    1
#define TAB_ABOUT     2
#define TAB_MAX       TAB_OPTION

#define OPTION_DISPLAY_MODE 1
#define OPTION_SYNC_FREQ    2
#define OPTION_FRAMESKIP    3
#define OPTION_VSYNC        4
#define OPTION_CLOCK_FREQ   5
#define OPTION_SHOW_FPS     6
#define OPTION_CONTROL_MODE 7

extern PspImage *Screen;

EmulatorOptions SmsOptions;

static int TabIndex;
static int ResumeEmulation;
static PspImage *Background;
static PspImage *NoSaveIcon;

static const char *QuickloadFilter[] = { "SMS", "GG", "ZIP", '\0' };

static const char *ScreenshotDir = "screens";
static const char *SaveStateDir = "states";

char *GameName;
char *ScreenshotPath;
static char *SaveStatePath;

#define WIDTH  256
#define HEIGHT 192

/* Tab labels */
static const char *TabLabel[] = 
{
  "Game",
  "Options",
  "About"
};

static void LoadOptions();
static void InitOptionDefaults();

int OnMenuItemChanged(const struct PspUiMenu *uimenu,
  PspMenuItem* item, const PspMenuOption* option);
int OnMenuOk(const void *uimenu, const void* sel_item);
int OnMenuButtonPress(const struct PspUiMenu *uimenu,
  PspMenuItem* sel_item, u32 button_mask);

int OnSplashButtonPress(const struct PspUiSplash *splash, u32 button_mask);
void OnSplashRender(const void *uiobject, const void *null);
const char* OnSplashGetStatusBarText(const struct PspUiSplash *splash);

int  OnGenericCancel(const void *uiobject, const void *param);
void OnGenericRender(const void *uiobject, const void *item_obj);
int  OnGenericButtonPress(const PspUiFileBrowser *browser, const char *path, 
  u32 button_mask);

int OnQuickloadOk(const void *browser, const void *path);

/* Define various menu options */
static const PspMenuOptionDef
  ToggleOptions[] = {
    { "Disabled", (void*)0 },
    { "Enabled", (void*)1 },
    { NULL, NULL } },
  ScreenSizeOptions[] = {
    { "Actual size", (void*)DISPLAY_MODE_UNSCALED },
    { "4:3 scaled (fit height)", (void*)DISPLAY_MODE_FIT_HEIGHT },
    { "16:9 scaled (fit screen)", (void*)DISPLAY_MODE_FILL_SCREEN },
    { NULL, NULL } },
  FrameLimitOptions[] = {
    { "Disabled", (void*)0 },
    { "60 fps (NTSC)", (void*)60 },
    { "50 fps (PAL)", (void*)50 },
    { NULL, NULL } },
  FrameSkipOptions[] = {
    { "No skipping", (void*)0 },
    { "Skip 1 frame", (void*)1 },
    { "Skip 2 frames", (void*)2 },
    { "Skip 3 frames", (void*)3 },
    { "Skip 4 frames", (void*)4 },
    { "Skip 5 frames", (void*)5 },
    { NULL, NULL } },
  PspClockFreqOptions[] = {
    { "222 MHz", (void*)222 },
    { "300 MHz", (void*)300 },
    { "333 MHz", (void*)333 },
    { NULL, NULL } },
  ControlModeOptions[] = {
    { "\026\242\020 cancels, \026\241\020 confirms (US)", (void*)0 },
    { "\026\241\020 cancels, \026\242\020 confirms (Japan)", (void*)1 },
    { NULL, NULL } };

static const PspMenuItemDef
  OptionMenuDef[] = {
    { "\tVideo", NULL, NULL, -1, NULL },
    { "Screen size",         (void*)OPTION_DISPLAY_MODE, 
      ScreenSizeOptions,   -1, "\026\250\020 Change screen size" },
    { "\tPerformance", NULL, NULL, -1, NULL },
    { "Frame limiter",       (void*)OPTION_SYNC_FREQ, 
      FrameLimitOptions,   -1, "\026\250\020 Change screen update frequency" },
    { "Frame skipping",      (void*)OPTION_FRAMESKIP,
      FrameSkipOptions,    -1, "\026\250\020 Change number of frames skipped per update" },
    { "VSync",               (void*)OPTION_VSYNC,
      ToggleOptions,       -1, "\026\250\020 Enable to reduce tearing; disable to increase speed" },
    { "PSP clock frequency", (void*)OPTION_CLOCK_FREQ,
      PspClockFreqOptions, -1, 
      "\026\250\020 Larger values: faster emulation, faster battery depletion (default: 222MHz)" },
    { "Show FPS counter",    (void*)OPTION_SHOW_FPS,
      ToggleOptions,       -1, "\026\250\020 Show/hide the frames-per-second counter" },
    { "\tMenu", NULL, NULL, -1, NULL },
    { "Button mode",        (void*)OPTION_CONTROL_MODE,
      ControlModeOptions,  -1, "\026\250\020 Change OK and Cancel button mapping" },
    { NULL, NULL }
  };

PspUiSplash SplashScreen = 
{
  OnSplashRender,
  OnGenericCancel,
  OnSplashButtonPress,
  NULL
};

PspUiMenu OptionUiMenu =
{
  NULL,                  /* PspMenu */
  OnGenericRender,       /* OnRender() */
  OnMenuOk,              /* OnOk() */
  OnGenericCancel,       /* OnCancel() */
  OnMenuButtonPress,     /* OnButtonPress() */
  OnMenuItemChanged,     /* OnItemChanged() */
};

PspUiFileBrowser QuickloadBrowser = 
{
  OnGenericRender,
  OnQuickloadOk,
  OnGenericCancel,
  OnGenericButtonPress,
  QuickloadFilter,
  0
};

void InitMenu()
{
  InitEmulator();

  /* Reset variables */
  TabIndex = TAB_ABOUT;
  Background = NULL;
  GameName = NULL;

  /* Load the background image */
  Background = pspImageLoadPng("background.png");

  /* Init NoSaveState icon image */
  NoSaveIcon=pspImageCreate(136, 114);
  pspImageClear(NoSaveIcon, RGB(66,66,66));

  /* Initialize options menu */
  OptionUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(OptionUiMenu.Menu, OptionMenuDef);

  /* Initialize paths */
  SaveStatePath 
    = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(SaveStateDir) + 2));
  sprintf(SaveStatePath, "%s%s/", pspGetAppDirectory(), SaveStateDir);
  ScreenshotPath 
    = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(ScreenshotDir) + 2));
  sprintf(ScreenshotPath, "%s%s/", pspGetAppDirectory(), ScreenshotDir);

  /* Initialize options */
  LoadOptions();

  /* Initialize UI components */
  UiMetric.Background = Background;
  UiMetric.Font = &PspStockFont;
  UiMetric.Left = 8;
  UiMetric.Top = 24;
  UiMetric.Right = 472;
  UiMetric.Bottom = 240;
  UiMetric.OkButton = (!SmsOptions.ControlMode) ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
  UiMetric.CancelButton = (!SmsOptions.ControlMode) ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
  UiMetric.ScrollbarColor = PSP_VIDEO_GRAY;
  UiMetric.ScrollbarBgColor = 0x44ffffff;
  UiMetric.ScrollbarWidth = 10;
  UiMetric.DialogBorderColor = PSP_VIDEO_GRAY;
  UiMetric.DialogBgColor = PSP_VIDEO_DARKGRAY;
  UiMetric.TextColor = PSP_VIDEO_GRAY;
  UiMetric.SelectedColor = PSP_VIDEO_GREEN;
  UiMetric.SelectedBgColor = COLOR(0,0,0,0x55);
  UiMetric.StatusBarColor = PSP_VIDEO_WHITE;
  UiMetric.BrowserFileColor = PSP_VIDEO_GRAY;
  UiMetric.BrowserDirectoryColor = PSP_VIDEO_YELLOW;
  UiMetric.GalleryIconsPerRow = 5;
  UiMetric.GalleryIconMarginWidth = 8;
  UiMetric.MenuItemMargin = 20;
  UiMetric.MenuSelOptionBg = PSP_VIDEO_BLACK;
  UiMetric.MenuOptionBoxColor = PSP_VIDEO_GRAY;
  UiMetric.MenuOptionBoxBg = COLOR(0, 0, 0, 0xBB);
  UiMetric.MenuDecorColor = PSP_VIDEO_YELLOW;
  UiMetric.DialogFogColor = COLOR(0, 0, 0, 88);
  UiMetric.TitlePadding = 4;
  UiMetric.TitleColor = PSP_VIDEO_WHITE;
  UiMetric.MenuFps = 30;
}

void DisplayMenu()
{
  PspMenuItem *item;
  ResumeEmulation = 0;

  /* Menu loop */
  do
  {
    ResumeEmulation = 0;

    /* Set normal clock frequency */
    pspSetClockFrequency(222);
    /* Set buttons to autorepeat */
    pspCtrlSetPollingMode(PSP_CTRL_AUTOREPEAT);

    /* Display appropriate tab */
    switch (TabIndex)
    {
    case TAB_QUICKLOAD:
      pspUiOpenBrowser(&QuickloadBrowser, GameName);
      break;
    case TAB_OPTION:
      /* Init menu options */
      item = pspMenuFindItemByUserdata(OptionUiMenu.Menu,
        (void*)OPTION_DISPLAY_MODE);
      pspMenuSelectOptionByValue(item, (void*)SmsOptions.DisplayMode);
      item = pspMenuFindItemByUserdata(OptionUiMenu.Menu,
        (void*)OPTION_SYNC_FREQ);
      pspMenuSelectOptionByValue(item, (void*)SmsOptions.UpdateFreq);
      item = pspMenuFindItemByUserdata(OptionUiMenu.Menu,
        (void*)OPTION_FRAMESKIP);
      pspMenuSelectOptionByValue(item, (void*)(int)SmsOptions.Frameskip);
      item = pspMenuFindItemByUserdata(OptionUiMenu.Menu,
        (void*)OPTION_VSYNC);
      pspMenuSelectOptionByValue(item, (void*)SmsOptions.VSync);
      item = pspMenuFindItemByUserdata(OptionUiMenu.Menu,
        (void*)OPTION_CLOCK_FREQ);
      pspMenuSelectOptionByValue(item, (void*)SmsOptions.ClockFreq);
      item = pspMenuFindItemByUserdata(OptionUiMenu.Menu,
        (void*)OPTION_SHOW_FPS);
      pspMenuSelectOptionByValue(item, (void*)SmsOptions.ShowFps);
      item = pspMenuFindItemByUserdata(OptionUiMenu.Menu,
        (void*)OPTION_CONTROL_MODE);
      pspMenuSelectOptionByValue(item, (void*)SmsOptions.ControlMode);

      pspUiOpenMenu(&OptionUiMenu, NULL);
      break;
    case TAB_ABOUT:
      pspUiSplashScreen(&SplashScreen);
      break;
    }

    if (!ExitPSP)
    {
      /* Set clock frequency during emulation */
      pspSetClockFrequency(SmsOptions.ClockFreq);
      /* Set buttons to normal mode */
      pspCtrlSetPollingMode(PSP_CTRL_NORMAL);

      /* Resume emulation */
      if (ResumeEmulation) RunEmulator();
    }
  } while (!ExitPSP);
}

int OnGenericCancel(const void *uiobject, const void* param)
{
  if (GameName) ResumeEmulation = 1;
  return 1;
}

void OnSplashRender(const void *splash, const void *null)
{
  int fh, i, x, y, height;
  const char *lines[] = 
  { 
    "SMS Plus version 1.2.1 ("__DATE__")",
    "\026http://psp.akop.org/smsplus",
    " ",
    "2007 Akop Karapetyan",
    "1998-2004 Charles MacDonald",
    NULL
  };

  fh = pspFontGetLineHeight(UiMetric.Font);

  for (i = 0; lines[i]; i++);
  height = fh * (i - 1);

  /* Render lines */
  for (i = 0, y = SCR_HEIGHT / 2 - height / 2; lines[i]; i++, y += fh)
  {
    x = SCR_WIDTH / 2 - pspFontGetTextWidth(UiMetric.Font, lines[i]) / 2;
    pspVideoPrint(UiMetric.Font, x, y, lines[i], PSP_VIDEO_GRAY);
  }

  /* Render PSP status */
  OnGenericRender(splash, null);
}

int  OnSplashButtonPress(const struct PspUiSplash *splash, 
  u32 button_mask)
{
  return OnGenericButtonPress(NULL, NULL, button_mask);
}

/* Handles drawing of generic items */
void OnGenericRender(const void *uiobject, const void *item_obj)
{
  static int percentiles[] = { 60, 30, 12, 0 };
  static char caption[32];
  int percent_left, time_left, hr_left, min_left, i, width;
  pspTime time;

  time_left = pspGetBatteryTime();
  sceRtcGetCurrentClockLocalTime(&time);
  percent_left = pspGetBatteryPercent();
  hr_left = time_left / 60;
  min_left = time_left % 60;

  /* Determine appropriate charge icon */
  for (i = 0; i < 4; i++)
    if (percent_left >= percentiles[i])
      break;

  /* Display PSP status (time, battery, etc..) */
  sprintf(caption, "\270%2i/%2i %02i%c%02i %c%3i%% (%02i:%02i) ", 
    time.month, time.day, 
    time.hour, (time.microseconds > 500000) ? ':' : ' ', time.minutes, 
    (percent_left > 6) 
      ? 0263 + i 
      : 0263 + i + (time.microseconds > 500000) ? 0 : 1, 
    percent_left, 
    (hr_left < 0) ? 0 : hr_left, (min_left < 0) ? 0 : min_left);

  width = pspFontGetTextWidth(UiMetric.Font, caption);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH - width, 0, caption, PSP_VIDEO_WHITE);

  /* Draw tabs */
  int height = pspFontGetLineHeight(UiMetric.Font);
  int x;

  for (i = 0, x = 5; i <= TAB_MAX; i++, x += width + 10)
  {
    /* Determine width of text */
    width = pspFontGetTextWidth(UiMetric.Font, TabLabel[i]);

    /* Draw background of active tab */
    if (i == TabIndex)
      pspVideoFillRect(x - 5, 0, x + width + 5, height + 1, 0xff959595);

    /* Draw name of tab */
    pspVideoPrint(UiMetric.Font, x, 0, TabLabel[i], PSP_VIDEO_WHITE);
  }
}

int OnGenericButtonPress(const PspUiFileBrowser *browser, 
  const char *path, u32 button_mask)
{
  /* If L or R are pressed, switch tabs */
  if (button_mask & PSP_CTRL_LTRIGGER)
  { if (--TabIndex < 0) TabIndex=TAB_MAX; }
  else if (button_mask & PSP_CTRL_RTRIGGER)
  { if (++TabIndex > TAB_MAX) TabIndex=0; }
  else if ((button_mask & (PSP_CTRL_START | PSP_CTRL_SELECT)) 
    == (PSP_CTRL_START | PSP_CTRL_SELECT))
  {
    if (pspUtilSaveVramSeq(ScreenshotPath, "ui"))
      pspUiAlert("Saved successfully");
    else
      pspUiAlert("ERROR: Not saved");
    return 0;
  }
  else return 0;

  return 1;
}

int  OnMenuItemChanged(const struct PspUiMenu *uimenu, 
  PspMenuItem* item, const PspMenuOption* option)
{
  if (uimenu == &OptionUiMenu)
  {
    switch((int)item->Userdata)
    {
    case OPTION_DISPLAY_MODE:
      SmsOptions.DisplayMode = (int)option->Value;
      break;
    case OPTION_SYNC_FREQ:
      SmsOptions.UpdateFreq = (int)option->Value;
      break;
    case OPTION_FRAMESKIP:
      SmsOptions.Frameskip = (int)option->Value;
      break;
    case OPTION_VSYNC:
      SmsOptions.VSync = (int)option->Value;
      break;
    case OPTION_CLOCK_FREQ:
      SmsOptions.ClockFreq = (int)option->Value;
      break;
    case OPTION_SHOW_FPS:
      SmsOptions.ShowFps = (int)option->Value;
      break;
    case OPTION_CONTROL_MODE:
      SmsOptions.ControlMode = (int)option->Value;
      UiMetric.OkButton = (!(int)option->Value) ? PSP_CTRL_CROSS
        : PSP_CTRL_CIRCLE;
      UiMetric.CancelButton = (!(int)option->Value) ? PSP_CTRL_CIRCLE
        : PSP_CTRL_CROSS;
      break;
    }
  }

  return 1;
}

int OnMenuOk(const void *uimenu, const void* sel_item)
{
  return 0;
}

int  OnMenuButtonPress(const struct PspUiMenu *uimenu, 
  PspMenuItem* sel_item, 
  u32 button_mask)
{
  return OnGenericButtonPress(NULL, NULL, button_mask);
}

int OnQuickloadOk(const void *browser, const void *path)
{
  int first_time = 0;

  if (GameName)
  {
    free(GameName);
    system_poweroff();
  }
  else first_time = 1;

  if (!load_rom(path))
  {
    pspUiAlert("Error loading cartridge");
    return 0;
  }

  GameName = strdup(path);

  if (first_time)
  {
    pspUiFlashMessage("Initializing for first-time use\nPlease wait...");
    system_init();
  }
  else system_reinit();

  system_poweron();

  ResumeEmulation = 1;
  return 1;
}

/* Load options */
void LoadOptions()
{
InitOptionDefaults();
/*
  char *path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(OptionsFile) + 1));
  sprintf(path, "%s%s", pspGetAppDirectory(), OptionsFile);

  /* Initialize INI structure *
  INIFile ini;
  InitINI(&ini);

  /* Read the file *
  if (!LoadINI(&ini, path))
  {
    /* File does not exist; load defaults *
    TrashINI(&ini);
    InitOptionDefaults();
  }
  else
  {
    char path[PSP_FILEIO_MAX_PATH_LEN + 1];

    /* Load values *
    DisplayMode = INIGetInteger(&ini, "Video", "Display Mode", DISPLAY_MODE_UNSCALED);
    UpdateFreq = INIGetInteger(&ini, "Video", "Update Frequency", 60);
    Frameskip = INIGetInteger(&ini, "Video", "Frameskip", 1);
    VSync = INIGetInteger(&ini, "Video", "VSync", 0);
    ClockFreq = INIGetInteger(&ini, "Video", "PSP Clock Frequency", 222);
    ShowFps = INIGetInteger(&ini, "Video", "Show FPS", 0);
    ControlMode = INIGetInteger(&ini, "Menu", "Control Mode", 0);

    Mode = (Mode&~MSX_VIDEO) | INIGetInteger(&ini, "System", "Timing", Mode & MSX_VIDEO);
    Mode = (Mode&~MSX_MODEL) | INIGetInteger(&ini, "System", "Model", Mode & MSX_MODEL);
    RAMPages = INIGetInteger(&ini, "System", "RAM Pages", RAMPages);
    VRAMPages = INIGetInteger(&ini, "System", "VRAM Pages", VRAMPages);

    if (DiskPath) { free(DiskPath); DiskPath = NULL; }
    if (CartPath) { free(CartPath); CartPath = NULL; }
    if (Quickload) { free(Quickload); Quickload = NULL; }

    if (INIGetString(&ini, "File", "Disk Path", path, PSP_FILEIO_MAX_PATH_LEN))
      DiskPath = strdup(path);
    if (INIGetString(&ini, "File", "Cart Path", path, PSP_FILEIO_MAX_PATH_LEN))
      CartPath = strdup(path);
    if (INIGetString(&ini, "File", "Game Path", path, PSP_FILEIO_MAX_PATH_LEN))
      Quickload = strdup(path);
  }

  /* Clean up *
  free(path);
  TrashINI(&ini);
*/
}

/* Initialize options to system defaults */
void InitOptionDefaults()
{
  SmsOptions.ControlMode = 0;
  SmsOptions.DisplayMode = DISPLAY_MODE_UNSCALED;
  SmsOptions.UpdateFreq = 60;
  SmsOptions.Frameskip = 1;
  SmsOptions.VSync = 0;
  SmsOptions.ClockFreq = 222;
  SmsOptions.ShowFps = 0;
}

/* Release menu resources */
void TrashMenu()
{
  TrashEmulator();

  /* Trash menus */
  pspMenuDestroy(OptionUiMenu.Menu);
/*
  pspMenuDestroy(SystemUiMenu.Menu);
  pspMenuDestroy(ControlUiMenu.Menu);
  pspMenuDestroy(SaveStateGallery.Menu);
*/

  /* Trash images */
  if (Background) pspImageDestroy(Background);
  if (NoSaveIcon) pspImageDestroy(NoSaveIcon);

  if (GameName) free(GameName);
  free(ScreenshotPath);
  free(SaveStatePath);
}
