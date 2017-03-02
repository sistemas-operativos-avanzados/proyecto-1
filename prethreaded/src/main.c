#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>




typedef struct {
    pthread_t		thread_tid;		/* thread ID */
    long			thread_count;	/* # connections handled */
} Thread;
Thread	*tptr;		/* array of Thread structures; calloc'ed */

#define	MAXNCLI	32
int					clifd[MAXNCLI], iget, iput;
pthread_mutex_t		clifd_mutex;
pthread_cond_t		clifd_cond;

static int			nthreads;
pthread_mutex_t		clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;



char notFoundPage[] = "<html><head><title>404</head></title>"
        "<body><p>404: El recurso solicitado no se encontró</p></body></html>";

char* ROOT_FOLDER = "web-resources";


typedef struct {
    char *ext;
    char *mediatype;
} extn;


extn extensions[] ={
        {".gif", "image/gif" },
        {".txt", "text/plain" },
        {".jpg", "image/jpg" },
        {".jpeg","image/jpeg"},
        {".png", "image/png" },
        {".ico", "image/ico" },
        {".zip", "image/zip" },
        {".gz",  "image/gz"  },
        {".tar", "image/tar" },
        {".htm", "text/html" },
        {".html","text/html" },
        {".php", "text/html" },
        {".css", "text/css"},
        {".pdf","application/pdf"},
        {".js", "application/javascript"},
        {".zip","application/octet-stream"},
        {".rar","application/octet-stream"},
        {0,0}
};



char *okHeader = "HTTP/1.1 200 OK\r\nContent-Type: %s charset=UTF-8\r\nServer : SOA-Server-Forked\r\n\r\n";
char *notFoundHeader = "HTTP/1.1 400 Not Found\r\nContent-Type: text/html charset=UTF-8\r\nServer : SOA-Server-Forked\r\n\r\n";
char *notSupportedHeader = "HTTP/1.1 415 Unsupported Media Type\r\nnServer : SOA-Server-Forked\r\n\r\n";



/*
 * Esta funcion actua como manejador de la "interrupcion" del Crtl+C
 */
void handleShutdown(int sig){

    printf("\nfinalizando... \n");
    int i;

    for (i = 0; i < nthreads; i++)
        printf("thread %d, %ld connections\n", i, tptr[i].thread_count);

    printf("\nSOA-Server-Prethreaded: Bye! \n");
    exit(0);
}


/*
 * Esta funcion captura la señal de Crtl+C, para llamar a finalizar al programa
 */
int catch_signal(int sig, void (*handler)(int)){
    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    return sigaction(sig, &action, NULL);
}



int openListener(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        printf("No se pudo abrir el socket \n");
        exit(1);
    }
}


void bindToPort(int socket, int port){
    struct sockaddr_in server_addr;
    int reuse = 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1){
        printf("No se pudo establecer opcion reuse en el socket \n");
    }

    if(bind(socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
        printf("bind \n");
        close(socket);
        exit(1);
    }
}





void web_child(int sockfd) {

    printf("web client %d \n", sockfd);
    return;

//    for ( ; ; ) {
//
//        printf("inside web client \n");

//        if ( (nread = Readline(sockfd, line, MAXLINE)) == 0)
//            return;		/* connection closed by other end */

        /* 4line from client specifies #bytes to write back */
//        ntowrite = atol(line);
//        if ((ntowrite <= 0) || (ntowrite > MAXN))
//            err_quit("client request for %d bytes", ntowrite);

//        Writen(sockfd, result, ntowrite);
//    }
}

char *successHeader(char *mimeType){

    char header[500];
    sprintf(header, okHeader, mimeType);
    return strdup(header);
}


char *mimeType(char* resourceExt){

    int i;
    for (i = 0; extensions[i].ext != NULL; i++) {
        if (strcmp(resourceExt, extensions[i].ext) == 0) {
            return extensions[i].mediatype;
        }
    }

    return "application/octed-stream";
}




int getFileSize(int fd) {
    struct stat stat_struct;
    if (fstat(fd, &stat_struct) == -1)
        return (1);
    return (int) stat_struct.st_size;
}



