#include <stdlib.h> 
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "nuphase-buf.h"


//TODO: is this really necessary on a single-core ARM? 
#define MEMORY_FENCE __sync_synchronize(); 

static size_t buffer_count = 0; 


struct nuphase_buf
{
  void * mem; 
  volatile size_t produced_count; 
  volatile size_t consumed_count; 
  size_t memb_size;
  size_t capacity; 
  size_t index; 
}; 



nuphase_buf_t* nuphase_buf_init(size_t max_capacity, size_t memb_size) 
{
  nuphase_buf_t * b = malloc(sizeof(struct nuphase_buf)); 

  if (!b) 
  {
    fprintf(stderr,"Can't allocate buffer. Are we out of memory!?"); 
    return 0; 
  }


  b->mem = calloc(memb_size, max_capacity); 
  if (!b->mem)
  {
    fprintf(stderr,"Can't allocate buffer memory. Are we out of memory!?"); 
    free(b); 
    return 0; 
  }
  
  b->produced_count = 0; 
  b->consumed_count = 0; 
  b->capacity = max_capacity; 
  b->memb_size = memb_size; 
  b->index = buffer_count++; 

  
  return b; 
}


size_t nuphase_buf_capacity(const nuphase_buf_t *b)
{
  return b->capacity; 
}

size_t nuphase_buf_occupancy(const nuphase_buf_t *b)
{
  return b->produced_count - b->consumed_count; 
}

void* nuphase_buf_getmem(nuphase_buf_t *b )
{

  int warned = 0;
  while(nuphase_buf_capacity(b) == nuphase_buf_occupancy(b))
  {
    if (!warned) 
    {
      fprintf(stderr,"WARNING: Buffer %zd is full!\n", b->index); 
      warned++; 
    }
    sched_yield(); 
  }

  return b->mem  + b->memb_size * (b->produced_count % b->capacity); 
}

void nuphase_buf_commit(nuphase_buf_t * b) 
{
  MEMORY_FENCE
  b->produced_count++;
}

void nuphase_buf_push(nuphase_buf_t *b, const void * mem)
{
  void * ptr =  nuphase_buf_getmem(b); 
  memcpy(ptr,mem,b->memb_size); 
  nuphase_buf_commit(b); 
}

void * nuphase_buf_pop(nuphase_buf_t * b, void * dest)
{
  if (!dest) dest = malloc(b->memb_size); 
  while (b->produced_count - b->consumed_count == 0)
  {
    sched_yield(); 
  }

  memcpy(dest, b->mem + b->memb_size * (b->consumed_count % b->capacity), b->memb_size); 
  MEMORY_FENCE
  b->consumed_count++; 
  return dest; 
}


int nuphase_buf_destroy(nuphase_buf_t *b) 
{
  int occupancy = nuphase_buf_occupancy(b); 
  free(b->mem);
  free(b); 
  return occupancy; 
}





