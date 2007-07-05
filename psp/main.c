#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"

#include "menu.h"

PSP_MODULE_INFO("SMS Plus PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

static void ExitCallback(void* arg)
{
  ExitPSP = 1;
}

int main(int argc,char *argv[])
{
  /* Initialize PSP */
  pspInit(argv[0]);
  pspAudioInit(768);
  pspCtrlInit();
  pspVideoInit();

  /* Initialize callbacks */
  pspRegisterCallback(PSP_EXIT_CALLBACK, ExitCallback, NULL);
  pspStartCallbackThread();

  /* Start emulation */
  InitMenu();
  DisplayMenu();
  TrashMenu();

  /* Release PSP resources */
  pspAudioShutdown();
  pspVideoShutdown();
  pspShutdown();

  return(0);
}
