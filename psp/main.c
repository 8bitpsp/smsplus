#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"

PSP_MODULE_INFO("SMS Plus PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

static void ExitCallback(void* arg)
{
  ExitPSP = 1;
}

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

  /* Redirect stdout/stderr */
  //sceKernelStdoutReopen("ms0:/stdout.txt", PSP_O_WRONLY, 0777);
  //sceKernelStderrReopen("ms0:/stderr.txt", PSP_O_WRONLY, 0777);

  /* Release PSP resources */
  pspAudioEnd();
  pspVideoShutdown();
  pspShutdown();

  return(0);
}

