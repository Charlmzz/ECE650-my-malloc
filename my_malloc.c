#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "my_malloc.h"

/*
    bf_malloc will return the best available memory chunk that is greater but 
    closest to user required size. If no such memory chunk exists, it will 
    call sbrk() to allocate a new memory chunk on heap, then return that chunk.
    The input of this function is size and the output is a void* that points to 
    the available memory chunk.
*/
void *bf_malloc_lock(size_t size){
    size_t total = size+nodeSize;
    malloc_n *curr= (void *)head;
    malloc_n *rst = NULL; //stores the available memory chunk
    size_t currSize = (size_t)-1; //infinity, largest num in size_t
    while (curr!=NULL){
        size_t currsz = curr->sz;
        if (currsz>size && currsz<currSize){ //smallest free chunk greater than needed
            currSize = currsz;
            rst = curr;
        }else if (currsz==size){ //if exactly the same, we know we can directly return as it is already optimal
            if (curr==head){
                head = curr->next;
            }else{
                curr->prev->next = curr->next;
            }
            if (curr->next!=NULL){
                curr->next->prev = curr->prev;
            }
            return (void*)curr+nodeSize;
        }
        curr = curr->next;
    }
    
    if (rst!=NULL){ //meaning there is a chunk available already, the two cases are the same as in ff_malloc
        if (currSize>total){
            malloc_n *split = (malloc_n *) ((void*)rst+nodeSize+(rst->sz)-total);
            split->sz = size;
            split->prev = NULL;
            split->next = NULL;
            rst->sz -=total;
            return (void *)split+nodeSize;    
        }else{
            if (rst->prev!=NULL){
                rst->prev->next = rst->next;
            }else{
                head = rst->next;
            }
            if (rst->next!=NULL){
                rst->next->prev = rst->prev;
            }

            return (void*)rst+nodeSize;
        }
    }else{ //if no such chunk available, we call sbrk for new chunk
        void *ptr = sbrk(total);
        if (ptr == (void*)-1) return NULL;
        malloc_n *newChunk = ptr;
        newChunk->prev = NULL;
        newChunk->next = NULL;
        newChunk->sz = size;
        return (void *)newChunk+nodeSize;
    }
}

/*
    bf_malloc will return the best available memory chunk that is greater but 
    closest to user required size. If no such memory chunk exists, it will 
    call sbrk() to allocate a new memory chunk on heap, then return that chunk.
    The input of this function is size and the output is a void* that points to 
    the available memory chunk.
*/
void *bf_malloc_nolock(size_t size){
    size_t total = size+nodeSize;
    malloc_n *curr= (void *)unlockHead;
    malloc_n *rst = NULL; //stores the available memory chunk
    size_t currSize = (size_t)-1; //infinity, largest num in size_t
    while (curr!=NULL){
        size_t currsz = curr->sz;
        if (currsz>size && currsz<currSize){ //smallest free chunk greater than needed
            currSize = currsz;
            rst = curr;
        }else if (currsz==size){ //if exactly the same, we know we can directly return as it is already optimal
            if (curr==unlockHead){
                unlockHead = curr->next;
            }else{
                curr->prev->next = curr->next;
            }
            if (curr->next!=NULL){
                curr->next->prev = curr->prev;
            }
            return (void*)curr+nodeSize;
        }
        curr = curr->next;
    }
    
    if (rst!=NULL){ //meaning there is a chunk available already, the two cases are the same as in ff_malloc
        if (currSize>total){
            malloc_n *split = (malloc_n *) ((void*)rst+nodeSize+(rst->sz)-total);
            split->sz = size;
            split->prev = NULL;
            split->next = NULL;
            rst->sz -=total;
            return (void *)split+nodeSize;    
        }else{
            if (rst->prev!=NULL){
                rst->prev->next = rst->next;
            }else{
                unlockHead = rst->next;
            }
            if (rst->next!=NULL){
                rst->next->prev = rst->prev;
            }

            return (void*)rst+nodeSize;
        }
    }else{ //if no such chunk available, we call sbrk for new chunk
        pthread_mutex_lock(&lock);
        void *ptr = sbrk(total);
        
        if (ptr == (void*)-1) return NULL;
        malloc_n *newChunk = ptr;
        newChunk->prev = NULL;
        newChunk->next = NULL;
        newChunk->sz = size;
        pthread_mutex_unlock(&lock);
        return (void *)newChunk+nodeSize;
        
    }
}

