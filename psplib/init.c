/** PSP helper library ***************************************/
/**                                                         **/
/**                         init.c                          **/
/**                                                         **/
/** This file contains routines for INI file loading/saving **/
/** Its idea is based on a similar library by Marat         **/
/** Fayzullin                                               **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "init.h"

#include <string.h>
#include <malloc.h>
#include <stdio.h>

PspInit* pspInitCreate()
{
  PspInit *init = (PspInit*)malloc(sizeof(PspInit));
  if (!init) return NULL;

  init->Head = NULL;

  return init;
}

int pspInitParse(PspInit *init, const char *file)
{
  FILE *file;
  if (!(file = fopen(file, "r"))) return 0;

  while (!feof(file))
  {
    
  }

  fclose(file);
  return 1;
}

void pspInitDestroy(PspInit *init)
{
  free(init);
}
