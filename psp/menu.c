#include "menu.h"

#include <time.h>
#include <psptypes.h>
#include <psprtc.h>
#include <malloc.h>
#include <string.h>
#include <pspkernel.h>

#include "emumain.h"

#include "types.h"
#include "loadrom.h"
#include "system.h"
#include "state.h"
#include "shared.h"

#include "fileio.h"
#include "image.h"
#include "ui.h"
#include "menu.h"
#include "ctrl.h"
#include "psp.h"
#include "util.h"
#include "init.h"

#define TAB_QUICKLOAD 0
#define TAB_STATE     1
#define TAB_CONTROL   2
#define TAB_OPTION    3
#define TAB_SYSTEM    4
#define TAB_ABOUT     5
#define TAB_MAX       TAB_SYSTEM

#define OPTION_DISPLAY_MODE 1
#define OPTION_SYNC_FREQ    2
#define OPTION_FRAMESKIP    3
#define OPTION_VSYNC        4
#define OPTION_CLOCK_FREQ   5
#define OPTION_SHOW_FPS     6
#define OPTION_CONTROL_MODE 7

#define SYSTEM_SCRNSHOT    1
#define SYSTEM_RESET       2

extern PspImage *Screen;

EmulatorOptions SmsOptions;

static int TabIndex;
static int ResumeEmulation;
static PspImage *Background;
static PspImage *NoSaveIcon;

static const char *QuickloadFilter[] = { "SMS", "GG", "ZIP", '\0' },
  PresentSlotText[] = "\026\244\020 Save\t\026\001\020 Load\t\026\243\020 Delete",
  EmptySlotText[] = "\026\244\020 Save",
  ControlHelpText[] = "\026\250\020 Change mapping\t\026\001\020 Save to \271\t\026\243\020 Load defaults";

static const char *ScreenshotDir = "screens";
static const char *SaveStateDir = "savedata";
static const char *ButtonConfigFile = "buttons";
static const char *OptionsFile = "smsplus.ini";

char *GameName;
char *ScreenshotPath;
static char *SaveStatePath;
static char *GamePath;

#define WIDTH  256
#define HEIGHT 192

/* Tab labels */
static const char *TabLabel[] = 
{
  "Game",
  "Save/Load",
  "Controls",
  "Options",
  "System",
  "About"
};

static void LoadOptions();
static int  SaveOptions();
static void InitOptionDefaults();

static void DisplayStateTab();

static void InitButtonConfig();
static int  SaveButtonConfig();
static int  LoadButtonConfig();

PspImage*   LoadStateIcon(const char *path);
int         LoadState(const char *path);
PspImage*   SaveState(const char *path, PspImage *icon);

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

int  OnSaveStateOk(const void *gallery, const void *item);
int  OnSaveStateButtonPress(const PspUiGallery *gallery, PspMenuItem* item, 
       u32 button_mask);

int OnQuickloadOk(const void *browser, const void *path);

