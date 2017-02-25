# Makefile para el programa rastreador. Cuando se ejecuta el commando 'make' se compila y se crea el ejecutable.
# La carpeta 'target/' va a servir como punto en donde se van a dejar los archivos resultantes del proceso
# de compilacion

CC=gcc
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=threaded
SDIR=src
ODIR=target

all: clean $(ODIR) threaded

main.o: $(SDIR)/*.c
	$(CC) -pthreaded -c $(SOURCES) -o $(OBJECTS)

threaded: main.o
	$(CC) main.o -o $(EXECUTABLE)

$(ODIR):
	mkdir target/

clean: cleanobj cleanexec

cleanobj:
    rm main.o

cleanexec:
    rm $(EXECUTABLE)