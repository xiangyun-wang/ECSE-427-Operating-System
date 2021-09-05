/*
 * =====================================================================================
 *
 *	Filename:  		sma.c
 *
 *  Description:	Base code for Assignment 3 for ECSE-427 / COMP-310
 *
 *  Version:  		1.0
 *  Created:  		6/11/2020 9:30:00 AM
 *  Revised:  		-
 *  Compiler:  		gcc
 *
 *  Author:  		Mohammad Mushfiqur Rahman
 *      
 *  Instructions:   Please address all the "TODO"s in the code below and modify 
 * 					them accordingly. Feel free to modify the "PRIVATE" functions.
 * 					Don't modify the "PUBLIC" functions (except the TODO part), unless
 * 					you find a bug! Refer to the Assignment Handout for further info.
 * =====================================================================================
 */

/* Includes */
#include "sma.h" // Please add any libraries you plan to use inside this file
#include <stdint.h>
#include <string.h>
/* Definitions*/
#define MAX_TOP_FREE (128 * 1024) // Max top free block size = 128 Kbytes
//	TODO: Change the Header size if required
#define FREE_BLOCK_HEADER_SIZE 2 * sizeof(uintptr_t) + 2*sizeof(int) // Size of the Header in a free memory block
//	TODO: Add constants here
#define BLOCK_FOOT_SIZE 2 * sizeof(int)				// Size of free tag and size int at the end of a memory block
#define USED_BLOCK_HEADER_SIZE 2 * sizeof(int)		// Header size of a used memory block
#define ADR_LEN sizeof(uintptr_t)					// Length in byte of an address
#define INT_LEN sizeof(int)							// Length in byte of an int
#define EXCESS_SIZE 100*1024						// excess size when allocating new memory block using sbrk

typedef enum //	Policy type definition
{
	WORST,
	NEXT
} Policy;

char *sma_malloc_error;
void *freeListHead = NULL;			  //	The pointer to the HEAD of the doubly linked free memory list
void *freeListTail = NULL;			  //	The pointer to the TAIL of the doubly linked free memory list
unsigned long totalAllocatedSize = 0; //	Total Allocated memory in Bytes
unsigned long totalFreeSize = 0;	  //	Total Free memory in Bytes in the free memory list
Policy currentPolicy = WORST;		  //	Current Policy
//	TODO: Add any global variables here
void *lastAllocated = NULL;			// pointer of the last allocated block of memory

/*
 * =====================================================================================
 *	Public Functions for SMA
 * =====================================================================================
 */

/*
 *	Funcation Name: sma_malloc
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates a memory block of input size from the heap, and returns a 
 * 					pointer pointing to it. Returns NULL if failed and sets a global error.
 */
void *sma_malloc(int size)			
{
	void *pMemory = NULL;

	// Checks if the free list is empty
	if (freeListHead == NULL)
	{
		// Allocate memory by increasing the Program Break
		pMemory = allocate_pBrk(size);
	}
	// If free list is not empty
	else
	{
		// Allocate memory from the free memory list
		pMemory = allocate_freeList(size);

		// If a valid memory could NOT be allocated from the free memory list
		if (pMemory == (void *)-2)
		{
			// Allocate memory by increasing the Program Break
			pMemory = allocate_pBrk(size);
		}
	}

	// Validates memory allocation
	if (pMemory < 0 || pMemory == NULL)																						
	{
		sma_malloc_error = "Error: Memory allocation failed!";
		return NULL;
	}

	// Updates SMA Info
	totalAllocatedSize += size;
	lastAllocated = pMemory;								// ADD A NEW LINE TO UPDATE lastAllocated
	return pMemory;
}

/*
 *	Funcation Name: sma_free
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Deallocates the memory block pointed by the input pointer
 */
void sma_free(void *ptr)
{
	//	Checks if the ptr is NULL
	if (ptr == NULL)
	{
		puts("Error: Attempting to free NULL!");
	}
	//	Checks if the ptr is beyond Program Break
	else if (ptr > sbrk(0))
	{
		puts("Error: Attempting to free unallocated space!");
	}
	else
	{
		//	Adds the block to the free memory list
		add_block_freeList(ptr);
		
	}
}

