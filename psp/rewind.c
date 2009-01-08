#include <stdlib.h>

#include "rewind.h"

struct rewind_state
{
  void *data;
  struct rewind_state *prev;
  struct rewind_state *next;
};

static int get_psp_max_free_memory();

int pl_rewind_init(pl_rewind *rewind,
  int (*save_state)(void *),
  int (*load_state)(void *),
  int (*get_state_size)())
{
	int memory_needed = (int)((float)get_psp_max_free_memory() * 0.85);
  int state_data_size = get_state_size();
  int state_count = (int)((float)memory_needed / (float)state_data_size);

	if (state_count <= 0)
    return 0;

  /* First state */
  struct rewind_state *head, *prev, *cur;
  if (!(head = (struct rewind_state*)malloc(sizeof(struct rewind_state))))
    return 0;
  head->data = malloc(state_data_size);
  prev = head;

  /* The rest */
  int i;
  for (i = 1; i < state_count; i++)
  {
    cur = (struct rewind_state*)malloc(sizeof(struct rewind_state));
    cur->data = malloc(state_data_size);
    prev->next = cur;
    cur->prev = prev;
    prev = cur;
  }

  /* Make circular */
  head->prev = prev;
  prev->next = head;

  /* Set selected node */
  rewind->start = head;
  rewind->current = rewind->start;

  /* */
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
	struct rewind_state *next;
  rewind->current->prev->next = NULL;

	while (rewind->current)
  {
    next = rewind->current->next;
    free(rewind->current->data);
    free(rewind->current);
    rewind->current = next;
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

  /* Move starting point */
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

/*
Author: davex
e-mail: efengeler@gmail.com
*

#include "rewind.h"



int num_rwnd_states = 0;
int rwnd_state_size = 0;
int g_memory_allocated = 0;

struct rewind_state{
	int have_data;
	unsigned char *data;
	struct rewind_state *next;
	struct rewind_state *prev;
};


struct rewind_state *ptr_rewind_states, *prev_state, *next_state;




int get_psp_max_free_memory(void){
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



void allocate_rewind_states(void){
	struct rewind_state *created_state, *first_state;
	int i;
	
	int total_rewind_memory = (int)( (float)(get_psp_max_free_memory()) * 0.85 ); //reserves 85% of free memory
	
	rwnd_state_size = get_save_state_size() + 100;  //declared in "state.h"	
	num_rwnd_states = (int) ( (float)total_rewind_memory / (float) rwnd_state_size );
	if( num_rwnd_states <= 0){
		g_memory_allocated = 0;
		return;
	}else{
		g_memory_allocated = 1;
	}
	
	//reserves first state
	created_state =  (struct rewind_state *)malloc( sizeof(struct rewind_state) );
	created_state->have_data = 0;
	created_state->data = (unsigned char *) malloc( rwnd_state_size);
	first_state = created_state;
	prev_state = first_state;
	
	//reserves remaining states
	for( i = 1; i< num_rwnd_states; i++){
		created_state  = (struct rewind_state *)malloc( sizeof(struct rewind_state) );
		created_state->have_data = 0;
		created_state->data = (unsigned char *) malloc( rwnd_state_size);
		created_state ->prev = prev_state;
		prev_state->next = created_state;
		prev_state = created_state;
	}
	
	
	//make list be circular
	created_state->next = first_state; 
	first_state->prev = created_state; 
	ptr_rewind_states = first_state;
	
}


void free_rewind_states(void){
	if( g_memory_allocated == 0)
		return;
	
	struct rewind_state *now_state; 
	
	now_state = ptr_rewind_states;
	prev_state = now_state->prev;
	prev_state->next = NULL;
	
	while(1){
		if ( now_state == NULL)
			break;
		next_state = now_state->next;
		free(now_state->data );
		free(now_state);
		now_state = next_state;
	}
	g_memory_allocated = 0;
}



void save_rewind_state(void){
	if( g_memory_allocated == 0){
		return;
	}
	save_state_to_mem(ptr_rewind_states->data);//declared in "state.h"
	ptr_rewind_states->have_data = 1;
	ptr_rewind_states = ptr_rewind_states->next;
}

int read_rewind_state(void){
	if( g_memory_allocated == 0){
		return -999;
	}
	
	int ret_val = -999;
	prev_state = ptr_rewind_states->prev;

	if (prev_state->have_data > 0 ){
		load_state_from_mem(prev_state->data); //declared in  "state.h"
		prev_state->have_data = 0;
		ptr_rewind_states = ptr_rewind_states->prev;
		ret_val = 1;
	}
	return ret_val;
}
*/