void OnSystemRender(const void *uiobject, const void *item_obj);

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
    { NULL, NULL } },
  ButtonMapOptions[] = {
    /* Unmapped */
    { "None", (void*)0 },
    /* Special */
    { "Special: Open Menu", (void*)(SPC|SPC_MENU) },  
    /* Joystick */
    { "Joystick Up",        (void*)(JOY|INPUT_UP) },
    { "Joystick Down",      (void*)(JOY|INPUT_DOWN) },
    { "Joystick Left",      (void*)(JOY|INPUT_LEFT) },
    { "Joystick Right",     (void*)(JOY|INPUT_RIGHT) },
    { "Joystick Button I",  (void*)(JOY|INPUT_BUTTON1) },
    { "Joystick Button II", (void*)(JOY|INPUT_BUTTON2) },
    /* Joystick */
    { "Start (GG) / Pause (SMS)", (void*)(SYS|INPUT_START|INPUT_PAUSE) },
    { "Soft Reset (SMS)",    (void*)(SYS|INPUT_RESET) },
    /* End */
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
  },
  ControlMenuDef[] = {
    { "\026"PSP_FONT_ANALUP,     (void*)MAP_ANALOG_UP,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_ANALDOWN,   (void*)MAP_ANALOG_DOWN,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_ANALLEFT,   (void*)MAP_ANALOG_LEFT,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_ANALRIGHT,  (void*)MAP_ANALOG_RIGHT,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_UP,         (void*)MAP_BUTTON_UP,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_DOWN,       (void*)MAP_BUTTON_DOWN,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_LEFT,       (void*)MAP_BUTTON_LEFT,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_RIGHT,      (void*)MAP_BUTTON_RIGHT,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_SQUARE,     (void*)MAP_BUTTON_SQUARE,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_CROSS,      (void*)MAP_BUTTON_CROSS,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_CIRCLE,     (void*)MAP_BUTTON_CIRCLE,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_TRIANGLE,   (void*)MAP_BUTTON_TRIANGLE,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_LTRIGGER,   (void*)MAP_BUTTON_LTRIGGER,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_RTRIGGER,   (void*)MAP_BUTTON_RTRIGGER,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_SELECT,     (void*)MAP_BUTTON_SELECT,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_START,      (void*)MAP_BUTTON_START,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_LTRIGGER"+"PSP_FONT_RTRIGGER,
                           (void*)MAP_BUTTON_LRTRIGGERS,
      ButtonMapOptions, -1, ControlHelpText },
    { "\026"PSP_FONT_START"+"PSP_FONT_SELECT,
                           (void*)MAP_BUTTON_STARTSELECT,
      ButtonMapOptions, -1, ControlHelpText },
    { NULL, NULL }
  },
  SystemMenuDef[] = {
    { "\tSystem", NULL, NULL, -1, NULL },
    { "Reset",            (void*)SYSTEM_RESET,
      NULL,             -1, "\026\001\020 Reset" },
    { "Save screenshot",  (void*)SYSTEM_SCRNSHOT,
      NULL,             -1, "\026\001\020 Save screenshot" },
    { NULL, NULL }
  };

PspUiSplash SplashScreen =
{
  OnSplashRender,
  OnGenericCancel,
  OnSplashButtonPress,
  NULL
};

