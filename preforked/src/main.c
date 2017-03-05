#include <sys/wait.h>
#include "lib/common.h"
#include "lib/network.h"

void process_request(int fd) {

    struct http_request_data data = http_parse_request(fd);
    http_print_request_data(&data);

    write(fd, "mensaje recibido\n", 25);
}

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

void handleShutdown(int sig){

    printf("\nfinalizando... \n");

    if (errno != ECHILD) {
        printf("wait error");
    }

    printf("\nSOA-Server-Preforked: Bye! \n");
    exit(0);
}

int catch_signal(int sig, void (*handler)(int)){
    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    return sigaction(sig, &action, NULL);
}

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
    live_children = 0;
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

        //Check if terminated by signal
        if(WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            printf("Matado con señal=%d.\n", sig);

            //Restart child process, unless killed by SIGUSR1
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