/*
 *	Funcation Name: sma_mallopt
 *	Input type:		int
 * 	Output type:	void
 * 	Description:	Specifies the memory allocation policy
 */
void sma_mallopt(int policy)
{
	// Assigns the appropriate Policy
	if (policy == 1)
	{
		currentPolicy = WORST;
	}
	else if (policy == 2)
	{
		currentPolicy = NEXT;
	}
}

/*
 *	Funcation Name: sma_mallinfo
 *	Input type:		void
 * 	Output type:	void
 * 	Description:	Prints statistics about current memory allocation by SMA.
 */
void sma_mallinfo()
{
	//	Finds the largest Contiguous Free Space (should be the largest free block)
	int largestFreeBlock = get_largest_freeBlock();
	char str[70];																												// CHANGED FROM 60 TO 70

	//	Prints the SMA Stats
	sprintf(str, "Total number of bytes allocated: %lu", totalAllocatedSize);
	puts(str);
	sprintf(str, "Total free space: %lu", totalFreeSize);
	puts(str);
	sprintf(str, "Size of largest contigious free space (in bytes): %d", largestFreeBlock);
	puts(str);
	//show_freeList();
}

/*
 *	Funcation Name: sma_realloc
 *	Input type:		void*, int
 * 	Output type:	void*
 * 	Description:	Reallocates memory pointed to by the input pointer by resizing the
 * 					memory block according to the input size.
 */
