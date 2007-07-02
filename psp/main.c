#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"

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

  snd.fm_which = SND_EMU2413;
  snd.fps = FPS_NTSC;
  snd.fm_clock = CLOCK_NTSC;
  snd.psg_clock = CLOCK_NTSC;
  snd.sample_rate = 0; //44100;
  snd.mixer_callback = NULL;

  cart.rom = NULL;

  load_rom("sonic.sms");

  /* Initialize the virtual console emulation */
  system_init();
//  sms.territory = TERRITORY_EXPORT;
//  sms.use_fm = 0;

  system_poweron();

  running = 1;

  /* Main emulation loop */
  while(running)
  {
      /* Bump frame count *
      frame_count++;
      frames_rendered++;
      skip = (frame_count % frame_skip == 0) ? 0 : 1;

      /* Get current input *
      osd_update_inputs();
      check_ui_keys();

      /* Run the system emulation for a frame */
      system_frame(0); //skip);

      /* Update the display */
//      osd_play_streamed_sample_16(0, snd.output[0], snd.buffer_size, option.sndrate, 60, -100);
//      osd_play_streamed_sample_16(1, snd.output[1], snd.buffer_size, option.sndrate, 60,  100);
      osd_update_video();
  }

  system_poweroff();
  system_shutdown();

  if (Screen) pspImageDestroy(Screen);





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

  pspVideoEnd();

  /* Wait for VSync signal */
  pspVideoWaitVSync();

  /* Swap buffers */
  pspVideoSwapBuffers();
}

/* Save or load SRAM */
void system_manage_sram(uint8 *sram, int slot, int mode)
{
  memset(sram, 0x00, 0x8000);
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

