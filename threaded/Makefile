
CC=gcc
SOURCE=src/main.c
EXECUTABLE=threaded

all: programa clean
	
programa: 
	$(CC) -pthread -o $(EXECUTABLE) $(SOURCE)

clean:
	rm -f *.o
