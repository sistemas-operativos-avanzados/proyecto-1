# Makefile para el programa rastreador. Cuando se ejecuta el commando 'make' se compila y se crea el ejecutable.
# La carpeta 'target/' va a servir como punto en donde se van a dejar los archivos resultantes del proceso
# de compilacion

CC=gcc
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=secuencial
SDIR=src
ODIR=target

all: clean $(ODIR) secuencial

main.o: $(SDIR)/*.c
	$(CC) -c $(SDIR)/$(SOURCES) -o $(ODIR)/$(OBJECTS)

secuencial: main.o
	$(CC) $(ODIR)/main.o -o $(ODIR)/$(EXECUTABLE)

$(ODIR):
	mkdir target/

clean:
	rm -rf target/