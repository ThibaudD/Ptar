CC=gcc
FLAGS=-lm

all: ptar
	rm *.o

ptar: ptar.o
	$(CC) -pthread -o ptar ptar.o $(FLAGS)

ptar.o: ptar.c
	$(CC) -o ptar.o -c ptar.c $(FLAGS)
clean:
	rm -rf *.o

mrproper: clean
	rm -rf ptar
