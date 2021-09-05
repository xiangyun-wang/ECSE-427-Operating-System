# .DEFAULT_GOAL=all

CC=gcc
BINARY = -c
DESTINATION = sut.o

sut: sut.c
	$(CC) -o $(DESTINATION) $(BINARY) sut.c

clean:
	rm -rf $(DESTINATION)
