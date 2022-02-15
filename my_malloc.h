#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

/*
    Node struct of doubly linked free list
*/
struct malloc_node{
    size_t sz;
    struct malloc_node *prev;
    struct malloc_node *next;
};
typedef struct malloc_node malloc_n;

malloc_n *head = NULL; //head of the doublely ll, lock version
size_t nodeSize = sizeof(malloc_n); //metadata size
void freeSpace_lock(void *ptr); //called by both ff & bf free
void freeSpace_unlock(void *ptr); //called by both ff & bf free


void *bf_malloc_lock(size_t size);
void bf_free_lock(void *ptr);

void *bf_malloc_nolock(size_t size);
void bf_free_nolock(void *ptr);

//hw2
//thread sasfe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//thread safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

__thread malloc_n *unlockHead= NULL;