PspUiGallery SaveStateGallery = 
{
  NULL,                        /* PspMenu */
  OnGenericRender,             /* OnRender() */
  OnSaveStateOk,               /* OnOk() */
  OnGenericCancel,             /* OnCancel() */
  OnSaveStateButtonPress,      /* OnButtonPress() */
  NULL                         /* Userdata */
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

PspUiMenu ControlUiMenu =
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

PspUiMenu SystemUiMenu =
{
  NULL,                  /* PspMenu */
  OnSystemRender,        /* OnRender() */
  OnMenuOk,              /* OnOk() */
  OnGenericCancel,       /* OnCancel() */
  OnMenuButtonPress,     /* OnButtonPress() */
  OnMenuItemChanged,     /* OnItemChanged() */
};

/* Game configuration (includes button maps) */
struct ButtonConfig ActiveConfig;

/* Default configuration */
struct ButtonConfig DefaultConfig =
{
  {
    JOY|INPUT_UP,     /* Analog Up    */
    JOY|INPUT_DOWN,   /* Analog Down  */
    JOY|INPUT_LEFT,   /* Analog Left  */
    JOY|INPUT_RIGHT,  /* Analog Right */
    JOY|INPUT_UP,     /* D-pad Up     */
    JOY|INPUT_DOWN,   /* D-pad Down   */
    JOY|INPUT_LEFT,   /* D-pad Left   */
    JOY|INPUT_RIGHT,  /* D-pad Right  */
    JOY|INPUT_BUTTON2,/* Square       */
    JOY|INPUT_BUTTON1,/* Cross        */
    0,                /* Circle       */
    0,                /* Triangle     */
    0,                /* L Trigger    */
    0,                /* R Trigger    */
    0,                /* Select       */
    SYS|INPUT_START|INPUT_PAUSE,
                      /* Start        */
    SPC|SPC_MENU,     /* L+R Triggers */
    0,                /* Start+Select */
  }
};

/* Button masks */
const u64 ButtonMask[] = 
{
  PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER, 
  PSP_CTRL_START    | PSP_CTRL_SELECT,
  PSP_CTRL_ANALUP,    PSP_CTRL_ANALDOWN,
  PSP_CTRL_ANALLEFT,  PSP_CTRL_ANALRIGHT,
  PSP_CTRL_UP,        PSP_CTRL_DOWN,
  PSP_CTRL_LEFT,      PSP_CTRL_RIGHT,
  PSP_CTRL_SQUARE,    PSP_CTRL_CROSS,
  PSP_CTRL_CIRCLE,    PSP_CTRL_TRIANGLE,
  PSP_CTRL_LTRIGGER,  PSP_CTRL_RTRIGGER,
  PSP_CTRL_SELECT,    PSP_CTRL_START,
  0 /* End */
};

/* Button map ID's */
const int ButtonMapId[] = 
{
  MAP_BUTTON_LRTRIGGERS, 
  MAP_BUTTON_STARTSELECT,
  MAP_ANALOG_UP,       MAP_ANALOG_DOWN,
  MAP_ANALOG_LEFT,     MAP_ANALOG_RIGHT,
  MAP_BUTTON_UP,       MAP_BUTTON_DOWN,
  MAP_BUTTON_LEFT,     MAP_BUTTON_RIGHT,
  MAP_BUTTON_SQUARE,   MAP_BUTTON_CROSS,
  MAP_BUTTON_CIRCLE,   MAP_BUTTON_TRIANGLE,
  MAP_BUTTON_LTRIGGER, MAP_BUTTON_RTRIGGER,
  MAP_BUTTON_SELECT,   MAP_BUTTON_START,
  -1 /* End */
};

void InitMenu()
{
  InitEmulator();

  /* Reset variables */
  TabIndex = TAB_ABOUT;
  Background = NULL;
  GameName = NULL;
  GamePath = NULL;

  /* Load the background image */
  Background = pspImageLoadPng("background.png");

  /* Init NoSaveState icon image */
  NoSaveIcon=pspImageCreate(136, 114);
  pspImageClear(NoSaveIcon, RGB(0x0c,0,0x3f));

  /* Initialize state menu */
  SaveStateGallery.Menu = pspMenuCreate();
  int i;
  PspMenuItem *item;
  for (i = 0; i < 10; i++)
  {
    item = pspMenuAppendItem(SaveStateGallery.Menu, NULL, (void*)i);
    pspMenuSetHelpText(item, EmptySlotText);
  }

  /* Initialize options menu */
  OptionUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(OptionUiMenu.Menu, OptionMenuDef);

  /* Initialize control menu */
  ControlUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(ControlUiMenu.Menu, ControlMenuDef);

  /* Initialize system menu */
  SystemUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(SystemUiMenu.Menu, SystemMenuDef);

  /* Initialize paths */
  SaveStatePath 
    = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(SaveStateDir) + 2));
  sprintf(SaveStatePath, "%s%s/", pspGetAppDirectory(), SaveStateDir);
  ScreenshotPath 
    = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(ScreenshotDir) + 2));
  sprintf(ScreenshotPath, "%s%s/", pspGetAppDirectory(), ScreenshotDir);

  /* Initialize options */
  LoadOptions();

  /* Load default configuration */
  LoadButtonConfig();

  /* Initialize UI components */
  UiMetric.Background = Background;
  UiMetric.Font = &PspStockFont;
  UiMetric.Left = 8;
  UiMetric.Top = 24;
  UiMetric.Right = 472;
  UiMetric.Bottom = 250;
  UiMetric.OkButton = (!SmsOptions.ControlMode) ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
  UiMetric.CancelButton = (!SmsOptions.ControlMode) ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
  UiMetric.ScrollbarColor = PSP_VIDEO_GRAY;
  UiMetric.ScrollbarBgColor = 0x44ffffff;
  UiMetric.ScrollbarWidth = 10;
  UiMetric.DialogBorderColor = PSP_VIDEO_GRAY;
  UiMetric.DialogBgColor = PSP_VIDEO_DARKGRAY;
  UiMetric.TextColor = PSP_VIDEO_GRAY;
  UiMetric.SelectedColor = PSP_VIDEO_YELLOW;
  UiMetric.SelectedBgColor = COLOR(0xff,0xff,0xff,0x44);
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
  int i;
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
    case TAB_STATE:
      DisplayStateTab();
      break;
    case TAB_CONTROL:
      /* Load current button mappings */
      for (item = ControlUiMenu.Menu->First, i = 0; item; item = item->Next, i++)
        pspMenuSelectOptionByValue(item, (void*)ActiveConfig.ButtonMap[i]);
      pspUiOpenMenu(&ControlUiMenu, NULL);
      break;
    case TAB_QUICKLOAD:
      pspUiOpenBrowser(&QuickloadBrowser, (GameName) ? GameName : GamePath);
      break;
    case TAB_SYSTEM:
      pspUiOpenMenu(&SystemUiMenu, NULL);
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
    width = -10;

    if (!GameName && (i == TAB_STATE || i == TAB_SYSTEM))
      continue;

    /* Determine width of text */
    width = pspFontGetTextWidth(UiMetric.Font, TabLabel[i]);

    /* Draw background of active tab */
    if (i == TabIndex)
      pspVideoFillRect(x - 5, 0, x + width + 5, height + 1, COLOR(0x74,0x74,0xbe,0xff));

    /* Draw name of tab */
    pspVideoPrint(UiMetric.Font, x, 0, TabLabel[i], PSP_VIDEO_WHITE);
  }
}

