#ifndef _EMUMAIN_H
#define _EMUMAIN_H

void InitEmulator();
void RunEmulator();
void TrashEmulator();

typedef struct
{
  int ShowFps;
  int ControlMode;
  int ClockFreq;
  int DisplayMode;
  int VSync;
  int UpdateFreq;
  int Frameskip;
} EmulatorOptions;

#define DISPLAY_MODE_UNSCALED    0
#define DISPLAY_MODE_FIT_HEIGHT  1
#define DISPLAY_MODE_FILL_SCREEN 2

#endif // _EMUMAIN_H
