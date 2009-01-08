/*
Author: davex
e-mail: efengeler@gmail.com
*/

#ifndef _PL_REWIND_H
#define _PL_REWIND_H

struct rewind_state;

typedef struct
{
  int state_data_size;
  int state_count;
  struct rewind_state *start;
  struct rewind_state *current;
  int (*save_state)(void *);
  int (*load_state)(void *);
  int (*get_state_size)();
} pl_rewind;

int  pl_rewind_init(pl_rewind *rewind,
  int (*save_state)(void *),
  int (*load_state)(void *),
  int (*get_state_size)());
void pl_rewind_realloc(pl_rewind *rewind);
void pl_rewind_destroy(pl_rewind *rewind);
void pl_rewind_reset(pl_rewind *rewind);
int  pl_rewind_save(pl_rewind *rewind);
int  pl_rewind_restore(pl_rewind *rewind);

#endif // _PL_REWIND_H
