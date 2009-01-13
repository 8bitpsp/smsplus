/* psplib/pl_rewind.c
   State rewinding routines

   Copyright (C) 2009 Akop Karapetyan
   Based on code by DaveX (efengeler@gmail.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Author contact information: dev@psp.akop.org
*/

#include <stdlib.h>
#include <stdio.h>

#include "pl_rewind.h"

typedef struct rewind_state
{
  void *data;
  struct rewind_state *prev;
  struct rewind_state *next;
} rewind_state_t;

static int get_free_memory();

int pl_rewind_init(pl_rewind *rewind,
  int (*save_state)(void *),
  int (*load_state)(void *),
  int (*get_state_size)())
{
	float memory_needed = (float)get_free_memory() * 0.85;
  int state_data_size = get_state_size();
  int state_count = (int)(memory_needed 
                    / (float)(state_data_size + sizeof(rewind_state_t)));

	if (state_count <= 0)
    return 0;

  /* First state */
  rewind_state_t *head, *prev, *curr;
  if (!(head = (rewind_state_t*)malloc(sizeof(rewind_state_t))))
    return 0;
  head->data = malloc(state_data_size);
  prev = head;

  /* The rest */
  int i;
  for (i = 1; i < state_count; i++)
  {
    curr = (rewind_state_t*)malloc(sizeof(rewind_state_t));
    curr->data = malloc(state_data_size);
    prev->next = curr;
    curr->prev = prev;
    prev = curr;
  }

  /* Make circular */
  head->prev = prev;
  prev->next = head;

  /* Init structure */
  rewind->start = head;
  rewind->current = NULL;
  rewind->save_state = save_state;
  rewind->load_state = load_state;
  rewind->get_state_size = get_state_size;
  rewind->state_data_size = state_data_size;
  rewind->state_count = state_count;

  return 1;
}

void pl_rewind_realloc(pl_rewind *rewind)
{
  pl_rewind_destroy(rewind);
  pl_rewind_init(rewind,
    rewind->save_state,
    rewind->load_state,
    rewind->get_state_size);
}

void pl_rewind_destroy(pl_rewind *rewind)
{
  rewind->start->prev->next = NULL; /* Prevent infinite loop */
	rewind_state_t *curr, *next;

  for (curr = rewind->start; curr; curr = next)
  {
    next = curr->next;
    free(curr->data);
    free(curr);
  }

  rewind->start = NULL;
  rewind->current = NULL;
}

void pl_rewind_reset(pl_rewind *rewind)
{
  rewind->current = NULL;
}

int pl_rewind_save(pl_rewind *rewind)
{
  rewind_state_t *slot = 
    (rewind->current) ? rewind->current->next
                      : rewind->start;

	if (!rewind->save_state(slot->data))
    return 0;

	rewind->current = slot;

  /* Move starting point forward, if we've reached the start node */
  if (slot == rewind->start)
    rewind->start = slot->next;

  return 1;
}

int pl_rewind_restore(pl_rewind *rewind)
{
  /* Can't go past the starting point */
  if (!rewind->current)
    return 0;

  rewind_state_t *slot = rewind->current;
  rewind->current = 
    (slot == rewind->start) ? NULL
                            : rewind->current->prev;

  return rewind->load_state(slot->data);
}

static int get_free_memory()
{
  const int 
    chunk_size = 65536, // 64 kB
    chunks = 1024; // 65536 * 1024 = 64 MB
  void *mem_reserv[chunks];
  int total_mem = 0, i;

  /* Initialize */	
  for (i = 0; i < chunks; i++)
    mem_reserv[i] = NULL;

  /* Allocate */
  for (i = 0; i < chunks; i++)
  {
    if (!(mem_reserv[i] = malloc(chunk_size)))
      break;

    total_mem += chunk_size;
  }

  /* Free */
  for (i = 0; i < chunks; i++)
  {
    if (!mem_reserv[i])
      break;
    free(mem_reserv[i]);
  }
	
	return total_mem;
}