void *sma_realloc(void *ptr, int size)
{
	// TODO: 	Should be similar to sma_malloc, except you need to check if the pointer address
	//			had been previously allocated.
	// Hint:	Check if you need to expand or contract the memory. If new size is smaller, then
	//			chop off the current allocated memory and add to the free list. If new size is bigger
	//			then check if there is sufficient adjacent free space to expand, otherwise find a new block
	//			like sma_malloc.
	//			Should not accept a NULL pointer, and the size should be greater than 0.
	
	void* out = ptr;
	uintptr_t tmp_ptr = (uintptr_t)ptr;			// cast to uintptr_t type to do pointer arithmetic
	if(ptr==NULL || size <= 0 || *(int*)(tmp_ptr-2*INT_LEN)!=1){		// Error for NULL pointer, size smaller than 0, and free memory block
		sma_malloc_error = "Error: NULL pointer or Non-positive size or Pointer not allocated!";
	}else{
		int extra = size - get_blockSize(ptr);			// extra size needed for the new block of memory
		if(extra<0){									// extra size negative, chop off the extra free memory
			*(int*)(tmp_ptr-INT_LEN) = size;
			*(int*)(tmp_ptr+size) = size;
			*(int*)(tmp_ptr+size+INT_LEN) = 1;
			allocate_block((void*)tmp_ptr, size, (-1)*extra,0);
		}else if(extra>0){								// extra size positive, need extra free memory for reallocate
			// check for adjacent for available space
			uintptr_t front = tmp_ptr - USED_BLOCK_HEADER_SIZE - BLOCK_FOOT_SIZE;
			uintptr_t back = tmp_ptr+get_blockSize((void*)tmp_ptr)+BLOCK_FOOT_SIZE+USED_BLOCK_HEADER_SIZE;
			if(back<(uintptr_t)sbrk(0)&&*(int*)(back-2*INT_LEN) == 0&&get_blockSize((void*)back)>=extra){		// check for available free block at back
				//puts("reallocate merge with back");
				int back_left = get_blockSize((void*)back)-extra;
				remove_block_freeList((void*)(back-INT_LEN));
				if(back_left!=0){							// if the block at back has free memory left, add back to free list
					*(int*)(tmp_ptr-INT_LEN) = size;
					*(int*)(tmp_ptr+get_blockSize((void*)tmp_ptr)) = get_blockSize((void*)tmp_ptr);
					*(int*)(tmp_ptr+get_blockSize((void*)tmp_ptr)+INT_LEN) = 1;
					uintptr_t left = tmp_ptr+get_blockSize((void*)tmp_ptr)+BLOCK_FOOT_SIZE+USED_BLOCK_HEADER_SIZE;
					*(int*)(left-INT_LEN) = back_left;
					*(int*)(left+get_blockSize((void*)left)) = back_left;
					add_block_freeList((void*)left);
				}else{
					*(int*)(tmp_ptr-INT_LEN) = get_blockSize((void*)tmp_ptr)+get_blockSize((void*)back)+USED_BLOCK_HEADER_SIZE+USED_BLOCK_HEADER_SIZE;
					*(int*)(tmp_ptr+get_blockSize((void*)tmp_ptr)) = get_blockSize((void*)tmp_ptr);
					*(int*)(tmp_ptr+get_blockSize((void*)tmp_ptr)+INT_LEN) = 1;
				}		// merge with back do not need to copy memory
			}else if(freeListHead!=NULL&&front>(uintptr_t)freeListHead&&*(int*)(front+INT_LEN)==0&&*(int*)front>=extra){		// check for available free block at front
				//puts("reallocate merge with front");
				front -= *(int*)front;
				int front_left = get_blockSize((void*)front)-extra;
				remove_block_freeList((void*)(front-INT_LEN));
				memcpy((void*)front, (void*)tmp_ptr, get_blockSize((void*)tmp_ptr));		// copy memory
				if(front_left!=0){												// if the block at the front has free memory left, add back to free list
					*(int*)(front-2*INT_LEN) = 1;
					*(int*)(front-INT_LEN) = size;
					*(int*)(front+get_blockSize((void*)front)) = get_blockSize((void*)front);
					*(int*)(front+get_blockSize((void*)front)+INT_LEN) = 1;
					uintptr_t left = front+get_blockSize((void*)front)+BLOCK_FOOT_SIZE+USED_BLOCK_HEADER_SIZE;
					*(int*)(left-INT_LEN) = front_left;
					*(int*)(left+get_blockSize((void*)left)) = front_left;
					add_block_freeList((void*)left);
				}else{
					*(int*)(front-INT_LEN) = get_blockSize((void*)front)+get_blockSize((void*)tmp_ptr)+USED_BLOCK_HEADER_SIZE+USED_BLOCK_HEADER_SIZE;
					*(int*)(front+get_blockSize((void*)front)) = get_blockSize((void*)front);
					*(int*)(front+get_blockSize((void*)front)+INT_LEN) = 1;
				}
				out = (void*) front;
			}else{												// if no adjacent front or back free block, then reallocate to new block of free memory using sma_malloc
				void* tmp = sma_malloc(size);
				memcpy(tmp, ptr, get_blockSize(ptr));			// copy the content to new free block
				sma_free(ptr);									// free the previous block
				out = tmp;
			}
		}
	}
	return out;

}

/*
 * =====================================================================================
 *	Private Functions for SMA
 * =====================================================================================
 */

/*
 *	Funcation Name: allocate_pBrk
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory by increasing the Program Break
 */
void *allocate_pBrk(int size)
{
	void *newBlock = NULL;
	int excessSize= EXCESS_SIZE;

	//	TODO: 	Allocate memory by incrementing the Program Break by calling sbrk() or brk()
	//	Hint:	Getting an exact "size" of memory might not be the best idea. Why?
	//			Also, if you are getting a larger memory, you need to put the excess in the free list

	//	Allocates the Memory Block

	if(freeListHead!=NULL&&*(int*)((uintptr_t)sbrk(0)-INT_LEN)==0){		// if there is free memory at the end of the heap but not big enough
		int len_left = *(int*)((uintptr_t)sbrk(0)-2*INT_LEN);
		void* tmp_new_block = sbrk( size - len_left + excessSize);		// allocate extra free memory needed with desired excess size
		uintptr_t cur_last = (uintptr_t)freeListTail+INT_LEN;
		newBlock = (void*)cur_last;
		remove_block_freeList((void*)cur_last);							// remove the tail free memory
	}else{
		newBlock = sbrk(USED_BLOCK_HEADER_SIZE + size + BLOCK_FOOT_SIZE + excessSize)+USED_BLOCK_HEADER_SIZE;		// if no free memory at the end of the heap, allocate new free memory block using sbrk
	}

	*(int*)((uintptr_t)newBlock-INT_LEN) = size;	// set size of the new block
	*(int*)((uintptr_t)newBlock-2*INT_LEN) = 1;		// set the free flag
	*(int*)((uintptr_t)newBlock+size) = size;		// set size
	*(int*)((uintptr_t)newBlock+size+INT_LEN) = 1;	// set the free flag

	allocate_block(newBlock, size, excessSize, 0);	

	return newBlock;
}

