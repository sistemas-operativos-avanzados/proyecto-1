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
#include <pthread.h>


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


char *okHeader = "HTTP/1.1 200 OK\r\nContent-Type: %s charset=UTF-8\r\nServer : SOA-Server-Secuencial\r\n\r\n";
char *notFoundHeader = "HTTP/1.1 400 Not Found\r\nContent-Type: text/html charset=UTF-8\r\nServer : SOA-Server-Secuencial\r\n\r\n";
char *notSupportedHeader = "HTTP/1.1 415 Unsupported Media Type\r\nnServer : SOA-Server-Secuencial\r\n\r\n";



static void	*doit(void *);

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

    if(fd_server){
        printf("\nFinalizando conexion \n");
        close(fd_server);
    }

    printf("\nSOA-Server-Secuencial: Bye! \n");
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

int main(int argc, char *argv[]){

    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_len, len;



    pthread_t tid;

    struct sockaddr *cliaddr;
    int *iptr;

    if(catch_signal(SIGINT, handleShutdown) == -1){
        printf("No se pudo mapear el manejador");
        exit(1);
    }

    fd_server = openListener();
    //TODO: Valor del puerto tiene que ser pasado por parametro
    bindToPort(fd_server, 8080);

    if(listen(fd_server, 10) == -1){ // una cola de 10 listeners
        printf("\n listen error \n");
        close(fd_server);
        exit(1);
    }

    cliaddr = malloc(sin_len);
    while(1) {
        len = sin_len;
        iptr = malloc(sizeof(int));
        *iptr = accept(fd_server, (struct sockaddr *) &cliaddr, &len);

//        int fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);
//        if (fd_client == -1) {
//            printf("\n**** Conexion fallida **** \n");
//            continue;
//        }

        pthread_create(&tid, NULL, &doit, iptr);


//        printf("\n==> Conexion iniciada\n");
//        memset(buf, 0, 2048); //     <==== aqui se va a dejar todo lo que viene en la peticion HTTP
//        memset(filePath, 0, 500); // <==== aqui se va a dejar la ruta al archivo
//        //TODO: valorar si cambiar este read por transmision bit a bit
//        //TODO: validar si la peticion es de tipo HTTP GET, sino descartarla
//        read(fd_client, buf, 2047);
////        printf("%s \n", buf);
//
//
//        //Creando la ruta hacia el archivo, esto deberia de ir en una funcion por aparte
//        strcpy(filePath, ROOT_FOLDER);
//        int i = 4, j = 0;
//        char urlPath[500];
//        memset(urlPath, 0, 500);
//        while(buf[i] != ' '){ // <=== iterar hasta encontrar el primer espacio en blanco, hasta alli llega el path
//            urlPath[j] = buf[i];
//            i++; j++;
//        }
//        strcat(filePath, urlPath);
//        printf("path = %s \n", filePath);
//
//        int fileResource = open(filePath, O_RDONLY);
//        if(fileResource != -1) {
//
//            char *mime = mimeType(strstr(filePath, "."));
//            char *header = successHeader(mime);
//            printf("HEADER = %s", header);
//
//            write(fd_client, header, strlen(header));
//
//            int length;
//            if ((length = getFileSize(fileResource)) == -1) {
//                printf("Error obtiendo tamanno de archivo\n");
//            }
//
//            size_t total_bytes_sent = 0;
//            ssize_t bytes_sent;
//            while (total_bytes_sent < length) {
//                if ((bytes_sent = sendfile(fd_client, fileResource, 0, length - total_bytes_sent)) <= 0) {
//                    printf("sendfile error\n");
//                    return -1;
//                }
//                total_bytes_sent += bytes_sent;
//            }
//            close(fileResource);
//
//
//        }else{
//            printf("Archivo no se encuentra \n");
//            write(fd_client, notFoundHeader, strlen(notFoundHeader));
//            write(fd_client, notFoundPage, strlen(notFoundPage));
//        }
//
//        printf("<== Finalizando conexion\n\n");
//        close(fd_client);
    }

    return 0;

}


static void *doit(void *arg){
    int fd_client = *((int *) arg);
    free(arg);
    pthread_self();
    puts("==> Nuevo Thread\n");

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
    while(buf[i] != ' '){ // <=== iterar hasta encontrar el primer espacio en blanco, hasta alli llega el path
        urlPath[j] = buf[i];
        i++; j++;
    }
    strcat(filePath, urlPath);
    printf("path = %s \n", filePath);


    int fileResource = open(filePath, O_RDONLY);
    printf("file = %d", fileResource);
    if(fileResource != -1) {

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


    }else{
        printf("Archivo no se encuentra \n");
        write(fd_client, notFoundHeader, strlen(notFoundHeader));
        write(fd_client, notFoundPage, strlen(notFoundPage));
    }

    puts("<== Finalizando conexion\n\n");

    close(fd_client);
    return (NULL);
}