# Makefile para el programa rastreador. Cuando se ejecuta el commando 'make' se compila y se crea el ejecutable.
# La carpeta 'target/' va a servir como punto en donde se van a dejar los archivos resultantes del proceso
# de compilacion

CC=gcc
SOURCES=client.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=client
SDIR=src
ODIR=target

all: clean $(ODIR) client

client.o: $(SDIR)/*.c
	$(CC) -c -pthread $(SDIR)/$(SOURCES) -o $(ODIR)/$(OBJECTS)

client: client.o
	$(CC) -pthread $(ODIR)/client.o -o $(ODIR)/$(EXECUTABLE)

$(ODIR):
	mkdir target/

clean:
	rm -rf target/