/*
 *	Funcation Name: allocate_freeList
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory from the free memory list
 */
void *allocate_freeList(int size)
{
	void *pMemory = NULL;

	if (currentPolicy == WORST)
	{
		// Allocates memory using Worst Fit Policy
		pMemory = allocate_worst_fit(size);
	}
	else if (currentPolicy == NEXT)
	{
		// Allocates memory using Next Fit Policy
		pMemory = allocate_next_fit(size);
	}
	else
	{
		pMemory = NULL;
	}

	return pMemory;
}

/*
 *	Funcation Name: allocate_worst_fit
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory using Worst Fit from the free memory list
 */
void *allocate_worst_fit(int size)
{
	void *worstBlock = NULL;
	int excessSize;
	int blockFound = 0;

	//	TODO: 	Allocate memory by using Worst Fit Policy
	//	Hint:	Start off with the freeListHead and iterate through the entire list to 
	//			get the largest block

	int largest = get_largest_freeBlock();					// get the size of the largest contigious free memory
	if(largest>size){										// if the largest size is greater than size wanted, find the largest free block and use it
		uintptr_t cur = (uintptr_t)freeListHead;
		while(cur!=0){
			if(*(int*)cur == largest){
				blockFound = 1;
				worstBlock = (void*)(cur+INT_LEN);
				excessSize = *(int*)cur-size;
				break;
			}
			cur = *(uintptr_t*)(cur+INT_LEN+ADR_LEN);
		}
	}

	if (blockFound){										// if proper block is found, set the flag and size
		*(int*)((uintptr_t)worstBlock-2*INT_LEN) = 1;
		*(int*)((uintptr_t)worstBlock-INT_LEN) = size;
		*(int*)((uintptr_t)worstBlock+get_blockSize(worstBlock)) = size;
		*(int*)((uintptr_t)worstBlock+get_blockSize(worstBlock)+INT_LEN) = 1;
	}

	//	Checks if appropriate block is found.
	if (blockFound)
	{
		//	Allocates the Memory Block
		allocate_block(worstBlock, size, excessSize, 1);
	}
	else
	{
		//	Assigns invalid address if appropriate block not found in free list
		worstBlock = (void *)-2;
	}

	return worstBlock;
}

/*
 *	Funcation Name: allocate_next_fit
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory using Next Fit from the free memory list
 */
