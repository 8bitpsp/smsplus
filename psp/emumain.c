#include "emumain.h"

#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"
#include "perf.h"

#include "shared.h"
#include "sound.h"
#include "system.h"

/* AKTODO */
int fr=1281;

PspImage *Screen;

static PspFpsCounter FpsCounter;
extern int ShowFPS;

inline void ParseInput();
inline void RenderVideo();
inline void RenderAudio();

void InitEmulator()
{
  /* Initialize screen buffer */
  Screen = pspImageCreate(256, 256);
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

/* AKTODO: */
FILE*foo=fopen("ms0:/log.txt","w");
fclose(foo);
load_rom("sonic.sms");

  /* Initialize the virtual console emulation */
  system_init();

  sms.territory = TERRITORY_EXPORT;

  system_poweron();
}
void AudioCallback(void* buf, unsigned int *length, void *userdata);

void RunEmulator()
{
  /* Set clock frequency during emulation */
  pspSetClockFrequency(300);

  /* Init performance counter */
  pspPerfInitFps(&FpsCounter);
pspAudioSetChannelCallback(0, AudioCallback, 0);
  /* Main emulation loop */
  while (!ExitPSP)
  {
    /* Check input */
    ParseInput();

    /* Run the system emulation for a frame */
    system_frame(0);

    /* Sound */
//    RenderAudio();

    /* Display */
    RenderVideo();
  }

  /* Set normal clock frequency */
  pspSetClockFrequency(222);
}

void TrashEmulator()
{
  /* Trash screen */
  if (Screen) pspImageDestroy(Screen);

  /* Release emulation resources */
  system_poweroff();
  system_shutdown();
}

void ParseInput()
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
    if(pad.Buttons & PSP_CTRL_ANALUP) input.pad[0] |= INPUT_UP;
    else if(pad.Buttons & PSP_CTRL_ANALDOWN) input.pad[0] |= INPUT_DOWN;

    if(pad.Buttons & PSP_CTRL_ANALLEFT) input.pad[0] |= INPUT_LEFT;
    else if(pad.Buttons & PSP_CTRL_ANALRIGHT) input.pad[0] |= INPUT_RIGHT;

    if(pad.Buttons & PSP_CTRL_CROSS)  input.pad[0] |= INPUT_BUTTON2;
    if(pad.Buttons & PSP_CTRL_SQUARE)  input.pad[0] |= INPUT_BUTTON1;

    if(pad.Buttons & PSP_CTRL_START)  input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;
  }
}

void RenderVideo()
{
  /* Update the display */
  pspVideoBegin();

  /* Draw the screen */
  pspVideoPutImage(Screen, 0, 0, Screen->Width, Screen->Height);

  /* Show FPS counter */
  if (ShowFPS)
  {
    /* AKTODO */
    static char fps_display[32];
    sprintf(fps_display, " %3.02f (fr %i)", pspPerfGetFps(&FpsCounter), fr--);
    
    int width = pspFontGetTextWidth(&PspStockFont, fps_display);
    int height = pspFontGetLineHeight(&PspStockFont);
    
    pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_VIDEO_BLACK);
    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_VIDEO_WHITE);
  }

  pspVideoEnd();

  /* Wait for VSync signal */
//  pspVideoWaitVSync();

  /* Swap buffers */
  pspVideoSwapBuffers();
}

void AudioCallback(void* buf, unsigned int *length, void *userdata)
{
  PspSample *OutBuf = (PspSample*)buf;
  int i;

  for(i=0;i<*length;i++) 
  {
    OutBuf[i].Left = snd.output[0][i];
    OutBuf[i].Right = snd.output[1][i];
  }
}

