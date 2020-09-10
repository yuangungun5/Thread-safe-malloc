//Header file

#ifndef _MY_MALLOC_H
#define _MY_MALLOC_H

#include <stdlib.h>
#include <stdio.h>

typedef struct datablock{
  size_t size;
  struct datablock* next;
  char* addr;
}block_t;

block_t* extend_lock(size_t s, int _lock);
void split(block_t* b, block_t* prev, size_t s, block_t** p_head);
void remfree(block_t* block, block_t* prev, block_t** p_head);
void merge(block_t *b, block_t* prev);

block_t *bf_search(size_t s,int* flag, block_t** p_head);
//Best Fit malloc/free
void *bf_malloc(size_t size, block_t** p_head, int _lock);
void bf_free(void *ptr, block_t** p_head);

//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

#endif

