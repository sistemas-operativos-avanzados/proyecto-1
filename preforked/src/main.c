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


char webpage[] =
        "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                "<!doctype html>\r\n"
                "<html><head><title>Mi pagina</title></head>\r\n"
                "<body><h1>Bienvenidos</h1>\r\n"
                "<p>Esta es mi linda pagina!!!<br/>\r\n"
                "<a><img src=\"cowboy.jpg\" title=\"un Cowboy\"></a></p>\r\n"
                "</body></html>\r\n"
;


char notFoundPage[] = "<html><head><title>404</head></title>"
        "<body><p>404: El recurso solicitado no se encontró</p></body></html>";

int fd_server;
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

static int		nchildren;
static pid_t	*pids;

// -----------------------------------------------------------------------------------------------------

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



/*
 * Esta funcion actua como manejador de la "interrupcion" del Crtl+C
 */
void handleShutdown(int sig){

    printf("\nfinalizando...");
    int i;

    /* terminate all children */
    for (i = 0; i < nchildren; i++) {
        printf("Finalizando PID %d \n", pids[i]);
        kill(pids[i], SIGTERM);
    }

    while (wait(NULL) > 0) ;        /* wait for all children */

    if (errno != ECHILD) {
        printf("wait error");
    }

    printf("\nSOA-Server-Preforked: Bye! \n");
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

/*
 *
 */
int openListener(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        printf("No se pudo abrir el socket \n");
        exit(1);
    }
}

/*
 *
 */
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

int getFileSize(int fd) {
    struct stat stat_struct;
    if (fstat(fd, &stat_struct) == -1)
        return (1);
    return (int) stat_struct.st_size;
}


// MAIN -----------------------------------------------------------------------------------------------------

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
    printf("file = %d", fileResource);
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




void child_main(int i, int listedfd, int addrlen){

    int connfd;
    socklen_t clilen;
    struct sockaddr *cliaddr;

    cliaddr = malloc(addrlen);
    int pid = getpid();
    printf("Proceso %ld iniciando \n", (long) pid);

    for( ; ; ){
        clilen = addrlen;
        connfd = accept(listedfd, cliaddr, &clilen);
        printf("Solicitud atendida por PID %d\n", pid);

        processRequest(connfd);

        close(connfd);
    }

}



pid_t  child_make(int i, int listedfd, int addrlen){

    pid_t pid;

    if((pid = fork()) > 0){
        return pid;
    }

    child_main(i, listedfd, addrlen);

}


int main(int argc, char *argv[]){

    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_len = sizeof(client_addr);
    char buf[2048];
    char filePath[500];
    int i;

    fd_server = openListener();
    //TODO: Valor del puerto tiene que ser pasado por parametro
    bindToPort(fd_server, 8080);

    if(listen(fd_server, 10) == -1){ // una cola de 10 listeners
        printf("\n listen error \n");
        close(fd_server);
        exit(1);
    }


    nchildren = 5;
    pids = calloc(nchildren, sizeof(pid_t));

    for (i = 0; i < nchildren; i++) {
        pids[i] = child_make(i, fd_server, sin_len);    /* parent returns */
    }

    if(catch_signal(SIGINT, handleShutdown) == -1){
        printf("No se pudo mapear el manejador");
        exit(1);
    }

    for( ; ; ){
        pause();
    }


    return 0;

}





//
//
//
//
//
//
//
//
//
///* include child_main */
//void  child_main(int i, int listenfd, int addrlen) {
//    int				connfd;
//    void			processRequest(int);
//    socklen_t		clilen;
//    struct sockaddr	*cliaddr;
//
//    cliaddr = malloc(addrlen);
//
//    printf("child %ld starting\n", (long) getpid());
//    for ( ; ; ) {
//        clilen = addrlen;
//        connfd = accept(listenfd, cliaddr, &clilen);
//
////        web_child(connfd);		/* process the request */
//        processRequest(connfd);
//
////        puts("hola \n");
//
//        close(connfd);
//    }
//}
//
//
//pid_t child_make(int i, int listenfd, int addrlen) {
//    pid_t	pid;
//    void	child_main(int, int, int);
//
//    if ( (pid = fork()) > 0)
//        return(pid);		/* parent */
//
//    child_main(i, listenfd, addrlen);	/* never returns */
//}
//
//
//
//
//
//int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp) {
//
//    int				listenfd, n;
//    const int		on = 1;
//    struct addrinfo	hints, *res, *ressave;
//
//    bzero(&hints, sizeof(struct addrinfo));
//    hints.ai_flags = AI_PASSIVE;
//    hints.ai_family = AF_UNSPEC;
//    hints.ai_socktype = SOCK_STREAM;
//
//    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
////        err_quit("tcp_listen error for %s, %s: %s", host, serv, gai_strerror(n));
//        printf("tcp_listen error for %s, %s: %s", host, serv, gai_strerror(n));
//    }
//    ressave = res;
//
//    do {
//        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//        if (listenfd < 0)
//            continue;		/* error, try next one */
//
//        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
//        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
//            break;			/* success */
//
//        close(listenfd);	/* bind error, close and try next one */
//
//    } while ( (res = res->ai_next) != NULL);
//
//    if (res == NULL) {    /* errno from final socket() or bind() */
////        err_sys("tcp_listen error for %s, %s", host, serv);
//        printf("tcp_listen error for %s, %s", host, serv);
//    }
//
//    listen(listenfd, 1024);
//
//    if (addrlenp) {
//        *addrlenp = res->ai_addrlen;    /* return size of protocol address */
//    }
//
//    freeaddrinfo(ressave);
//
//    return(listenfd);
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//int main(int argc, char **argv) {
//
//    int listenfd, i;
//    socklen_t addrlen;
//    void sig_int(int);
//    pid_t child_make(int, int, int);
//
//    if (argc == 3) {
//        listenfd = tcp_listen(NULL, argv[1], &addrlen);
//    } else if (argc == 4) {
//        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
//    } else {
//        printf("usage: serv02 [ <host> ] <port#> <#children>");
//    }
//    nchildren = atoi(argv[argc-1]);
//    pids = calloc(nchildren, sizeof(pid_t));
//
//    for (i = 0; i < nchildren; i++) {
//        pids[i] = child_make(i, listenfd, addrlen);    /* parent returns */
//    }
//
////    Signal(SIGINT, sig_int);
//    catch_signal(SIGINT, handleShutdown);
//
//    for ( ; ; )
//        pause();	/* everything done by children */
//}
///* end serv02 */