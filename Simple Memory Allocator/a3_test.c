/*
 * =====================================================================================
 *
 *	Filename:  		a3_test.c
 *
 * 	Description:	Example of testing code of MyMalloc.
 *
 *  Version:  		1.0
 *  Created:  		6/11/2020 9:30:00 AM
 *  Revised:  		-
 *  Compiler:  		gcc
 *
 *  Authors:  		Devarun Bhattacharya, 
 * 					Mohammad Mushfiqur Rahman
 * 
 * 	Intructions:	Please address all the "TODO"s in the code below and modify 
 * 					them accordingly. No need to modify anything else, unless you 
 * 					find a bug in the tester! Don't modify the tester to circumvent 
 * 					the bug in your code!
 * =====================================================================================
 */

/* Includes */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "sma.h"

int main(int argc, char *argv[])
{
	int i, count = 0;
	void *ptr, *limitafter = NULL, *limitbefore = NULL;
	char *c[32], *ct;
	int *c2[32];
	char str[60];

	// Test 1: Find the holes
	puts("Test 1: Excess Memory Allocation...");

	// Allocating 32 kbytes of memory..
	for (i = 0; i < 32; i++)
	{
		c[i] = (char *)sma_malloc(1024);
		//sma_mallinfo();
		// sprintf(str, "c[i]: %p", c[i]);
		// puts(str);
	}

	// Now deallocating some of the slots ..to free
	for (i = 10; i < 18; i++)
	{
		sma_free(c[i]);
		//sma_mallinfo();
		//sprintf(str, "Freeing c[%d]: %p", i,c[i]);
		//puts(str);
	}

	// Allocate some storage .. this should go into the freed storage
	ct = (char *)sma_malloc(5 * 1024);
	//sma_mallinfo();
	// sprintf(str, "CT : %p", ct);
	// puts(str);

	// Testing if you are allocating excess memory at the end
	if (ct > c[31])
		puts("\t\t\t\t PASSED\n");
	else
		puts("\t\t\t\t FAILED\n");
	// Test 2: Program Break expansion Test
	puts("Test 2: Program break expansion test...");

	count = 0;
	for (i = 1; i < 40; i++)
	{
		limitbefore = sbrk(0);
		ptr = sma_malloc(1024 * 32 * i);
		//sma_mallinfo();
		limitafter = sbrk(0);

		if (limitafter > limitbefore)
			count++;
	}

	// Testing if the program breaks are incremented correctly
	if (count > 0 && count < 40)
		puts("\t\t\t\t PASSED\n");
	else
		puts("\t\t\t\t FAILED\n");
	
	// Test 3: Worst Fit Test
	puts("Test 3: Check for Worst Fit algorithm...");
	// Sets Policy to Worst Fit
	sma_mallopt(WORST_FIT);

	// Allocating 512 kbytes of memory..
	for (i = 0; i < 32; i++){
		c2[i] = (int *)sma_malloc(16 * 1024);
		//sprintf(str,"trial %d", i);
		//puts(str);
	}
	
	// Now deallocating some of the slots ..to free
	// One chunk of 5x16 kbytes
	sma_free(c2[31]);
	sma_free(c2[30]);
	sma_free(c2[29]);
	sma_free(c2[28]);
	sma_free(c2[27]);
	// One chunk of 3x16 kbytes
	sma_free(c2[25]);
	sma_free(c2[24]);
	sma_free(c2[23]);

	// One chunk of 2x16 kbytes
	sma_free(c2[20]);
	sma_free(c2[19]);

	// One chunk of 3x16 kbytes
	sma_free(c2[10]);
	sma_free(c2[9]);
	sma_free(c2[8]);

	// One chunk of 2x16 kbytes
	sma_free(c2[5]);
	sma_free(c2[4]);

	int *cp2 = (int *)sma_malloc(16 * 1024 * 2);

	// Testing if the correct hole has been allocated
	if (cp2 != NULL)
	{
		if (cp2 == c2[27] || cp2 == c2[28] || cp2 == c2[29] || cp2 == c2[30])
			puts("\t\t\t\t PASSED\n");
		else
			puts("\t\t\t\t FAILED\n");
	}
	else
	{
		puts("\t\t\t\t FAILED\n");
	}

	//	Freeing cp2
	sma_free(cp2);

	// Test 4: Next Fit Test
	puts("Test 4: Check for Next Fit algorithm...");
	// Sets Policy to Next Fit
	sma_mallopt(NEXT_FIT);
	//puts("test 4 1");
	int *cp3 = (int *)sma_malloc(16 * 1024 * 3);
	//puts("test 4 2");
	int *cp4 = (int *)sma_malloc(16 * 1024 * 2);

	// Testing if the correct holes have been allocated
	if (cp3 == c2[8] && cp3 != NULL)
	{
		if (cp4 == c2[19])
		{
			puts("\t\t\t\t PASSED\n");
		}
		else
		{
			puts("\t\t\t\t FAILED\n");
		}
	}
	else
	{
		puts("\t\t\t\t FAILED\n");
	}

	// Test 5: Realloc test (with Next Fit)
	puts("Test 5: Check for Reallocation with Next Fit...");
	// Writes some value pointed by the pointer
	if(cp3 != NULL && cp4 != NULL) {
		*cp3 = 427;
		*cp4 = 310;
	}
	
	// Calling realloc
	cp3 = (int *)sma_realloc(cp3, 16 * 1024 * 5);
	cp4 = (int *)sma_realloc(cp4, 16 * 1024 * 3);

	if (cp3 == c2[27] && cp3 != NULL && cp4 == c2[8] && cp4 != NULL)
	{
		//	Test the Data stored in the memory blocks
		if (*cp3 == 427 && *cp4 == 310) {
			puts("\t\t\t\t PASSED\n");
		}
		else {
			puts("inner if fail");
			puts("\t\t\t\t FAILED\n");
		}				
	}
	else
	{
		puts("\t\t\t\t FAILED\n");
	}

	//	Test 6: Print Stats
	puts("Test 6: Print SMA Statistics...");
	puts("===============================");
	sma_mallinfo();

	return (0);
}