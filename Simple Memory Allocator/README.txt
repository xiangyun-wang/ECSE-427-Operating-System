1. The original test file "a3_test.c" is used without any modification, and the test result is included in the folder. 

2. The value for the excessSize is a little bit tricky to choose. Some of the values may fail certain tests due to free 
   memory merging. In my "sma.c" file, I set the excessSize to 100*1024 Byte, and this value should be able to pass all 
   the tests.

3. In order to compile, run and test the code, a Makefile is created. 
	The testing routine is as follow:
	1. Navigate to current folder in the terminal and type "make sma_test" to compile sma.c and a3_test.c
	2. Type "./test" to run the test, and the result should appear in the terminal
	
	(If object file is wanted, you can type "make sma" to get "sma.o"...) 

4. Changes to the Public Functions
	1. "lastAllocated = pMemory;" is added at the end of "void* sma_malloc(int size)", to record the pointer of the 
	   last allocated memory.
	2. "char str[60]" is changed to "char str[70]" in "void sma_mallinfo()" to avoid warning. 

5. Question: Why the methods are decleared to be static in the header file?