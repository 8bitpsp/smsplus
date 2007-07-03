#include "menu.h"
#include "emumain.h"

#include "image.h"

extern PspImage *Screen;

int ShowFPS;

void InitMenu()
{
  InitEmulator();

  ShowFPS = 1;
}

void DisplayMenu()
{
  RunEmulator();
}

void TrashMenu()
{
  TrashEmulator();
}