void *allocate_next_fit(int size)
{
	void *nextBlock = NULL;
	int excessSize;
	int blockFound = 0;

	//	TODO: 	Allocate memory by using Next Fit Policy
	//	Hint:	You should use a global pointer to keep track of your last allocated memory address, and 
	//			allocate free blocks that come after that address (i.e. on top of it). Once you reach 
	//			Program Break, you start from the beginning of your heap, as in with the free block with
	//			the smallest address)

	if(freeListHead!=NULL){
		if(lastAllocated!=NULL&&(uintptr_t)lastAllocated<(uintptr_t)sbrk(0)){
		// search up
			uintptr_t cur = (uintptr_t)freeListHead;
			while(cur!=0&&cur<(uintptr_t)lastAllocated){
				cur = *(uintptr_t*)(cur+INT_LEN+ADR_LEN);
			}
			while(cur!=0){
				if(*(int*)cur>=size){
					blockFound = 1;
					nextBlock = (void*)(cur+INT_LEN);
					excessSize = *(int*)cur-size;
					break;
				}
				cur = *(uintptr_t*)(cur+INT_LEN+ADR_LEN);
			}
		}
		if(!blockFound){
			//search from start
			uintptr_t cur = (uintptr_t)freeListHead;
			while(cur!=0&&cur<(uintptr_t)lastAllocated){
				if(*(int*)cur>=size){
					blockFound = 1;
					nextBlock = (void*)(cur+INT_LEN);
					excessSize = *(int*)cur-size;
					break;
				}
				cur = *(uintptr_t*)(cur+INT_LEN+ADR_LEN);
			}
		}
		if(blockFound){
			*(int*)((uintptr_t)nextBlock-2*INT_LEN) = 1;
			*(int*)((uintptr_t)nextBlock-INT_LEN) = size;
			*(int*)((uintptr_t)nextBlock+get_blockSize(nextBlock)) = size;
			*(int*)((uintptr_t)nextBlock+get_blockSize(nextBlock)+INT_LEN) = 1;

		}
	}

	//	Checks if appropriate found is found.
	if (blockFound)
	{
		//	Allocates the Memory Block
		allocate_block(nextBlock, size, excessSize, 1);
	}
	else
	{
		//	Assigns invalid address if appropriate block not found in free list
		nextBlock = (void *)-2;
	}

	return nextBlock;
}

/*
 *	Funcation Name: allocate_block
 *	Input type:		void*, int, int, int
 * 	Output type:	void
 * 	Description:	Performs routine operations for allocating a memory block
 */
void allocate_block(void *newBlock, int size, int excessSize, int fromFreeList)
{
	void *excessFreeBlock; //	pointer for any excess free block
	int addFreeBlock;

	// 	Checks if excess free size is big enough to be added to the free memory list
	//	Helps to reduce external fragmentation

	//	TODO: Adjust the condition based on your Head and Tail size (depends on your TAG system)
	//	Hint: Might want to have a minimum size greater than the Head/Tail sizes
	addFreeBlock = excessSize > FREE_BLOCK_HEADER_SIZE+BLOCK_FOOT_SIZE;

	//	If excess free size is big enough
	if (addFreeBlock)
	{
		//	TODO: Create a free block using the excess memory size, then assign it to the Excess Free Block

		excessFreeBlock = (void*)((uintptr_t)newBlock+size+BLOCK_FOOT_SIZE+USED_BLOCK_HEADER_SIZE);
		int free_size = excessSize-USED_BLOCK_HEADER_SIZE-BLOCK_FOOT_SIZE;
		*(int*)((uintptr_t)excessFreeBlock-sizeof(int)) = free_size;	// set free block size
		*(int*)((uintptr_t)excessFreeBlock+free_size) = free_size;  	// set free block size
																		// free tag is set in add_block_freeList() function since sma_free() does not call allocate_block but the block from there also needs to set free tag
		
		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList)
		{
			//	Removes new block and adds the excess free block to the free list
			replace_block_freeList(newBlock, excessFreeBlock);
		}
		else
		{
			//	Adds excess free block to the free list
			add_block_freeList(excessFreeBlock);
		}
	}
	//	Otherwise add the excess memory to the new block
	else
	{
		//	TODO: Add excessSize to size and assign it to the new Block
		*(int*)(newBlock-INT_LEN) = size + excessSize;				// change size (add excess size to the previously allocated block)
		*(int*)(newBlock+size+excessSize) = size + excessSize;		// change size
		*(int*)(newBlock+size+excessSize+INT_LEN) = 1;				// set free tag
		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList)
		{
			//	Removes the new block from the free list
			remove_block_freeList(newBlock);
		}
	}
}

/*
 *	Funcation Name: replace_block_freeList
 *	Input type:		void*, void*
 * 	Output type:	void
 * 	Description:	Replaces old block with the new block in the free list
 */
