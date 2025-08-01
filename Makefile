CC=g++

all: main

main: main.c
	$(CC) -o run main.c

clean:
	rm run
