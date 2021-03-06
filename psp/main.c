#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "pl_snd.h"
#include "video.h"
#include "pl_psp.h"
#include "ctrl.h"

#include "menu.h"

PSP_MODULE_INFO(PSP_APP_NAME, 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

static void ExitCallback(void* arg)
{
  ExitPSP = 1;
}

int main(int argc,char *argv[])
{
  /* Initialize PSP */
  pl_psp_init(argv[0]);
  pl_snd_init(768, 1);
  pspCtrlInit();
  pspVideoInit();

  /* Initialize callbacks */
  pl_psp_register_callback(PSP_EXIT_CALLBACK,
                           ExitCallback,
                           NULL);
  pl_psp_start_callback_thread();

  /* Start emulation */
  InitMenu();
  DisplayMenu();
  TrashMenu();

  /* Release PSP resources */
  pl_snd_shutdown();
  pspVideoShutdown();
  pl_psp_shutdown();

  return(0);
}