void replace_block_freeList(void *oldBlock, void *newBlock)
{
	//	TODO: Replace the old block with the new block
	uintptr_t tmp_old = (uintptr_t) oldBlock;	// cast to uintptr_t
	uintptr_t tmp_new = (uintptr_t) newBlock;	// cast to uintptr_t

	// set new free
	*(int*)(tmp_new-2*INT_LEN) = 0;			//set free tag to new block
	*(int*)(tmp_new+get_blockSize((void*)tmp_new)+INT_LEN) = 0;
	// set new P and N
	*(uintptr_t*)(tmp_new) = 0;				// initialize P and N for new block with 0
	*(uintptr_t*)(tmp_new+ADR_LEN) = 0;

	uintptr_t old_pointer_next = *(uintptr_t*)(tmp_old+ADR_LEN);	// update N of new block
	*(uintptr_t*)(tmp_new+ADR_LEN) = old_pointer_next;
	uintptr_t old_pointer_pre = *(uintptr_t*)tmp_old;				// update P of new block
	*(uintptr_t*)tmp_new = old_pointer_pre;

	uintptr_t new_pointer = tmp_new-INT_LEN;		// pointer of the new block used for free list

	if(old_pointer_pre != 0){	// if there is a previous node, update the previous node
		*(uintptr_t*)(old_pointer_pre+INT_LEN+ADR_LEN) = new_pointer;
	}else{	// update head (if no previous, then it must be a head)
		freeListHead = (void*)new_pointer;
	}
	if(old_pointer_next != 0){	// if there is a next node, update the previous node
		*(uintptr_t*)(old_pointer_next+INT_LEN) = new_pointer;
	}else{	//update tail (if no next, then it must be a tail)
		freeListTail = (void*)new_pointer;
	}
	//	Updates SMA info
	totalAllocatedSize += get_blockSize(oldBlock);
	totalFreeSize -= get_blockSize(oldBlock);
}

/*
 *	Funcation Name: add_block_freeList
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Adds a memory block to the the free memory list
 */