void processRequest(int fd_client){

    char buf[2048];
    char filePath[500];

    puts("==> Conexion iniciada\n");
    memset(buf, 0, 2048); //     <==== aqui se va a dejar todo lo que viene en la peticion HTTP
    memset(filePath, 0, 500); // <==== aqui se va a dejar la ruta al archivo
    //TODO: valorar si cambiar este read por transmision bit a bit
    //TODO: validar si la peticion es de tipo HTTP GET, sino descartarla


    read(fd_client, buf, 2047);

    //Creando la ruta hacia el archivo, esto deberia de ir en una funcion por aparte
    strcpy(filePath, ROOT_FOLDER);
    int i = 4, j = 0;
    char urlPath[500];
    memset(urlPath, 0, 500);
    while (buf[i] != ' ') { // <=== iterar hasta encontrar el primer espacio en blanco, hasta alli llega el path
        urlPath[j] = buf[i];
        i++;
        j++;
    }
    strcat(filePath, urlPath);
    printf("path = %s \n", filePath);


    int fileResource = open(filePath, O_RDONLY);
    printf("file = %d\n", fileResource);
    if (fileResource != -1) {

        char *mime = mimeType(strstr(filePath, "."));
        char *header = successHeader(mime);
        printf("HEADER = %s", header);

        write(fd_client, header, strlen(header));

        int length;
        if ((length = getFileSize(fileResource)) == -1) {
            printf("Error obtiendo tamanno de archivo\n");
        }

        size_t total_bytes_sent = 0;
        ssize_t bytes_sent;
        while (total_bytes_sent < length) {
            if ((bytes_sent = sendfile(fd_client, fileResource, 0, length - total_bytes_sent)) <= 0) {
                puts("sendfile error\n");
//                return -1;
            }
            total_bytes_sent += bytes_sent;
        }
        close(fileResource);


    } else {
        printf("Archivo no se encuentra \n");
        write(fd_client, notFoundHeader, strlen(notFoundHeader));
        write(fd_client, notFoundPage, strlen(notFoundPage));
    }

    puts("<== Finalizando conexion\n\n");
}




void thread_make(int i) {
    void	*thread_main(void *);
    printf("inside thread_make i = %d\n", i);
    pthread_create(&tptr[i].thread_tid, NULL, &thread_main, &i);
    return;		/* main thread returns */
}

void *thread_main(void *arg) {
    int		connfd;
    void	web_child(int);

    int theArg = *((int *) arg);

    printf("thread %d starting\n", theArg);
    for ( ; ; ) {
        printf("1 -> iput %d, iget %d \n", iput, iget);
        pthread_mutex_lock(&clifd_mutex);
        printf("2 -> iput %d, iget %d \n", iput, iget);
        while (iget == iput) {
            printf("1 \n");
            pthread_cond_wait(&clifd_cond, &clifd_mutex);
        }
        connfd = clifd[iget];	/* connected socket to service */
        if (++iget == MAXNCLI)
            iget = 0;
        pthread_mutex_unlock(&clifd_mutex);
        tptr[theArg].thread_count++;

//        web_child(connfd);		/* process request */
        processRequest(connfd);
        close(connfd);
    }
}

int main(int argc, char **argv) {
    int			i, listenfd, connfd;
//    void		sig_int(int), thread_make(int);
    socklen_t	addrlen, clilen;
    struct sockaddr	*cliaddr;

//    if (argc == 3)
//        listenfd = Tcp_listen(NULL, argv[1], &addrlen);
//    else if (argc == 4)
//        listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
//    else
//        err_quit("usage: serv08 [ <host> ] <port#> <#threads>");


    listenfd = openListener();
    bindToPort(listenfd, 8080);

    if(listen(listenfd, 10) == -1){ // una cola de 10 listeners
        printf("\n listen error \n");
        close(listenfd);
        exit(1);
    }

    cliaddr = malloc(addrlen);

//    nthreads = atoi(argv[argc-1]);
    nthreads = 2;
    tptr = calloc(nthreads, sizeof(Thread));
    iget = iput = 0;

    /* 4create all the threads */
    for (i = 0; i < nthreads; i++)
        thread_make(i);		/* only main thread returns */

    if(catch_signal(SIGINT, handleShutdown) == -1){
        printf("No se pudo mapear el manejador");
        exit(1);
    }

    for ( ; ; ) {
        clilen = addrlen;
        connfd = accept(listenfd, cliaddr, &clilen);

        pthread_mutex_lock(&clifd_mutex);
        clifd[iput] = connfd;
        printf("0. iput %d, iget %d \n", iput, iget);
        if (++iput == MAXNCLI)
            iput = 0;
        if (iput == iget) {
//            err_quit("iput = iget = %d", iput);
            printf("iput = iget = %d \n", iput);
            exit(1);
        }
        pthread_cond_signal(&clifd_cond);
        pthread_mutex_unlock(&clifd_mutex);
    }
}

