/** PSP helper library ***************************************/
/**                                                         **/
/**                          menu.h                         **/
/**                                                         **/
/** This file contains declarations for a generic menu      **/
/** system                                                  **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PSP_MENU_H
#define _PSP_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PspMenuOption
{
  const void* Value;
  char* Text;
  struct PspMenuOption *Next;
  struct PspMenuOption *Prev;
} PspMenuOption;

typedef struct PspMenuItem
{
  char* Caption;
  const void *Icon;
  const void *Userdata;
  PspMenuOption *Options;
  const PspMenuOption *Selected;
  struct PspMenuItem *Next;
  struct PspMenuItem *Prev;
  char *HelpText;
} PspMenuItem;

typedef struct PspMenu
{
  PspMenuItem* First;
  PspMenuItem* Last;
  int Count;
  PspMenuItem* Selected;
} PspMenu;

typedef struct PspMenuOptionDef
{
  const char *Text;
  void *Value;
} PspMenuOptionDef;

typedef struct PspMenuItemDef
{
  const char *Caption;
  void *Userdata;
  const PspMenuOptionDef *OptionList;
  int   SelectedIndex;
  const char *HelpText;
} PspMenuItemDef;


PspMenu*       pspMenuCreate();
void           pspMenuLoad(PspMenu *menu, const PspMenuItemDef *def);
void           pspMenuClear(PspMenu* menu);
void           pspMenuDestroy(PspMenu* menu);
PspMenuItem*   pspMenuAppendItem(PspMenu* menu, const char* caption, const void *userdata);
PspMenuOption* pspMenuAppendOption(PspMenuItem *item, const char *text, const void *value, int select);
void           pspMenuSelectOptionByIndex(PspMenuItem *item, int index);
void           pspMenuSelectOptionByValue(PspMenuItem *item, const void *value);
void           pspMenuModifyOption(PspMenuOption *option, const char *text, const void *value);
void           pspMenuClearOptions(PspMenuItem* item);
PspMenuItem*   pspMenuGetNthItem(PspMenu *menu, int index);
PspMenuItem*   pspMenuFindItemByUserdata(PspMenu *menu, const void *userdata);
void           pspMenuSetCaption(PspMenuItem *item, const char *caption);
void           pspMenuSetHelpText(PspMenuItem *item, const char *helptext);

#ifdef __cplusplus
}
#endif

#endif  // _PSP_MENU_H