void add_block_freeList(void *block)
{
	//	TODO: 	Add the block to the free list
	//	Hint: 	You could add the free block at the end of the list, but need to check if there
	//			exits a list. You need to add the TAG to the list.
	//			Also, you would need to check if merging with the "adjacent" blocks is possible or not.
	//			Merging would be tideous. Check adjacent blocks, then also check if the merged
	//			block is at the top and is bigger than the largest free block allowed (128kB).

	//	Updates SMA info
	totalAllocatedSize -= get_blockSize(block);		// update SMA first
	totalFreeSize += get_blockSize(block);
	
	// start to add to the free list
	int added = 0;		// flag to show if the free block is added to the free list
	uintptr_t tmp_block = (uintptr_t)block;		// cast to uintptr_t type for arithmetic

	// set free tag
	*(int*)(tmp_block-2*INT_LEN) = 0;
	*(int*)(tmp_block+get_blockSize((void*)tmp_block)+INT_LEN) = 0;
	// initialize P and N
	*(uintptr_t*)(tmp_block) = 0;
	*(uintptr_t*)(tmp_block+ADR_LEN) = 0;

	if(freeListHead == NULL){						// if no free list, create a free list
		freeListHead = (void*)(tmp_block-INT_LEN);
		freeListTail = (void*)(tmp_block-INT_LEN);
	}else{
		uintptr_t cur = (uintptr_t)freeListHead;	// check from the head of the free list
		while(cur!=0){
			if(cur>tmp_block){						// if current free list address greater than the new free memory block, add to the front of the current free node
				// add to proper location
				if(cur==(uintptr_t)freeListHead){
					*(uintptr_t*)(tmp_block+ADR_LEN) = (uintptr_t)freeListHead;	// new node N points to cur node
					*(uintptr_t*)(cur+INT_LEN) = tmp_block-INT_LEN;				// cur node P points to new node
					freeListHead = (void*)(tmp_block-INT_LEN);					
				}else{
					uintptr_t previous_node = *(uintptr_t*)(cur+INT_LEN);
					*(uintptr_t*)(previous_node+INT_LEN+ADR_LEN) = tmp_block-INT_LEN;	// previous node N points to new node
					*(uintptr_t*)(tmp_block) = previous_node;							// new node P points to previous node
					*(uintptr_t*)(tmp_block+ADR_LEN) = cur;							// new node N points to cur node
					*(uintptr_t*)(cur+INT_LEN) = tmp_block-INT_LEN;				// cur node P points to new node
				}
				added = 1;
				break;
			}
			cur = *(uintptr_t*)(cur+INT_LEN+ADR_LEN);
		}
		if(!added){				// if not added, add to the tail of the free list
			*(uintptr_t*)((uintptr_t)freeListTail+INT_LEN+ADR_LEN) = tmp_block-INT_LEN;		
			*(uintptr_t*)(tmp_block) = (uintptr_t)freeListTail;							
			freeListTail = (void*)(tmp_block-INT_LEN);
		}
		
		// start to merge
		uintptr_t potential_merge_next = tmp_block+get_blockSize(block)+BLOCK_FOOT_SIZE;		// potential back free block
		uintptr_t potential_merge_pre = tmp_block-USED_BLOCK_HEADER_SIZE-INT_LEN;				// potential front free block
		int merge_next = 0;		// flag for merge with back
		int merge_pre = 0;		// flag for merge with front
		if(potential_merge_next<(uintptr_t)(sbrk(0))&&*(int*)potential_merge_next==0){			// if can merge with back
			merge_next = 1;
			potential_merge_next = potential_merge_next+USED_BLOCK_HEADER_SIZE;
		}
		if(potential_merge_pre>(uintptr_t)freeListHead&&*(int*)potential_merge_pre==0){			// if can merge with front
			merge_pre = 1;
			potential_merge_pre = potential_merge_pre-INT_LEN-*(int*)(potential_merge_pre-INT_LEN);
		}
		if(merge_pre){		// merge with front
			merge_with_front(potential_merge_pre,tmp_block);
			tmp_block = potential_merge_pre;
		}
		if(merge_next){		// merge with with back
			merge_with_front(tmp_block,potential_merge_next);
		}
	}
	//--------------------------------------------------------------------------------------------------------------
	if((uintptr_t)freeListTail+INT_LEN+*(int*)freeListTail+BLOCK_FOOT_SIZE==(uintptr_t)(sbrk(0))&&(*(int*)freeListTail) > MAX_TOP_FREE){	// top larger than 128kB, chop off
		// bring break pointer down
		int size = *(int*)freeListTail;
		if((uintptr_t)freeListHead==(uintptr_t)freeListTail){		// if this is the only free memory, reset free list 
			freeListHead=NULL;
			freeListTail=NULL;
			int i = brk((void*)((uintptr_t)freeListTail-INT_LEN));
		}else{														// remove the tail of the free list
			uintptr_t newListTail = *(uintptr_t*)((uintptr_t)freeListTail+INT_LEN);
			int i = brk((void*)((uintptr_t)freeListTail-INT_LEN));
			freeListTail = (void*)newListTail;
			*(uintptr_t*)((uintptr_t)freeListTail+INT_LEN+ADR_LEN) = 0;
		}
		totalFreeSize -= size;
	}
}

/*
 *	Funcation Name: remove_block_freeList
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Removes a memory block from the the free memory list
 */
