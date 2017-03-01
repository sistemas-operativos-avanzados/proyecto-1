#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include<pthread.h>

char *host, *filename, *portno;
int d_sock;

void error(char* msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

int open_socket(char* host, char* port) {
    struct addrinfo *res;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) == -1) {
        error("Can't resolve the address");
    }

    int d_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (d_sock == -1) {
        error("Can't open socket");
    }
    int c = connect(d_sock, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    if (c == -1) {
        error("Can't connect to socket");
    }
    return d_sock;
}

static void *make_request() {
    char buf[255];
    sprintf(buf, "GET /%s HTTP/1.1\r\nHost:%s\r\n\r\n", filename, host);
    send(d_sock, buf, strlen(buf), 0);
    char rec[256];
    int bytesRcvd;
    long unsigned bytes;

    do{
      	bytesRcvd = recv(d_sock, rec, 256, 0);
	if (bytesRcvd < 0)
            error("Can't read from server");

        if (bytesRcvd == 0)
            break;

        rec[bytesRcvd] = '\0';
        bytes = bytes + sizeof(rec);
	    //Print received data
        //printf("%s", rec);

    //}while (1); //Esta deberia ser la condicion correcta
    
    }while (bytesRcvd == 256);//Temporal workaround: when the server send least than 256 bytes the client keeps listening, server doesn't send any data and program never closes the connection
 
 return (NULL);
}

int main(int argc, char* argv[]) {
    
    if(argc != 6){
        //ei. ./client www.smartercommerce.net 80 resources/webinars 3 2
        printf("\n Usage: %s <host> <port> <filename> <n-threads> <n-cycles>", argv[0]);
        return 1;
    }
    int nthreads = 0, ncycles = 0;
    
    host = argv[1];
    portno = argv[2];
    filename = argv[3];
    nthreads = atoi(argv[4]);
    ncycles = atoi(argv[5]);
    d_sock = open_socket(host, portno);

    int i = 0, y = 0;
    int err;

    while(i < nthreads)
    {
        
	y = 0;
	do{
        //Si el numero de ciclos es 0, igualmente, crea el thread y lo ejecuta una vez
        pthread_t tid;
	    err = pthread_create(&(tid), NULL, &make_request, NULL);
        
	   if (err != 0)
	    printf("\ncan't create thread :[%s]", strerror(err));
	   else{
	    int threadno = i + 1;
        int cycleno = y + 1;
	    printf("Thread %i del ciclo %i fue creado correctamente\n", threadno, cycleno);
	    pthread_join(tid, NULL);
	   } 
		y++;
	    }while(y < ncycles);
     i++;
     }
		
    close(d_sock);
    return 0;
}
