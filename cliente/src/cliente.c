#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <time.h>


char *host, *filename, *portno;
int d_sock, ncycles = 0;

char *ok_response = "HTTP/1.1 200 OK";
char *not_found_response = "HTTP/1.1 400 Not Found";
char *not_supported_response = "HTTP/1.1 415 Unsupported Media Type";

double first_elapsed_time = -1;
int request_count = 0;
double request_elapsed_time_sum = 0;
int ok_code_count = 0;
int not_found_code_count = 0;
int not_supported_code_count = 0;
int other_code_count = 0;
int first_response_header_size = -1;
int first_response_data_size = -1;
pthread_mutex_t lock;

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
        error("No se puede resolver la dirección");
    }

    int d_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (d_sock == -1) {
        error("No se puede abrir el socket");
    }
    int c = connect(d_sock, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    if (c == -1) {
        error("No se puede conectar al socket");
    }
    return d_sock;
}


//"snapshot" del tiempo actual en nanosegundos
long get_current_time_ns(){

    long long s;
    long ns;
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);

    s = spec.tv_sec * 1LL;
    ns = spec.tv_nsec;

    long current_time_ns = (s * 1.0e9) + ns;

    return current_time_ns;

}

//calcula el tiempo (en milisegundos) transcurrido entre 2 "snapshots" de tiempo (en nanosegundos)
double get_time_diff_ms(long start, long end){

    long diff = end - start;
    double diff_ms = diff / 1.0e6;
	
    return diff_ms;
}



static void *make_request() {

	unsigned long t_id = syscall( __NR_gettid );

  printf("Hilo id %lu fue creado exitosamente\n", t_id);
  int cycles = 1;

//  printf("cycles %i\n", cycles);
//  printf("ncycles %i\n", ncycles);

  while(cycles <= ncycles){

	printf("\n********** Ejecutando ciclo %i de %i en hilo: %lu **********\n\n", cycles, ncycles, t_id);

	//se empieza a llevar la cuenta del tiempo
	long start_time = get_current_time_ns();

/*
Se envia el request
*/	


       char buf[1023];
       sprintf(buf, "GET /%s HTTP/1.1\r\nHost:%s\r\n\r\n", filename, host);
    
	d_sock = open_socket(host, portno);

       if((send(d_sock, buf, strlen(buf), 0)) == 0)
         error("\nError al enviar solicitud\n");


/*
Se lee la respuesta del server
*/

       char rec[7340032]; //7MB: tamanno del buffer para almacenar la respuesta, si es mayor a 7MB se tira un Segmentation Fault
	char *p = rec;

	int len = 1;
	int total_size = 0;
	int chunk_counter = 0;

	while(len > 0){
		len = recv(d_sock, p, 1024, MSG_WAITALL);
		p += len;
		if(len > 0){
			total_size += len;
			chunk_counter++;
			printf("(%lu: %i/%i) Tamaño del chunck leído #%i: %i\n", t_id, cycles, ncycles, chunk_counter, len);
		}
	}

	close(d_sock);

	//se termina de llevar la cuenta del tiempo
	long end_time = get_current_time_ns();	

	printf("(%lu: %i/%i) Total de bytes leídos: %i\n", t_id, cycles, ncycles, total_size);


/*
Se procesa la respuesta, especificamente para "separar" el header response de la data
*/


	int header_bytes_counter = 0;
	int response_code_size = 0;

	for(int i=0; i<total_size; i++){
		char c = rec[i];		
		header_bytes_counter++;
		if(c == '\n'){
			if(response_code_size == 0){
				response_code_size = header_bytes_counter -2; //removing '\r' and '\n' chars from the count
			}
			if(rec[i+1] == '\r' && rec[i+2] == '\n'){
				header_bytes_counter += 2;
				break;
			}
		}
	}


/*
Basado en el procesamiento anterior extraigo unicamente el response code
*/

	char response_code[response_code_size];
	for(int i=0; i<response_code_size; i++){
		response_code[i] = rec[i];
	}	

	int data_size = total_size - header_bytes_counter;

	printf("(%lu: %i/%i) Código de Respuesta: %s\n", t_id, cycles, ncycles, response_code);
	printf("(%lu: %i/%i) Tamaño del encabezado de Respuesta: %i\n", t_id, cycles, ncycles, header_bytes_counter);
	printf("(%lu: %i/%i) Tamaño de los datos de Respuesta: %i\n", t_id, cycles, ncycles, data_size);

	//se calcula el tiempo que tardo el request/response
	double elapsed_time = get_time_diff_ms(start_time, end_time);

	printf("(%lu: %i/%i) Tiempo transcurrido: %4.3f ms.\n", t_id, cycles, ncycles, elapsed_time);

/*
Llenando reportes
*/

	pthread_mutex_lock(&lock);

	
	if(first_elapsed_time == -1){
		first_elapsed_time = elapsed_time;
	}

	if(first_response_header_size == -1){
		first_response_header_size = header_bytes_counter;
	}

	if(first_response_data_size == -1){
		first_response_data_size = data_size;
	}

	request_count++;
	request_elapsed_time_sum += elapsed_time;

	if (strcmp(response_code, ok_response) == 0) {
            ok_code_count++;
        }else if (strcmp(response_code, not_found_response) == 0) {
            not_found_code_count++;
        }else if (strcmp(response_code, not_supported_response) == 0) {
            not_supported_code_count++;
        }else{
	    other_code_count++;
	}

	pthread_mutex_unlock(&lock);

  
	cycles++;
  }
  
  return (NULL);
}




