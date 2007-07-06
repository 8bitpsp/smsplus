#include "emumain.h"

#include <psptypes.h>
#include "time.h"
#include <psprtc.h>

#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"
#include "perf.h"

#include "shared.h"
#include "sound.h"
#include "system.h"

PspImage *Screen;

static PspFpsCounter FpsCounter;
static int ClearScreen;
static int ScreenX, ScreenY, ScreenW, ScreenH;
static int TicksPerUpdate;
static u32 TicksPerSecond;
static u64 LastTick;
static u64 CurrentTick;
static int Frame;

extern char *GameName;
extern EmulatorOptions SmsOptions;

inline int ParseInput();
inline void RenderVideo();
void AudioCallback(void* buf, unsigned int *length, void *userdata);

void InitEmulator()
{
  ClearScreen = 0;

  /* Initialize screen buffer */
  Screen = pspImageCreate(256, 192);
  pspImageClear(Screen, 0x8000);

  /* Set up bitmap structure */
  memset(&bitmap, 0, sizeof(bitmap_t));
  bitmap.width  = Screen->Width;
  bitmap.height = Screen->Height;
  bitmap.depth  = 16;
  bitmap.granularity = (bitmap.depth >> 3);
  bitmap.pitch  = bitmap.width * bitmap.granularity;
  bitmap.data   = (uint8 *)Screen->Pixels;
  bitmap.viewport.x = 0;
  bitmap.viewport.y = 0;
  bitmap.viewport.w = 256;
  bitmap.viewport.h = 192;

  /* Initialize sound structure */
  snd.fm_which = SND_EMU2413;
  snd.fps = FPS_NTSC;
  snd.fm_clock = CLOCK_NTSC;
  snd.psg_clock = CLOCK_NTSC;
  snd.sample_rate = 44100;
  snd.mixer_callback = NULL;

  sms.territory = TERRITORY_EXPORT;
}

void RunEmulator()
{
  float ratio;

  /* Recompute screen size/position */
  switch (SmsOptions.DisplayMode)
  {
  default:
  case DISPLAY_MODE_UNSCALED:
    ScreenW = Screen->Width;
    ScreenH = Screen->Height;
    break;
  case DISPLAY_MODE_FIT_HEIGHT:
    ratio = (float)SCR_HEIGHT / (float)Screen->Height;
    ScreenW = (float)Screen->Width * ratio;
    ScreenH = SCR_HEIGHT;
    break;
  case DISPLAY_MODE_FILL_SCREEN:
    ScreenW = SCR_WIDTH;
    ScreenH = SCR_HEIGHT;
    break;
  }
  ScreenX = (SCR_WIDTH / 2) - (ScreenW / 2);
  ScreenY = (SCR_HEIGHT / 2) - (ScreenH / 2);

  /* Init performance counter */
  pspPerfInitFps(&FpsCounter);

  /* Recompute update frequency */
  TicksPerSecond = sceRtcGetTickResolution();
  if (SmsOptions.UpdateFreq)
  {
    TicksPerUpdate = TicksPerSecond
      / (SmsOptions.UpdateFreq / (SmsOptions.Frameskip + 1));
    sceRtcGetCurrentTick(&LastTick);
  }
  Frame = 0;
  ClearScreen = 1;

  /* Resume sound */
  pspAudioSetChannelCallback(0, AudioCallback, 0);

  /* Wait for V. refresh */
  pspVideoWaitVSync();

  /* Main emulation loop */
  while (!ExitPSP)
  {
    /* Check input */
    if (ParseInput()) break;

    /* Run the system emulation for a frame */
    if (++Frame <= SmsOptions.Frameskip)
    {
      /* Skip frame */
      system_frame(1);
    }
    else
    {
      system_frame(0);
      Frame = 0;

      /* Display */
      RenderVideo();
    }
  }

  /* Stop sound */
  pspAudioSetChannelCallback(0, NULL, 0);
}

void TrashEmulator()
{
  /* Trash screen */
  if (Screen) pspImageDestroy(Screen);

  if (GameName)
  {
    /* Release emulation resources */
    system_poweroff();
    system_shutdown();
  }
}

int ParseInput()
{
  /* Reset input */
  input.system    = 0;
  input.pad[0]    = 0;
  input.pad[1]    = 0;
  input.analog[0] = 0x7F;
  input.analog[1] = 0x7F;

  static SceCtrlData pad;

  /* Check the input */
  if (pspCtrlPollControls(&pad))
  {
    if((pad.Buttons & PSP_CTRL_LTRIGGER)
      && (pad.Buttons & PSP_CTRL_RTRIGGER)) return 1;

    if(pad.Buttons & PSP_CTRL_ANALUP) input.pad[0] |= INPUT_UP;
    else if(pad.Buttons & PSP_CTRL_ANALDOWN) input.pad[0] |= INPUT_DOWN;

    if(pad.Buttons & PSP_CTRL_ANALLEFT) input.pad[0] |= INPUT_LEFT;
    else if(pad.Buttons & PSP_CTRL_ANALRIGHT) input.pad[0] |= INPUT_RIGHT;

    if(pad.Buttons & PSP_CTRL_CROSS)  input.pad[0] |= INPUT_BUTTON2;
    if(pad.Buttons & PSP_CTRL_SQUARE)  input.pad[0] |= INPUT_BUTTON1;

    if(pad.Buttons & PSP_CTRL_START)  input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;
  }

  return 0;
}

void RenderVideo()
{
  /* Update the display */
  pspVideoBegin();

  /* Clear the buffer first, if necessary */
  if (ClearScreen >= 0)
  {
    ClearScreen--;
    pspVideoClearScreen();
  }

  /* Draw the screen */
  pspVideoPutImageDirect(Screen, ScreenX, ScreenY, ScreenW, ScreenH);

  /* Show FPS counter */
  if (SmsOptions.ShowFps)
  {
    /* AKTODO */
    static char fps_display[32];
    sprintf(fps_display, " %3.02f", pspPerfGetFps(&FpsCounter));

    int width = pspFontGetTextWidth(&PspStockFont, fps_display);
    int height = pspFontGetLineHeight(&PspStockFont);

    pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_VIDEO_BLACK);
    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_VIDEO_WHITE);
  }

  pspVideoEnd();

  /* Wait if needed */
  if (SmsOptions.UpdateFreq)
  {
    do { sceRtcGetCurrentTick(&CurrentTick); }
    while (CurrentTick - LastTick < TicksPerUpdate);
    LastTick = CurrentTick;
  }

  /* Wait for VSync signal */
  if (SmsOptions.VSync) pspVideoWaitVSync();

  /* Swap buffers */
  pspVideoSwapBuffers();
}

void AudioCallback(void* buf, unsigned int *length, void *userdata)
{
  PspSample *OutBuf = (PspSample*)buf;
  int i;

  for(i = 0; i < *length; i++) 
  {
    OutBuf[i].Left = snd.output[0][i] * 2;
    OutBuf[i].Right = snd.output[1][i] * 2;
  }
}

