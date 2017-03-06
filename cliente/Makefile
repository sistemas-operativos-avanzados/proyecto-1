# Makefile para el programa rastreador. Cuando se ejecuta el commando 'make' se compila y se crea el ejecutable.
# La carpeta 'target/' va a servir como punto en donde se van a dejar los archivos resultantes del proceso
# de compilacion

CC=gcc
SOURCES=cliente.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=cliente
SDIR=src
ODIR=target

all: clean $(ODIR) cliente

cliente.o: $(SDIR)/*.c
	$(CC) -c -pthread $(SDIR)/$(SOURCES) -o $(ODIR)/$(OBJECTS)

cliente: cliente.o
	$(CC) -pthread $(ODIR)/cliente.o -o $(ODIR)/$(EXECUTABLE)

$(ODIR):
	mkdir target/

clean:
	rm -rf target/