int OnGenericButtonPress(const PspUiFileBrowser *browser, 
  const char *path, u32 button_mask)
{
  int tab_index;

  /* If L or R are pressed, switch tabs */
  if (button_mask & PSP_CTRL_LTRIGGER)
  {
    TabIndex--;
    do
    {
      tab_index = TabIndex;
      if (!GameName && (TabIndex == TAB_STATE || TabIndex == TAB_SYSTEM)) TabIndex--;
      if (TabIndex < 0) TabIndex = TAB_MAX;
    } while (tab_index != TabIndex);
  }
  else if (button_mask & PSP_CTRL_RTRIGGER)
  {
    TabIndex++;
    do
    {
      tab_index = TabIndex;
      if (!GameName && (TabIndex == TAB_STATE || TabIndex == TAB_SYSTEM)) TabIndex++;
      if (TabIndex > TAB_MAX) TabIndex = 0;
    } while (tab_index != TabIndex);
  }
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
  if (uimenu == &ControlUiMenu)
  {
    ActiveConfig.ButtonMap[(int)item->Userdata] = (unsigned int)option->Value;
  }
  else if (uimenu == &OptionUiMenu)
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
  if (uimenu == &ControlUiMenu)
  {
    /* Save to MS */
    if (SaveButtonConfig())
      pspUiAlert("Changes saved");
    else
      pspUiAlert("ERROR: Changes not saved");
  }
  else if (uimenu == &SystemUiMenu)
  {
    switch ((int)((const PspMenuItem*)sel_item)->Userdata)
    {
    case SYSTEM_RESET:

      /* Reset system */
      if (pspUiConfirm("Reset the system?"))
      {
        ResumeEmulation = 1;
        system_reset();
        return 1;
      }
      break;

    case SYSTEM_SCRNSHOT:

      /* Save screenshot */
      if (!pspUtilSavePngSeq(ScreenshotPath, pspFileIoGetFilename(GameName), Screen))
        pspUiAlert("ERROR: Screenshot not saved");
      else
        pspUiAlert("Screenshot saved successfully");
      break;
    }
  }

  return 0;
}

int  OnMenuButtonPress(const struct PspUiMenu *uimenu, 
  PspMenuItem* sel_item, 
  u32 button_mask)
{
  if (uimenu == &ControlUiMenu)
  {
    if (button_mask & PSP_CTRL_TRIANGLE)
    {
      PspMenuItem *item;
      int i;

      /* Load default mapping */
      InitButtonConfig();

      /* Modify the menu */
      for (item = ControlUiMenu.Menu->First, i = 0; item; item = item->Next, i++)
        pspMenuSelectOptionByValue(item, (void*)DefaultConfig.ButtonMap[i]);

      return 0;
    }
  }

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

  if (GamePath) free(GamePath);
  GamePath = pspFileIoGetParentDirectory(GameName);

  if (first_time)
  {
    pspUiFlashMessage("Initializing for first-time use\nPlease wait...");
    system_init();
  }
  else system_reinit();

  system_poweron();

  /* Reset viewport */
  if (IS_GG)
  {
    Screen->Viewport.X = 48;
    Screen->Viewport.Y = 24;
    Screen->Viewport.Width = 160;
    Screen->Viewport.Height = 144;
  }
  else
  {
    Screen->Viewport.X = 0;
    Screen->Viewport.Y = 0;
    Screen->Viewport.Width = 256;
    Screen->Viewport.Height = 192;
  }

  ResumeEmulation = 1;
  return 1;
}

