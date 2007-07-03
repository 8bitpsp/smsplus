#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"
#include "perf.h"

#include "shared.h"
#include "sound.h"
#include "system.h"

PSP_MODULE_INFO("SMS Plus PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

int running;
PspImage *Screen;

static void ExitCallback(void* arg)
{
  ExitPSP = 1;
  running = 0;
}

static PspFpsCounter FpsCounter;
static int fr=0;

void osd_update_video(void);

/** main() ***************************************************/
/** This is a main() function used in PSP port.             **/
/*************************************************************/
int main(int argc,char *argv[])
{
  /* Initialize PSP */
  pspInit(argv[0]);
  pspAudioInit();
  pspCtrlInit();
  pspVideoInit();

  /* Initialize callbacks */
  pspRegisterCallback(PSP_EXIT_CALLBACK, ExitCallback, NULL);
  pspStartCallbackThread();





    /* Set clock frequency during emulation */
    pspSetClockFrequency(300);

  pspPerfInitFps(&FpsCounter);

  load_rom("sonic.sms");

    /* Set up bitmap structure */
//    bitmap.data   = (uint8 *)&sms_bmp->line[0][0];

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

  snd.fm_which = SND_YM2413;// SND_EMU2413;
  snd.fps = FPS_NTSC;
  snd.fm_clock = CLOCK_NTSC;
  snd.psg_clock = CLOCK_NTSC;
  snd.sample_rate = 44100;
  snd.mixer_callback = NULL;

  /* Initialize the virtual console emulation */
  system_init();
//  sms.territory = TERRITORY_EXPORT;
//  sms.use_fm = 0;

  system_poweron();

  running = 1;
FILE *foo;
foo=fopen("ms0:/log.txt","w");
fprintf(foo,"init\n");
fclose(foo);

  /* Main emulation loop */
  while(running)
  {
    input.system    = 0;
    input.pad[0]    = 0;
    input.pad[1]    = 0;
    input.analog[0] = 0x7F;
    input.analog[1] = 0x7F;

  SceCtrlData pad;
  /* Check the input */
  if (pspCtrlPollControls(&pad))
  {
        if(pad.Buttons & PSP_CTRL_ANALUP) input.pad[0] |= INPUT_UP;
        else
        if(pad.Buttons & PSP_CTRL_ANALDOWN) input.pad[0] |= INPUT_DOWN;
        if(pad.Buttons & PSP_CTRL_ANALLEFT) input.pad[0] |= INPUT_LEFT;
        else
        if(pad.Buttons & PSP_CTRL_ANALRIGHT) input.pad[0] |= INPUT_RIGHT;
        if(pad.Buttons & PSP_CTRL_CROSS)  input.pad[0] |= INPUT_BUTTON2;
        if(pad.Buttons & PSP_CTRL_SQUARE)  input.pad[0] |= INPUT_BUTTON1;
        if(pad.Buttons & PSP_CTRL_START)  input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;
  }


      /* Bump frame count *
      frame_count++;
      frames_rendered++;
      skip = (frame_count % frame_skip == 0) ? 0 : 1;

      /* Get current input *
      osd_update_inputs();
      check_ui_keys();
      */
if(fr>=1279)
{
foo=fopen("ms0:/log.txt","a");
fprintf(foo,"draw..");
fclose(foo);
}

      /* Run the system emulation for a frame */
      system_frame(0); //skip);
if(fr>=1279)
{
foo=fopen("ms0:/log.txt","a");
fprintf(foo,"er..");
fclose(foo);
}

      /* Update the display */
//      osd_play_streamed_sample_16(0, snd.output[0], snd.buffer_size, option.sndrate, 60, -100);
//      osd_play_streamed_sample_16(1, snd.output[1], snd.buffer_size, option.sndrate, 60,  100);
      osd_update_video();
if(fr>=1279)
{
foo=fopen("ms0:/log.txt","a");
fprintf(foo,"ring.\n");
fclose(foo);
}
  }

  system_poweroff();
  system_shutdown();

  if (Screen) pspImageDestroy(Screen);

  /* Set normal clock frequency */
  pspSetClockFrequency(222);




  /* Release PSP resources */
  pspAudioEnd();
  pspVideoShutdown();
  pspShutdown();

  return(0);
}

/* Update video */
void osd_update_video(void)
{
  pspVideoBegin();

  /* Draw the screen */
  pspVideoPutImage(Screen, 0, 0, Screen->Width, Screen->Height);

    float fps = pspPerfGetFps(&FpsCounter);
    static char fps_display[16];
    sprintf(fps_display, " %3.02f (fr %i) ", fps,fr++);

    int width = pspFontGetTextWidth(&PspStockFont, fps_display);
    int height = pspFontGetLineHeight(&PspStockFont);

    pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_VIDEO_BLACK);
    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_VIDEO_WHITE);

  pspVideoEnd();

  /* Wait for VSync signal */
//  pspVideoWaitVSync();

  /* Swap buffers */
  pspVideoSwapBuffers();
}

/* Save or load SRAM */
void system_manage_sram(uint8 *sram, int slot, int mode)
{
  switch(mode)
  {
  case SRAM_SAVE:
    break;
  case SRAM_LOAD:
    memset(sram, 0x00, 0x8000);
    break;
  }
/*
    char name[PATH_MAX];
    FILE *fd;
    strcpy(name, game_name);
    strcpy(strrchr(name, '.'), ".sav");

    switch(mode)
    {
        case SRAM_SAVE:
            if(sms.save)
            {
                fd = fopen(name, "wb");
                if(fd)
                {
                    fwrite(sram, 0x8000, 1, fd);
                    fclose(fd);
                }
            }
            break;

        case SRAM_LOAD:
            fd = fopen(name, "rb");
            if(fd)
            {
                sms.save = 1;
                fread(sram, 0x8000, 1, fd);
                fclose(fd);
            }
            else
            {
                /* No SRAM file, so initialize memory *
                memset(sram, 0x00, 0x8000);
            }
            break;
    }
*/
}

