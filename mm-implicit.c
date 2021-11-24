// This file gives you a starting point to implement malloc using implicit list
// Each chunk has a header (of type header_t) and does *not* include a footer
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "mm-common.h"
#include "mm-implicit.h"
#include "memlib.h"

// turn "debug" on while you are debugging correctness. 
// Turn it off when you want to measure performance
static bool debug = false; 

size_t hdr_size = sizeof(header_t);

void 
init_chunk(header_t *p, size_t csz, bool allocated)
{
	p->size = csz;
	p->allocated = allocated;
}

// Helper function next_chunk returns a pointer to the header of 
// next chunk after the current chunk h.
// It returns NULL if h is the last chunk on the heap.
// If h is NULL, next_chunk returns the first chunk if heap is non-empty, and NULL otherwise.
header_t *
next_chunk(header_t *h)
{
	if(h == NULL){
		if(mem_heapsize() == 0) return NULL;
		return (header_t*) mem_heap_lo();;
	}
	header_t* next = (header_t*) ((char*)h + h->size);
	if((void*) next == (char*)mem_heap_hi() + 1) return NULL;
	return next;
}


/* 
 * mm_init initializes the malloc package.
 */
int mm_init(void)
{
	//double check that hdr_size should be 16-byte aligned
	assert(hdr_size == align(hdr_size));
	// start with an empty heap. 
	// no additional initialization code is necessary for implicit list.
	return 0;
}


// helper function first_fit traverses the entire heap chunk by chunk from the begining. 
// It returns the first free chunk encountered whose size is bigger or equal to "csz".  
// It returns NULL if no large enough free chunk is found.
// Please use helper function next_chunk when traversing the heap
header_t *
first_fit(size_t csz)
{
	header_t* p = next_chunk(NULL);
	while(p && (p->allocated || p->size < csz)){
		p = next_chunk(p);
		if(p == NULL) return NULL;
	}
	return p;
}

// helper function split cuts the chunk into two chunks. The first chunk is of size "csz", 
// the second chunk contains the remaining bytes. 
// You must check that the size of the original chunk is big enough to enable such a cut.
void
split(header_t *original, size_t csz)
{	
	size_t remain_size = original->size - csz;
	size_t min_payload = 16;
	if(remain_size < hdr_size + min_payload)
		return;

	original->size = csz;
	header_t* n = (header_t*) ((char*)original + csz);
	init_chunk(n, remain_size, false);
}

// helper function ask_os_for_chunk invokes the mem_sbrk function to ask for a chunk of 
// memory (of size csz) from the "operating system". It initializes the new chunk 
// using helper function init_chunk and returns the initialized chunk.
header_t *
ask_os_for_chunk(size_t csz)
{
	header_t* p = (header_t*) mem_sbrk(csz);
	init_chunk(p, csz, false);
	return p;
}

/* 
 * mm_malloc allocates a memory block of size bytes
 */
void *
mm_malloc(size_t size)
{
	//make requested payload size aligned
	size = align(size);
	//chunk size is aligned because both payload and header sizes
	//are aligned
	size_t csz = hdr_size + size;

	header_t *p = first_fit(csz);
	if(p == NULL){
		p = ask_os_for_chunk(csz);
		p->size = csz;
	}
	else
		split(p, csz);
	p->allocated = true;
	p = p+1;

	//Your code here 
	//to obtain a free chunk p to satisfy this request.
	//
	//The code logic should be:
	//Try to find a free chunk using helper function first_fit
	//    If found, split the chunk (using helper function split).
	//    If not found, ask OS for new memory using helper ask_os_for_chunk
	//Set the chunk's status to be allocated


	//After finishing obtaining free chunk p, 
	//check heap correctness to catch bugs
	if (debug) {
		mm_checkheap(true);
	}
	return (void *)p;
}

// Helper function payload_to_header returns a pointer to the 
// chunk header given a pointer to the payload of the chunk 
header_t *
payload2header(void *p)
{
	header_t* ptr = ((header_t*) p)-1;
	return ptr;
}

// Helper function coalesce merges free chunk h with subsequent 
// consecutive free chunks to become one large free chunk.
// You should use next_chunk when implementing this function
void
coalesce(header_t *h)
{
	size_t total_size = 0;
	header_t* ptr = h;
	while(ptr && !(ptr->allocated)){
		total_size += ptr->size;
		ptr = next_chunk(ptr);
	}
	h->size = total_size;
}

/*
 * mm_free frees the previously allocated memory block
 */
void 
mm_free(void *p)
{
	header_t* ptr = payload2header(p);
	ptr->allocated = false;
	coalesce(ptr);

	// Obtain pointer to current chunk using helper payload_to_header 
	// Set current chunk status to "free"
	// Call coalesce() to merge current chunk with subsequent free chunks
	  
	  
	// After freeing the chunk, check heap correctness to catch bugs
	if (debug) {
		mm_checkheap(true);
	}
}	

/*
 * mm_realloc changes the size of the memory block pointed to by ptr to size bytes.  
 * The contents will be unchanged in the range from the start of the region up to the minimum of   
 * the  old  and  new sizes.  If the new size is larger than the old size, the added memory will   
 * not be initialized.  If ptr is NULL, then the call is equivalent to malloc(size).
 * if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr).
 */

void *
mm_realloc(void *ptr, size_t size)
{
	size = align(size);
	if(ptr == NULL) //original pointer to memory block is null
		return mm_malloc(size);
	if(size == 0){ //free the memory block
		mm_free(ptr);
		return NULL;
	}
	size_t csz = hdr_size + size;
	header_t* h = payload2header(ptr);
	size_t original_size = h->size;
	if(original_size >= csz){ //size decrease
		split(h, csz);
		return ptr;
	}
	//size increase
	header_t* next = next_chunk(h);
	//check whether the next block can accomadate remaining size
	if(next && !next->allocated && (next->size >= csz - original_size)){
		h->size += next->size;
		split(h, csz);
		return ptr;
	}
	//allocate a new chunk and free original chunk
	char* new_ptr = (char*)mm_malloc(size);
	char* old_ptr = (char*)ptr;
	for(int i = 0; i < original_size - hdr_size; i++)
		*(new_ptr + i) = *(old_ptr + i);
	mm_free(ptr);
	return (void*) new_ptr;
	  
	// Check heap correctness after realloc to catch bugs
	if (debug) {
		mm_checkheap(true);
	}
}


/*
 * mm_checkheap checks the integrity of the heap and returns a struct containing 
 * basic statistics about the heap. Please use helper function next_chunk when 
 * traversing the heap
 */
heap_info_t 
mm_checkheap(bool verbose) 
{
	heap_info_t info;
	info.num_allocated_chunks = 0;
	info.num_free_chunks = 0;
	info.free_size = 0;
	info.allocated_size = 0;


	// traverse the heap to fill in the fields of info
	// header_t* p = (header_t*) mem_heap_lo();
	header_t *p = next_chunk(NULL);
	while(p){
		if(p->allocated){
			info.num_allocated_chunks ++;
			info.allocated_size += p->size;
		}
		else{
			info.num_free_chunks ++;
			info.free_size += p->size;
		}
		p = next_chunk(p);
	}
	// correctness of implicit heap amounts to the following assertion.
	assert(mem_heapsize() == (info.allocated_size + info.free_size));
	return info;
}

