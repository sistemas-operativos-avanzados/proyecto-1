#include <sys/wait.h>
#include "lib/common.h"
#include "lib/network.h"
#include <fcntl.h>
#include <sys/stat.h>

/*
Servidor Preforked HTTP:
==========================
 - Soporta únicamente solicitudes HTTP GET
 - Cada vez que llega una solicitud, uno de los procesos, creados al inicio, lo atiende si se encuentra desocupado
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

char notFoundPage[] = "<html><head><title>404</head></title><body><p>404: El recurso solicitado no se encontró</p></body></html>";

char* ROOT_FOLDER = "web-resources";

char *okHeader = "HTTP/1.1 200 OK\r\nContent-Type: %s charset=UTF-8\r\nServer : SOA-Server-Preforked\r\n\r\n";
char *notFoundHeader = "HTTP/1.1 400 Not Found\r\nContent-Type: text/html charset=UTF-8\r\nServer : SOA-Server-Preforked\r\n\r\n";
char *notSupportedHeader = "HTTP/1.1 415 Unsupported Media Type\r\nnServer : SOA-Server-Preforked\r\n\r\n";

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

void process_request(int fd_client) {
    char buf[2048];
    char filePath[500];

    puts("==> Conexion iniciada\n");
    memset(buf, 0, 2048); //     <==== aqui se va a dejar todo lo que viene en la peticion HTTP
    memset(filePath, 0, 500); // <==== aqui se va a dejar la ruta al archivo

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

//===========================================================================
// Función que se encarga de crear procesos. Es llamada dentro del main. 
// Se puede llamar cuando se crean procesos al inicio o cuando los procesos 
// son terminados con señal distinta de SIGUSR1
//
// Acá también se procesa la petición hecha desde un browser o utilizando
// herramientas cómo cURL
//===========================================================================
pid_t fork_child(int listen_fd)
{
    struct sockaddr addr;
    socklen_t addr_len = sizeof(addr);

    pid_t child_pid = fork();
    if(child_pid < 0) {
        perror("fallo de fork!");
        exit(1);
    }

    if(child_pid == 0) {

        printf("Hijo iniciado (PID %d)\n", getpid());

        while (1) {

            int accept_fd = Accept(listen_fd, &addr, &addr_len);
            printf("Recibido en (PID %d)\n", getpid());

            process_request(accept_fd);
            close(accept_fd);
        }
    } else {
        return child_pid;
    }
}

//===========================================================================
// Función que se encarga de manejar la "interrupción elegante"
//===========================================================================
void handleShutdown(int sig){

    printf("\nfinalizando... \n");

    if (errno != ECHILD) {
        printf("wait error");
    }

    printf("\nSOA-Server-Preforked: Bye! \n");
    exit(0);
}

//===========================================================================
// Función que se encarga de capturar la señal para la "interrupción elegante"
//===========================================================================
int catch_signal(int sig, void (*handler)(int)){
    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    return sigaction(sig, &action, NULL);
}

//===========================================================================
// Función principal. Por parámetro se reciben el puerto y el número de 
// procesos a crear. 
// Ejemplo de llamada: preforked 8080 5
// Lo anterior quiere decir que se está creando un servidor de tipo 
// preforked con 5 procesos escuchando el puerto 8080. 
//
// 
//===========================================================================
int main(int argc, char **argv) {

    if (argc < 3) {
        fprintf(stderr, "Uso: %s <puerto> <numero de procesos> \n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int port = atoi(argv[1]);
    int listen_fd = Open_listenfd(port);
    printf("%s escuchando en %d \n", argv[0], port);

    int processes = atoi(argv[2]);

    //Now fork from here. Each process will 'accept'.
    int c = 0;
    int live_children = 0;
    while(c++ < processes) {
        pid_t child_pid = fork_child(listen_fd);
        if(child_pid != 0) {
            //Parent is counting children forked
            live_children++;
        }
    }

    while(live_children) {

        if(catch_signal(SIGINT, handleShutdown) == -1){
            printf("No se pudo mapear el manejador");
            exit(1);
        }

        int status = 0;
        pid_t cp = wait(&status);

        printf("Hijo (PID %d) muerto. ", cp);

        if(WIFEXITED(status))
            printf("Terminación normal con estado de salida=%d\n", WEXITSTATUS(status));

        //Revisa si alguna señal lo ha terminado
        if(WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            printf("Matado con señal=%d.\n", sig);

            //Reinicia un proceso, a menos de ser terminado con SIGUSR1
            if(sig != SIGUSR1) {
                printf("Proceso hijo muerto. Reapareciendo .. ");

                pid_t child_pid = fork_child(listen_fd);
                if(child_pid != 0) {
                    live_children++;
                }

                printf("Hecho\n");
            } else {
                printf("Hijo matado con señal SIGUSR1. No va a reaparecer.\n");
            }
        }

        if(WIFSTOPPED(status))
            printf("Detenido con señal=%d\n", WSTOPSIG(status));

        if(WIFCONTINUED(status))
            printf("Continuando\n");

        live_children--;
    }

    Close(listen_fd);
    printf("Adiós!\n");

    return EXIT_SUCCESS;
}
