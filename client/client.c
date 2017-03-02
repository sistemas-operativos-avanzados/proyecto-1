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
int d_sock, ncycles = 0;

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
  int cycles = 0;
  while(cycles < ncycles){
    do{
       char buf[1023];
       sprintf(buf, "GET /%s HTTP/1.1\r\nHost:%s\r\n\r\n", filename, host);
    
       if((send(d_sock, buf, strlen(buf), 0)) == 0)
         error("Error to send");

       char rec[1024];
       ssize_t bytesRcvd;
       if ((bytesRcvd = recv(d_sock, rec, 1024, 0)) == 0)
            break;

        bytesRcvd = read(d_sock, rec, 1024);
        rec[bytesRcvd] = '\0';
        //printf("%s", rec);

    }while (1); 
  cycles++;
}
 close(d_sock);
 return (NULL);
}

int main(int argc, char* argv[]) {
    
    if(argc != 6){
        //ei. ./client proveinca.com 80 fs_files/user_img/Composicion%20PEX-AL-PEX.jpg 2 120

        printf("\n Usage: %s <host> <port> <filename> <n-threads> <n-cycles>", argv[0]);
        return 1;
    }
    int nthreads = 0;
    
    host = argv[1];
    portno = argv[2];
    filename = argv[3];
    nthreads = atoi(argv[4]);
    ncycles = atoi(argv[5]);
    int i = 1, y = 1;
    pthread_t tid;

    while(i <= nthreads)
    {
        d_sock = open_socket(host, portno);
	int err = pthread_create(&(tid), NULL, &make_request, NULL);
        printf("Thread id %lu fue creado correctamente\n", pthread_self());
	if (err != 0)
	    error("Can't create de thread");
	 else
         pthread_join(tid, NULL);
     i++;
     }
		
    return 0;
}
