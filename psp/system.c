#include "shared.h"

/* Save or load SRAM */
void system_manage_sram(uint8 *sram, int slot, int mode)
{
  /* STUB */
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
