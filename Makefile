CC=g++

all: main

main: main.c
	$(CC) -g -o run main.c

clean:
	rm run