/*
    freeSpace will add current mem chunk freed by the user to the free list based on address order. 
    If current mem chunk has a successive address with either the previous chunk 
    or the next chunk, the successive chunks get mergered together.
    The input of this function is a pointer which points to the memory that the user wants to free.
    There is no output for this function.
*/
void freeSpace_lock(void *ptr){
    malloc_n *newFree = ptr - nodeSize; //in free list, always points to the chunk's very start (include matadata)
    malloc_n *curr;
    curr = head;
    while (curr!=NULL){
        if (newFree<curr){ //comparing the addresses, insert the newly freed based on address order from small to large.
            //insersion starts, inserting newly freed chunk and link the previous and the next
            if (curr->prev==NULL){
               head = newFree;
               newFree->prev = NULL; 
            }else{
               curr->prev->next = newFree;
               newFree->prev = curr->prev;
            }
            newFree->next = curr;
            curr->prev = newFree; //insertion ends
            //merge starts
            if ((void*)newFree+nodeSize+(newFree->sz) == (void *)curr){ //check if this chunk is successive with the next one
                newFree->sz += (size_t)(curr->sz+nodeSize);
                newFree->next = curr->next;
                if (curr->next!=NULL){ //not tail
                    curr->next->prev = newFree;
                }
            }
            if (head!=newFree){ //check if this chunk is successive with previous chunk
                if ((void*)(newFree->prev)+newFree->prev->sz+nodeSize==(void*)newFree){ //can be combined
                    newFree->prev->sz += nodeSize+newFree->sz;
                    newFree->prev->next = newFree->next;
                    if (newFree->next!=NULL){
                        newFree->next->prev = newFree->prev;
                    } 
                }
            } //merge ends
            return;
        }
        curr = curr->next;
    }
    //if such place is not found, put it in the end of the list
    if (head==NULL){ //check if current list is empty (no head)
        head = newFree;
        newFree->next = NULL;
    }else{
        curr = head;
        while (curr->next!=NULL){ //find the last node in the free list
            curr = curr->next;
        }
        //now curr points at last node in list, we insert new freed chunk and check if it can be merged
        curr->next = newFree;
        newFree->prev = curr;
        newFree->next = NULL;
        if ((void*)(newFree->prev)+newFree->prev->sz+nodeSize==(void*)newFree){
            newFree->prev->sz += nodeSize+newFree->sz;
            newFree->prev->next = newFree->next;
            if (newFree->next!=NULL){
                newFree->next->prev = newFree->prev;
            } 
        }
    }
}

/*
    freeSpace will add current mem chunk freed by the user to the free list based on address order. 
    If current mem chunk has a successive address with either the previous chunk 
    or the next chunk, the successive chunks get mergered together.
    The input of this function is a pointer which points to the memory that the user wants to free.
    There is no output for this function.
*/
void freeSpace_nolock(void *ptr){
    malloc_n *newFree = ptr - nodeSize; //in free list, always points to the chunk's very start (include matadata)
    malloc_n *curr;
    curr = unlockHead;
    while (curr!=NULL){
        if (newFree<curr){ //comparing the addresses, insert the newly freed based on address order from small to large.
            //insersion starts, inserting newly freed chunk and link the previous and the next
            if (curr->prev==NULL){
               unlockHead = newFree;
               newFree->prev = NULL; 
            }else{
               curr->prev->next = newFree;
               newFree->prev = curr->prev;
            }
            newFree->next = curr;
            curr->prev = newFree; //insertion ends
            //merge starts
            if ((void*)newFree+nodeSize+(newFree->sz) == (void *)curr){ //check if this chunk is successive with the next one
                newFree->sz += (size_t)(curr->sz+nodeSize);
                newFree->next = curr->next;
                if (curr->next!=NULL){ //not tail
                    curr->next->prev = newFree;
                }
            }
            if (unlockHead!=newFree){ //check if this chunk is successive with previous chunk
                if ((void*)(newFree->prev)+newFree->prev->sz+nodeSize==(void*)newFree){ //can be combined
                    newFree->prev->sz += nodeSize+newFree->sz;
                    newFree->prev->next = newFree->next;
                    if (newFree->next!=NULL){
                        newFree->next->prev = newFree->prev;
                    } 
                }
            } //merge ends
            return;
        }
        curr = curr->next;
    }
    //if such place is not found, put it in the end of the list
    if (unlockHead==NULL){ //check if current list is empty (no head)
        unlockHead = newFree;
        newFree->next = NULL;
    }else{
        curr = unlockHead;
        while (curr->next!=NULL){ //find the last node in the free list
            curr = curr->next;
        }
        //now curr points at last node in list, we insert new freed chunk and check if it can be merged
        curr->next = newFree;
        newFree->prev = curr;
        newFree->next = NULL;
        if ((void*)(newFree->prev)+newFree->prev->sz+nodeSize==(void*)newFree){
            newFree->prev->sz += nodeSize+newFree->sz;
            newFree->prev->next = newFree->next;
            if (newFree->next!=NULL){
                newFree->next->prev = newFree->prev;
            } 
        }
    }
}

//hw2 functions

void *ts_malloc_lock(size_t size){
    pthread_mutex_lock(&lock);
    void *ptr = bf_malloc_lock(size);
    pthread_mutex_unlock(&lock);
    return ptr;
}

void *ts_malloc_nolock(size_t size){
    void *ptr = bf_malloc_nolock(size);
    return ptr;
}

/*
    bf_free calls freeSpace() to do the actual freeing.
*/
void ts_free_lock(void *ptr){
    pthread_mutex_lock(&lock);
    freeSpace_lock(ptr);
    pthread_mutex_unlock(&lock);
}

/*
    bf_free calls freeSpace() to do the actual freeing.
*/
void ts_free_nolock(void *ptr){
    freeSpace_nolock(ptr);
}