int OnSaveStateOk(const void *gallery, const void *item)
{
  if (!GameName) { TabIndex++; return 0; }

  char *path;
  const char *config_name = pspFileIoGetFilename(GameName);

  path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
  sprintf(path, "%s%s.s%02i", SaveStatePath, config_name,
    (int)((const PspMenuItem*)item)->Userdata);

  if (pspFileIoCheckIfExists(path) && pspUiConfirm("Load state?"))
  {
    if (LoadState(path))
    {
      ResumeEmulation = 1;
      pspMenuFindItemByUserdata(((const PspUiGallery*)gallery)->Menu,
        ((const PspMenuItem*)item)->Userdata);
      free(path);

      return 1;
    }
    pspUiAlert("ERROR: State failed to load");
  }

  free(path);
  return 0;
}

int OnSaveStateButtonPress(const PspUiGallery *gallery, 
      PspMenuItem *sel, 
      u32 button_mask)
{
  if (!GameName) { TabIndex++; return 0; }

  if (button_mask & PSP_CTRL_SQUARE 
    || button_mask & PSP_CTRL_TRIANGLE)
  {
    char *path;
    char caption[32];
    const char *config_name = pspFileIoGetFilename(GameName);
    PspMenuItem *item = pspMenuFindItemByUserdata(gallery->Menu, sel->Userdata);

    path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
    sprintf(path, "%s%s.s%02i", SaveStatePath, config_name, (int)item->Userdata);

    do /* not a real loop; flow control construct */
    {
      if (button_mask & PSP_CTRL_SQUARE)
      {
        if (pspFileIoCheckIfExists(path) && !pspUiConfirm("Overwrite existing state?"))
          break;

        pspUiFlashMessage("Saving, please wait ...");

        PspImage *icon;
        if (!(icon = SaveState(path, Screen)))
        {
          pspUiAlert("ERROR: State not saved");
          break;
        }

        SceIoStat stat;

        /* Trash the old icon (if any) */
        if (item->Icon && item->Icon != NoSaveIcon)
          pspImageDestroy((PspImage*)item->Icon);

        /* Update icon, help text */
        item->Icon = icon;
        pspMenuSetHelpText(item, PresentSlotText);

        /* Get file modification time/date */
        if (sceIoGetstat(path, &stat) < 0)
          sprintf(caption, "ERROR");
        else
          sprintf(caption, "%02i/%02i/%02i %02i:%02i", 
            stat.st_mtime.month,
            stat.st_mtime.day,
            stat.st_mtime.year - (stat.st_mtime.year / 100) * 100,
            stat.st_mtime.hour,
            stat.st_mtime.minute);

        pspMenuSetCaption(item, caption);
      }
      else if (button_mask & PSP_CTRL_TRIANGLE)
      {
        if (!pspFileIoCheckIfExists(path) || !pspUiConfirm("Delete state?"))
          break;

        if (!pspFileIoDelete(path))
        {
          pspUiAlert("ERROR: State not deleted");
          break;
        }

        /* Trash the old icon (if any) */
        if (item->Icon && item->Icon != NoSaveIcon)
          pspImageDestroy((PspImage*)item->Icon);

        /* Update icon, caption */
        item->Icon = NoSaveIcon;
        pspMenuSetHelpText(item, EmptySlotText);
        pspMenuSetCaption(item, "Empty");
      }
    } while (0);

    if (path) free(path);
    return 0;
  }

  return OnGenericButtonPress(NULL, NULL, button_mask);
}

/* Handles any special drawing for the system menu */
void OnSystemRender(const void *uiobject, const void *item_obj)
{
  int w, h, x, y;
  w = WIDTH / 2;
  h = HEIGHT / 2;
  x = SCR_WIDTH - w - 8;
  y = SCR_HEIGHT - h - 80;

  /* Draw a small representation of the screen */
  pspVideoShadowRect(x, y, x + w - 1, y + h - 1, PSP_VIDEO_BLACK, 3);
  pspVideoPutImage(Screen, x, y, w, h);
  pspVideoDrawRect(x, y, x + w - 1, y + h - 1, PSP_VIDEO_GRAY);

  OnGenericRender(uiobject, item_obj);
}

