#include "my_malloc.h"
#include <unistd.h>
#include <pthread.h>


// Initialization
block_t* head = NULL; // head of free list
__thread block_t* head_tls = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


//When search has no result for the required size,
//need to extend heap to start a new block
block_t* extend(size_t s, int _lock){
  //printf("************************** extend ******************************\n");
  void* p;
  if(_lock==1){
    p = sbrk(s+sizeof(block_t)); // increment on break pointer
  }
  else{
    pthread_mutex_lock(&lock);
    //printf("************************** no_lock extend ******************************\n");
    p = sbrk(s+sizeof(block_t));
    pthread_mutex_unlock(&lock);
  }
  if(p==(void*)-1){
    return NULL;
  }
  block_t* b = (block_t*) p;
  b->size = s;
  b->next = NULL;
  b->addr = p+sizeof(block_t);

  return b;
}

//When search did find a suitable block,
//but the size of this block is larger than metadata
void split(block_t* b, block_t* prev, size_t s, block_t** p_head){
  //printf("************************** split ******************************\n");
  block_t* new_free = (block_t *) (b->addr + s);
  new_free->next = b->next;

  if(b==*p_head){
    *p_head = new_free;
  }
  else{
    prev->next = new_free;
  }

  new_free->size = b->size - s - sizeof(block_t);
  new_free->addr = (char* )(new_free + 1);
  b->size = s;
  b->next = NULL;
}

//Remove one block from free list
void remfree(block_t* block, block_t* prev, block_t** p_head){
  //printf("************************** remove ******************************\n");
  if(block==*p_head){
    *p_head = (*p_head)->next;
  }
  else{
    prev->next = block->next;
  }
  block->next = NULL;
}

//Merge adjacent free regions after free()
void merge(block_t *b, block_t* prev){
  //printf("****************************   merge   **************************\n");
  if(prev && prev->addr+prev->size == (char* )b){
    prev->size += (b->size + sizeof(block_t));
    prev->next = b->next;
    b = prev;
    //printf("......   MERGE WITH FORMER ONE   ......\n");
  }
  if(b->next && b->addr+b->size == (char* )b->next){
    b->size += (b->next->size + sizeof(block_t));
    b->next = b->next->next;
    //printf("......   MERGE WITH LATER ONE   ......\n");
  }
}

//Best Fit search
block_t *bf_search(size_t s, int* flag, block_t** p_head){
  //printf("************************** BF search ******************************\n");
  block_t* curr = *p_head;
  block_t* bestprev = NULL;
  size_t min = 0;
  if(curr->size>=s){
    min = curr->size;
  }
  while(curr->next){
    if(curr->next->size>=s){
      if(min==0 || curr->next->size<min){
	bestprev = curr;
	min = curr->next->size;
      }
    }
    curr = curr->next;
  }
  if(min!=0){
    *flag = 1;
  }
  return bestprev;
}

//Best Fit malloc
void *bf_malloc(size_t size, block_t** p_head, int _lock){
  if(size<=0){
    return NULL;
  }
  
  if((*p_head)==NULL){
    return extend(size,_lock)->addr;
  }
  else{
    int bf_search_result = 0;
    int* bf_flag = &bf_search_result;
    block_t* block;
    block_t* prev;
    prev = bf_search(size,bf_flag,p_head);

    if(bf_search_result==0){
      return extend(size,_lock)->addr;
    }
    else{
      if(!prev){
	block = *p_head;
      }
      else{
	block = prev->next;
      }
      if(block->size < size+sizeof(block_t)){
	remfree(block,prev,p_head);
      }
      else{
	split(block,prev,size,p_head);
	block->size = size;
      }
      return block->addr;
    }
  }
}

//Best Fit free
void bf_free(void *ptr, block_t** p_head){
  //Verify if the input address is valid
  block_t* pblock = (block_t *)ptr - 1;
  if(*p_head){
    block_t* prev;
    if((unsigned long)pblock<(unsigned long)*p_head){
      pblock->next = *p_head;
      *p_head = pblock;
      prev = NULL;
    }
    else if((unsigned long)pblock==(unsigned long)*p_head){
      return;
    }
    else{
      block_t* curr = *p_head;
      while(curr->next && (unsigned long)(curr->next)<(unsigned long)pblock){
	if((unsigned long)curr==(unsigned long)pblock){ return;}
	curr = curr->next;
      }
      prev = curr;
      if(curr->next){
	if((unsigned long)curr->next>(unsigned long)pblock){
	  pblock->next = curr->next;
	  curr->next = pblock;
	}
	else{
	  return;
	}
      }
      else{
	// pblock will be the last one in the free list
	curr->next = pblock;
	pblock->next = NULL;
      }
    }
    merge(pblock, prev);
  }
  else{
    *p_head = pblock;
    pblock->next = NULL;
  }
}

//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size){
  pthread_mutex_lock(&lock);
  void *addr = bf_malloc(size,&head,1);
  pthread_mutex_unlock(&lock);
  return addr;
}

void ts_free_lock(void *ptr){
  pthread_mutex_lock(&lock);
  bf_free(ptr,&head);
  pthread_mutex_unlock(&lock);
}


//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size){
  //printf("========================= no_lock version malloc =====================\n");
  return bf_malloc(size,&head_tls,0);
}

void ts_free_nolock(void *ptr){
  bf_free(ptr,&head_tls);
}