void remove_block_freeList(void *block)
{
	//	TODO: 	Remove the block from the free list
	//	Hint: 	You need to update the pointers in the free blocks before and after this block.
	//			You also need to remove any TAG in the free block.

	//	Updates SMA info

	// record N and P
	uintptr_t pointer_next = *(uintptr_t*)(block+ADR_LEN); 
	uintptr_t pointer_pre = *(uintptr_t*)block;

	if(pointer_next == 0 && pointer_pre ==0){		// if it is the only free memory of the free list, reset
		freeListHead = NULL;
		freeListTail = NULL;
	}else if(pointer_next == 0){	//if tail, change tail
		*(uintptr_t*)(pointer_pre+INT_LEN+ADR_LEN) = 0;	// set the new tail to have next of 0
		freeListTail = (void*)pointer_pre;
	}else if(pointer_pre ==0){		//if head, change head
		*(uintptr_t*)(pointer_next+INT_LEN) = 0;	//set the new head to have previous of 0
		freeListHead = (void*)pointer_next;
	}else{
		*(uintptr_t*)(pointer_pre+INT_LEN+ADR_LEN) = pointer_next;	// update N of the previous node
		*(uintptr_t*)(pointer_next+INT_LEN) = pointer_pre;			// update P of the next node
	}

	totalAllocatedSize += get_blockSize(block);
	totalFreeSize -= get_blockSize(block);
}

/*
 *	Funcation Name: get_blockSize
 *	Input type:		void*
 * 	Output type:	int
 * 	Description:	Extracts the Block Size
 */
int get_blockSize(void *ptr)
{
	int *pSize;

	//	Points to the address where the Length of the block is stored
	pSize = (int *)ptr;
	pSize--;

	//	Returns the deferenced size
	return *(int *)pSize;
}

/*
 *	Funcation Name: get_largest_freeBlock
 *	Input type:		void
 * 	Output type:	int
 * 	Description:	Extracts the largest Block Size
 */
int get_largest_freeBlock()
{
	int largestBlockSize = 0;

	//	TODO: Iterate through the Free Block List to find the largest free block and return its size
	if(freeListHead==NULL){			// if no free list
		largestBlockSize = 0;
	}else{
		uintptr_t cur = (uintptr_t)freeListHead;
		while(cur!=0){							// loop the free list, find the largest block size
			if(largestBlockSize<*(int*)cur){
				largestBlockSize = *(int*)cur;
			}
			cur = *(uintptr_t*)(cur+INT_LEN+ADR_LEN);
		}
	}

	return largestBlockSize;
}

/**
 * Self added functions
**/

/*
 *	Funcation Name: merge_with_front
 *	Input type:		uintptr_t uintptr_t
 * 	Output type:	void
 * 	Description:	merge two blocks of free memory
 */
void merge_with_front(uintptr_t front, uintptr_t back){
	*(int*)(front-INT_LEN) = get_blockSize((void*)front)+USED_BLOCK_HEADER_SIZE+BLOCK_FOOT_SIZE+get_blockSize((void*)back);	//update size in header
	*(int*)(front+get_blockSize((void*)front)) = get_blockSize((void*)front);		// update size at the end of the block
	*(uintptr_t*)(front+ADR_LEN) = *(uintptr_t*)(back+ADR_LEN);				// update N
	//uintptr_t block_next = *(uintptr_t*)(back+ADR_LEN);
	if(back-INT_LEN == (uintptr_t)freeListTail){	// if the new block is tail, set it to tail
		freeListTail = (void*)(front-INT_LEN);
	}else{
		*(uintptr_t*)(*(uintptr_t*)(front+ADR_LEN)+INT_LEN) = front-INT_LEN;	// update P of the next free block
	}
}

/*
 *	Funcation Name: show_freeList
 *	Input type:		void
 * 	Output type:	void
 * 	Description:	show current status of free list
 */
void show_freeList(){
	char str[60];
	if(freeListHead!=NULL){
		int size;
		uintptr_t adr;
		uintptr_t cur = (uintptr_t)freeListHead;
		int counter = 0;
		sprintf(str,"Start of free list-----------------");
		puts(str);
		while(cur!=0){
			size = *(int*)cur;
			sprintf(str,"%d. free address: %p, free size: %d", counter,(void*)cur, size);
			puts(str);
			counter++;
			cur = *(uintptr_t*)((uintptr_t)cur+INT_LEN+ADR_LEN);
		}
		sprintf(str,"End of free list-----------------");
		puts(str);
	}else{
		sprintf(str,"nothing in the free list");
		puts(str);
	}
}