static void DisplayStateTab()
{
  if (!GameName) { TabIndex++; return; }

  PspMenuItem *item;
  SceIoStat stat;
  char caption[32];

  const char *config_name = pspFileIoGetFilename(GameName);
  char *path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
  char *game_name = strdup(config_name);
  char *dot = strrchr(game_name, '.');
  if (dot) *dot='\0';

  /* Initialize icons */
  for (item = SaveStateGallery.Menu->First; item; item = item->Next)
  {
    sprintf(path, "%s%s.s%02i", SaveStatePath, config_name,
      (int)item->Userdata);

    if (pspFileIoCheckIfExists(path))
    {
      if (sceIoGetstat(path, &stat) < 0)
        sprintf(caption, "ERROR");
      else
        sprintf(caption, "%02i/%02i/%02i %02i:%02i",
          stat.st_mtime.month,
          stat.st_mtime.day,
          stat.st_mtime.year - (stat.st_mtime.year / 100) * 100,
          stat.st_mtime.hour,
          stat.st_mtime.minute);

      pspMenuSetCaption(item, caption);
      item->Icon = LoadStateIcon(path);
      pspMenuSetHelpText(item, PresentSlotText);
    }
    else
    {
      pspMenuSetCaption(item, "Empty");
      item->Icon = NoSaveIcon;
      pspMenuSetHelpText(item, EmptySlotText);
    }
  }

  free(path);
  pspUiOpenGallery(&SaveStateGallery, game_name);
  free(game_name);

  /* Destroy any icons */
  for (item = SaveStateGallery.Menu->First; item; item = item->Next)
    if (item->Icon != NULL && item->Icon != NoSaveIcon)
      pspImageDestroy((PspImage*)item->Icon);
}

/* Initialize game configuration */
static void InitButtonConfig()
{
  memcpy(&ActiveConfig, &DefaultConfig, sizeof(struct ButtonConfig));
}

/* Load game configuration */
static int LoadButtonConfig()
{
  char *path;
  if (!(path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(ButtonConfigFile) + 6))))
    return 0;
  sprintf(path, "%s%s.cnf", pspGetAppDirectory(), ButtonConfigFile);

  /* Open file for reading */
  FILE *file = fopen(path, "r");
  free(path);

  /* If no configuration, load defaults */
  if (!file)
  {
    InitButtonConfig();
    return 1;
  }

  /* Read contents of struct */
  int nread = fread(&ActiveConfig, sizeof(struct ButtonConfig), 1, file);
  fclose(file);

  if (nread != 1)
  {
    InitButtonConfig();
    return 0;
  }

  return 1;
}

/* Save game configuration */
static int SaveButtonConfig()
{
  char *path;
  if (!(path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(ButtonConfigFile) + 6))))
    return 0;
  sprintf(path, "%s%s.cnf", pspGetAppDirectory(), ButtonConfigFile);

  /* Open file for writing */
  FILE *file = fopen(path, "w");
  free(path);
  if (!file) return 0;

  /* Write contents of struct */
  int nwritten = fwrite(&ActiveConfig, sizeof(struct ButtonConfig), 1, file);
  fclose(file);

  return (nwritten == 1);
}

/* Load options */
void LoadOptions()
{
  char *path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(OptionsFile) + 1));
  sprintf(path, "%s%s", pspGetAppDirectory(), OptionsFile);

  /* Initialize INI structure */
  PspInit *init = pspInitCreate();

  /* Read the file */
  if (!pspInitLoad(init, path))
  {
    /* File does not exist; load defaults */
    InitOptionDefaults();
  }
  else
  {
    /* Load values */
    SmsOptions.DisplayMode = pspInitGetInt(init, "Video", "Display Mode", DISPLAY_MODE_UNSCALED);
    SmsOptions.UpdateFreq = pspInitGetInt(init, "Video", "Update Frequency", 60);
    SmsOptions.Frameskip = pspInitGetInt(init, "Video", "Frameskip", 1);
    SmsOptions.VSync = pspInitGetInt(init, "Video", "VSync", 0);
    SmsOptions.ClockFreq = pspInitGetInt(init, "Video", "PSP Clock Frequency", 222);
    SmsOptions.ShowFps = pspInitGetInt(init, "Video", "Show FPS", 0);
    SmsOptions.ControlMode = pspInitGetInt(init, "Menu", "Control Mode", 0);

    if (GamePath) free(GamePath);
    GamePath = pspInitGetString(init, "File", "Game Path", NULL);
  }

  /* Clean up */
  free(path);
  pspInitDestroy(init);
}

