/*

Time rewind

Original Author: davex
e-mail: efengeler@gmail.com

Adopted and modified for psplib by Akop Karapetyan (dev@psp.akop.org)
*/

#include <stdlib.h>
#include <stdio.h>

#include "rewind.h"

typedef struct rewind_state
{
  void *data;
  struct rewind_state *prev;
  struct rewind_state *next;
} rewind_state_t;

static int get_psp_max_free_memory();

int pl_rewind_init(pl_rewind *rewind,
  int (*save_state)(void *),
  int (*load_state)(void *),
  int (*get_state_size)())
{
	float memory_needed = (float)get_psp_max_free_memory() * 0.85;
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
  rewind->start = rewind->current = head;
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
  rewind->current->prev->next = NULL; /* Cut off loop */
	rewind_state_t *curr, *next;

  for (curr = rewind->current; curr; curr = next)
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
  rewind->start = rewind->current;
}

int pl_rewind_save(pl_rewind *rewind)
{
	if (!rewind->save_state(rewind->current->data))
    return 0;

	rewind->current = rewind->current->next;

  /* Move starting point forward */
  if (rewind->current == rewind->start)
    rewind->start = rewind->current->next;

  return 1;
}

int pl_rewind_restore(pl_rewind *rewind)
{
  /* Can't go past the starting point */
  if (rewind->current == rewind->start)
    return 0;

  rewind->current = rewind->current->prev;
  return rewind->load_state(rewind->current->data);
}

static int get_psp_max_free_memory()
{
	unsigned char *mem;
	const int MEM_CHUNK_SIZE = 500*1024; //blocks of 500KBytes
	const int MAX_CHUNKS = 64;
	unsigned char* mem_reserv[MAX_CHUNKS];
	int total_mem = 0;
	int i = 0;
	
	//initializes
	for( i = 0; i< MAX_CHUNKS; i++){
		mem_reserv[i] = NULL;
	}
	
	//allocate
	for( i=0; i<MAX_CHUNKS; i++){
		mem = (unsigned char *)malloc(MEM_CHUNK_SIZE);
		if( mem != NULL){
			total_mem += MEM_CHUNK_SIZE;
			mem_reserv[i] = mem;
		}else{
			break;
		}
	}
	
	//free
	for( i=0; i<MAX_CHUNKS; i++){
		if( mem_reserv[i] != NULL)
			free( mem_reserv[i] );
	}
	
	return total_mem;
}

