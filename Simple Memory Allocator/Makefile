# .DEFAULT_GOAL=all

CC=gcc
CFLAGS=-fsanitize=signed-integer-overflow -fsanitize=undefined -g -std=gnu99 -O2 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow
LDFLAGS = -pthread
BINARY = -c
DESTINATION = sma.o
DESTINATION_TEST=test

sma: sma.c
	$(CC) -o $(DESTINATION) $(BINARY) sma.c

sma_test: a3_test.c sma.c
	$(CC) -o $(DESTINATION_TEST) $(CFLAGS) $(LDFLAGS) a3_test.c sma.c

clean:
	rm -rf $(DESTINATION)

clean_test:
	rm -rf $(DESTINATION_TEST)