/* Save options */
static int SaveOptions()
{
  char *path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(OptionsFile) + 1));
  sprintf(path, "%s%s", pspGetAppDirectory(), OptionsFile);

  /* Initialize INI structure */
  PspInit *init = pspInitCreate();

  /* Set values */
  pspInitSetInt(init, "Video", "Display Mode", SmsOptions.DisplayMode);
  pspInitSetInt(init, "Video", "Update Frequency", SmsOptions.UpdateFreq);
  pspInitSetInt(init, "Video", "Frameskip", SmsOptions.Frameskip);
  pspInitSetInt(init, "Video", "VSync", SmsOptions.VSync);
  pspInitSetInt(init, "Video", "PSP Clock Frequency",SmsOptions.ClockFreq);
  pspInitSetInt(init, "Video", "Show FPS", SmsOptions.ShowFps);
  pspInitSetInt(init, "Menu", "Control Mode", SmsOptions.ControlMode);

  if (GamePath) pspInitSetString(init, "File", "Game Path", GamePath);

  /* Save INI file */
  int status = pspInitSave(init, path);

  /* Clean up */
  pspInitDestroy(init);
  free(path);

  return status;
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
  GamePath = NULL;
}

/* Load state icon */
PspImage* LoadStateIcon(const char *path)
{
  /* Open file for reading */
  FILE *f = fopen(path, "r");
  if (!f) return NULL;

  /* Load image */
  PspImage *image = pspImageLoadPngOpen(f);
  fclose(f);

  return image;
}

/* Load state */
int LoadState(const char *path)
{
  /* Open file for reading */
  FILE *f = fopen(path, "r");
  if (!f) return 0;

  /* Load image into temporary object */
  PspImage *image = pspImageLoadPngOpen(f);
  pspImageDestroy(image);

  system_load_state(f);
  fclose(f);

  return 1;
}

/* Save state */
PspImage* SaveState(const char *path, PspImage *icon)
{
  /* Open file for writing */
  FILE *f;
  if (!(f = fopen(path, "w")))
    return NULL;

  /* Create thumbnail */
  PspImage *thumb;
  thumb = (icon->Viewport.Width < 200)
    ? pspImageCreateCopy(icon) : pspImageCreateThumbnail(icon);
  if (!thumb) { fclose(f); return NULL; }

  /* Write the thumbnail */
  if (!pspImageSavePngOpen(f, thumb))
  {
    pspImageDestroy(thumb);
    fclose(f);
    return NULL;
  }

  /* Save state */
  system_save_state(f);

  fclose(f);
  return thumb;
}

/* Release menu resources */
void TrashMenu()
{
  TrashEmulator();

  /* Save options */
  SaveOptions();

  /* Trash menus */
  pspMenuDestroy(OptionUiMenu.Menu);
  pspMenuDestroy(ControlUiMenu.Menu);
  pspMenuDestroy(SaveStateGallery.Menu);
  pspMenuDestroy(SystemUiMenu.Menu);

  /* Trash images */
  if (Background) pspImageDestroy(Background);
  if (NoSaveIcon) pspImageDestroy(NoSaveIcon);

  if (GameName) free(GameName);
  if (GamePath) free(GamePath);

  free(ScreenshotPath);
  free(SaveStatePath);
}

/* Save or load SRAM */
void system_manage_sram(uint8 *sram, int slot, int mode)
{
  FILE *fd;
  const char *config_name = pspFileIoGetFilename(GameName);
  char *path = (char*)malloc(sizeof(char)
    * (strlen(SaveStatePath) + strlen(config_name) + 8));
  sprintf(path, "%s%s.srm", SaveStatePath, config_name);

  switch(mode)
  {
  case SRAM_SAVE:
    if(sms.save)
    {
      fd = fopen(path, "w");
      if(fd)
      {
        fwrite(sram, 0x8000, 1, fd);
        fclose(fd);
      }
    }
    break;

  case SRAM_LOAD:
    fd = fopen(path, "r");
    if(fd)
    {
      sms.save = 1;
      fread(sram, 0x8000, 1, fd);
      fclose(fd);
    }
    else
    {
      /* No SRAM file, so initialize memory */
      memset(sram, 0x00, 0x8000);
    }
    break;
  }

  free(path);
}
