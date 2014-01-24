all: ptar

ptar: ptar.o
	gcc -pthread -o ptar ptar.o -lm

ptar.o: ptar.c
	gcc -o ptar.o -c ptar.c -lm
clean:
	rm *.o

mrproper: clean
	rm ptar
