#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>


/*
Servidor Threaded HTTP:
==========================
 - Soporta únicamente solicitudes HTTP GET
 - Se crea un thread para atender cada solicitud
 - El directorio web-resources actúa como "raiz" para servir los localizar y servir los archivos que se le solicitan
 - Cada vez que se genera una conexión, se desplega en el stdout información sobre la misma

Desarrollado por:
=================
- Raquel Elizondo Barrios
- Carlos Martin Flores Gonzalez
- Jose Daniel Salazar Vargas
- Oscar Rodríguez Arroyo
- Nelson Mendez Montero

 */


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


char *okHeader = "HTTP/1.1 200 OK\r\nContent-Type: %s charset=UTF-8\r\nServer : SOA-Server-Threaded\r\n\r\n";
char *notFoundHeader = "HTTP/1.1 400 Not Found\r\nContent-Type: text/html charset=UTF-8\r\nServer : SOA-Server-Threaded\r\n\r\n";
char *notSupportedHeader = "HTTP/1.1 415 Unsupported Media Type\r\nnServer : SOA-Server-Threaded\r\n\r\n";



static void	*doit(void *);


#define printable(ch) (isprint((unsigned char) ch) ? ch : '#')

/* ==========================================================================================
 * Funciones utilitarias
 * ========================================================================================= */


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


/*
 * Esta funcion actua como manejador de la "interrupcion" del Crtl+C
 */
void handleShutdown(int sig){

    if(fd_server){
        printf("\nFinalizando conexion \n");
        close(fd_server);
    }

    printf("\nSOA-Server-Threaded: Bye! \n");
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

static void usageError(char *progName, char *msg, int opt) {
    if (msg != NULL && opt != 0) {
        fprintf(stderr, "%s (-%c)\n", msg, printable(opt));
    }
    fprintf(stderr, "Uso: %s [-p puerto]\n", progName);
    exit(EXIT_FAILURE);
}


/* ==========================================================================================
 * Sockets: Abrir y asociar un puerto
 * ========================================================================================= */


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


// MAIN -----------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]){

    socklen_t sin_len, len;
    pthread_t tid;
    struct sockaddr *cliaddr;
    int *iptr;
    int opt, port;


    /* Validación del parametro de entrada -p */

    if(argc == 1){
        usageError(argv[0], "Falta parametro", 'p');
    }
    while((opt = getopt(argc, argv, "-p:")) != EOF) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case ':':
                usageError(argv[0], "Falta argumento", optopt);
            case '?':
                usageError(argv[0], "Opcion invalida", optopt);
            default:
                usageError(argv[0], "Falta argumento", optopt);
        }
    }


    if(catch_signal(SIGINT, handleShutdown) == -1){
        printf("No se pudo mapear el manejador");
        exit(1);
    }

    fd_server = openListener();
    //TODO: Valor del puerto tiene que ser pasado por parametro
    bindToPort(fd_server, port);

    if(listen(fd_server, 1024) == -1){
        printf("\n listen error \n");
        close(fd_server);
        exit(1);
    }

    printf("\nSOA-Server-Threaded: Iniciando en puerto: %d \n", port);

    cliaddr = malloc(sin_len);
    while(1) {
        len = sin_len;
        iptr = malloc(sizeof(int));
        *iptr = accept(fd_server, (struct sockaddr *) &cliaddr, &len);

        pthread_create(&tid, NULL, &doit, iptr);
    }

    return 0;

}


/* ==========================================================================================
 * Procesar una solicitud (request)
 * ========================================================================================= */
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
//    printf("file = %d", fileResource);
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