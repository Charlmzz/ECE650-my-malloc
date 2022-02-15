#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "my_malloc.h"

/*
    ff_malloc will return the first available memory chunk that satisfies 
    the size required by the user. If no such memory chunk exists, it will 
    call sbrk() to allocate a new memory chunk on heap, then return that chunk.
    The input of this function is size and the output is a void* that points to 
    the available memory chunk.
*/
void *ff_malloc(size_t size){
    size_t total = size+nodeSize;
    malloc_n *curr = head;
    //find the first suitable memory chunk in current free list
    while (curr!=NULL){
        //current chunk's available size is greater than user's requirement plus metadata size,
        //we split current chunk from the end and give the end part to user
        if (curr->sz>total){
            malloc_n *split = (malloc_n *) ((void*)curr+nodeSize+(curr->sz)-total);
            split->sz = size;
            curr->sz -=total;
            split->prev = NULL;
            split->next = NULL;
            return (void *)split+nodeSize;
        }else if(curr->sz>=size && curr->sz<total){ //if current chunk's available size is greater
            //than user's requirement, but cannot hold extra metadata, we assign current chunk directly to user
            if (curr->prev!=NULL){
                curr->prev->next = curr->next;
            }else{
                head = curr->next;
            }
            if (curr->next!=NULL){
                curr->next->prev = curr->prev;
            }
            return (void*)curr+nodeSize;
        }
        curr = curr->next;
    }
    //current free list has no suitable chunk, so sbrk for new mem chunk
    void *newSpace = sbrk(total);
    if (newSpace==(void*)-1){
        return NULL;
    }
    malloc_n* newChunk = (malloc_n*) newSpace;
    newChunk->prev = NULL;
    newChunk->next = NULL;
    newChunk->sz = size;
    dataSegSize+=total;
    return (void *)newChunk+nodeSize;
    
}

/*
    bf_malloc will return the best available memory chunk that is greater but 
    closest to user required size. If no such memory chunk exists, it will 
    call sbrk() to allocate a new memory chunk on heap, then return that chunk.
    The input of this function is size and the output is a void* that points to 
    the available memory chunk.
*/
void *bf_malloc(size_t size){
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
            rst->sz -=total;
            split->prev = NULL;
            split->next = NULL;
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
        malloc_n *newChunk = sbrk(total);
        newChunk->prev = NULL;
        newChunk->next = NULL;
        newChunk->sz = size;
        dataSegSize+=total;
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
void freeSpace(void *ptr){
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
    ff_free calls freeSpace() to do the actual freeing.
*/
void ff_free(void *ptr){
    freeSpace(ptr);
}

/*
    bf_free calls freeSpace() to do the actual freeing.
*/
void bf_free(void *ptr){
    freeSpace(ptr);
}

/*
    get_data_segment_size() returns the variable dataSegSize, which keeps 
    track of the entire heap memory allocated.
*/
unsigned long get_data_segment_size(){
    return dataSegSize;
}

/*
    get_data_segment_free_space_size() calculates how big current free list size is.
    This size includes the metadata of each chunk.
*/
unsigned long get_data_segment_free_space_size(){
    unsigned long freeSpaceSize=0;
    malloc_n *curr = head;
    while (curr!=NULL){
        freeSpaceSize+=curr->sz;
        freeSpaceSize+=nodeSize;
        curr = curr->next;
    }
    return freeSpaceSize;
}