int main(int argc, char* argv[]) {
    
    if(argc != 6){
        //ei. ./client proveinca.com 80 fs_files/user_img/Composicion%20PEX-AL-PEX.jpg 2 120

        printf("\n Uso: %s <maquina> <puerto> <archivo> <N-threads> <N-ciclos>", argv[0]);
        return 1;
    }
    int nthreads = 0;
    
    host = argv[1];
    portno = argv[2];
    filename = argv[3];
    nthreads = atoi(argv[4]);
    ncycles = atoi(argv[5]);
    int i = 0, y = 1;
    pthread_t tid;
    pthread_t ids[nthreads];

	
    if (pthread_mutex_init(&lock, NULL) != 0){
        error("\nFallo al iniciar el Mutex.\n");
    }


    while(i < nthreads)
    {
        //d_sock = open_socket(host, portno);
	int err = pthread_create(&(tid), NULL, &make_request, NULL);
        //printf("Thread id %lu fue creado correctamente\n", pthread_self());
	if (err != 0){
	    error("\nNo se puede crear el hilo.\n");
	}
	 else{
	    ids[i] = tid;
	}
//         pthread_join(tid, NULL);
     i++;
    }

    i = 0;
    while(i < nthreads){
      pthread_join(ids[i], NULL);
      i++;
    }

    pthread_mutex_destroy(&lock);

/*
Imprimiendo el reporte
*/

printf("\n\n\n++++++++++ REPORTE ++++++++++\n\n");
	
printf("Cantidad total de solicitudes: %i\n", request_count);
printf("Tiempo transcurrido promedio por solicitud: %4.3f ms.\n", request_elapsed_time_sum/request_count);
printf("Tiempo transcurrido para la primera solicitud: %4.3f ms.\n", first_elapsed_time);
printf("Tamaño total de la respuesta para la primera solicitud: %i\n", first_response_header_size + first_response_data_size);
printf("\tTamaño del encabezado: %i\n", first_response_header_size);
printf("\tTamaño de los datos: %i\n", first_response_data_size);
printf("Conteo de códigos de respuesta:\n");
printf("\t200 OK: %i veces.\n", ok_code_count);
printf("\t400 Not Found: %i veces.\n", not_found_code_count);
printf("\t415 Unsupported Media Type: %i veces.\n", not_supported_code_count);
printf("\tOtro: %i veces.\n", other_code_count);
	
printf("\n\nSOA-Cliente: Bye!\n");
    return 0;
}
