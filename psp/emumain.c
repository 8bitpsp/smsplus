#include "emumain.h"

#include <psptypes.h>
#include "time.h"
#include <psprtc.h>

#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"
#include "perf.h"
#include "util.h"

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
extern const u64 ButtonMask[];
extern const int ButtonMapId[];
extern struct ButtonConfig ActiveConfig;
extern char *ScreenshotPath;

inline int ParseInput();
inline void RenderVideo();
void AudioCallback(void* buf, unsigned int *length, void *userdata);

void InitEmulator()
{
  ClearScreen = 0;

  /* Initialize screen buffer */
  Screen = pspImageCreateVram(256, 192);

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
    ScreenW = Screen->Viewport.Width;
    ScreenH = Screen->Viewport.Height;
    break;
  case DISPLAY_MODE_FIT_HEIGHT:
    ratio = (float)SCR_HEIGHT / (float)Screen->Viewport.Height;
    ScreenW = (float)Screen->Viewport.Width * ratio;
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
    //* DEBUGGING
    if ((pad.Buttons & (PSP_CTRL_SELECT | PSP_CTRL_START))
      == (PSP_CTRL_SELECT | PSP_CTRL_START))
        pspUtilSaveVramSeq(ScreenshotPath, "game");
    //*/

    /* Parse input */
    int i, on, code;
    for (i = 0; ButtonMapId[i] >= 0; i++)
    {
      code = ActiveConfig.ButtonMap[ButtonMapId[i]];
      on = (pad.Buttons & ButtonMask[i]) == ButtonMask[i];

      /* Check to see if a button set is pressed. If so, unset it, so it */
      /* doesn't trigger any other combination presses. */
      if (on) pad.Buttons &= ~ButtonMask[i];

      if (code & JOY)
      {
        if (on) input.pad[0] |= CODE_MASK(code);
      }
      else if (code & SYS)
      {
        if (on)
        {
          if (CODE_MASK(code) == (INPUT_START | INPUT_PAUSE))
            input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;
        }
      }
      else if (code & SPC)
      {
        switch (CODE_MASK(code))
        {
        case SPC_MENU:
          if (on) return 1;
          break;
        }
      }
    }
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
