#include <stdlib.h>
#include <unistd.h>

/*
    Node struct of doubly linked free list
*/
struct malloc_node{
    size_t sz;
    struct malloc_node *prev;
    struct malloc_node *next;
};
typedef struct malloc_node malloc_n;

malloc_n *head = NULL; //head of the doublely ll
size_t nodeSize = sizeof(malloc_n); //metadata size
void freeSpace(void *ptr); //called by both ff & bf free

void *ff_malloc(size_t size);
void ff_free(void *ptr);


void *bf_malloc(size_t size);
void bf_free(void *ptr);

void print_list();

unsigned long dataSegSize = 0; //tracks the size allocated by sbrk in total
unsigned long get_data_segment_size();
unsigned long get_data_segment_free_space_size();