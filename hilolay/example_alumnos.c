#include <hilolay/alumnos.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdbool.h>

//INIT
char* ipServidor = "127.0.0.1";
int puertoServidor = 20000;
int conectadoAlServer = -1;
time_t start;
time_t end;
bool primeraEjecucion = true;

//DECLARACIONES FUNCIONES Y TYPEDEF
typedef enum{
	SUSE_INIT = 0,
	SUSE_CREATE = 1,
	SUSE_SCHEDULE_NEXT = 2,
	SUSE_WAIT = 3,
	SUSE_SIGNAL = 4,
	SUSE_JOIN = 5,
	SUSE_CLOSE = 6,
	TEST = 7,
	SUSE_CONTENT = 8,
	NO_CONNECTION = 100,
	ERROR_RECV = 101,
	HI_PLEASE_BE_MY_FRIEND = 102,
	ERROR_MESSAGE = 103
}t_header;

typedef struct{
	t_header head;
	size_t size;
	void* content;
}t_message;

int connect_to_server(char* host,int port, void*(*callback)());
int init_server(int port);
int create_socket();
int send_message(int socket, t_header head,const void* content, size_t size);
void free_t_message(t_message* message);
t_message* recv_message(int socket);
t_message* create_t_message(t_header head, size_t size,const void* content);
t_message* error(int res);
int send_status(int sock,t_header head, int status);
int get_status(t_message* message);

//BIBLIOTECA NET
#define MAX_CLIENTS 128
int connect_to_server(char* host,int port, void*(*callback)()) {
	int sock;

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host);
	server_addr.sin_port = htons(port);

	sock = create_socket();

	if(connect(sock,(struct sockaddr *)&server_addr, sizeof(server_addr))< 0){
		perror("ERROR CONECTAR SERVIDOR");
		return -errno;
	}

	if(callback != NULL)
		callback();

	return sock;
}
int init_server(int port){
	int  socket, val = 1;
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr =INADDR_ANY;
	servaddr.sin_port = htons(port);

	socket = create_socket();
	if (socket < 0) {
		char* error = strerror(errno);
		perror(error);
		free(error);
		return EXIT_FAILURE;
	}

	setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if (bind(socket,(struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
		return EXIT_FAILURE;
	}

	if (listen(socket, MAX_CLIENTS)< 0) {
		return EXIT_FAILURE;
	}

	return socket;

}
int create_socket(){
	return socket(AF_INET, SOCK_STREAM, 0);
}

//BIBLIOTECA PROTOCOL

int send_message(int socket, t_header head,const void* content, size_t size){
	t_message* message = create_t_message(head,size,content);
	int res = send(socket, &(message->size) , sizeof(size_t), 0);

	if(res >= 0){
		void* buffer = malloc(message->size);
		memcpy(buffer,&message->head,sizeof(t_header));
		memcpy(buffer + sizeof(t_header),message->content,size);
		res = send(socket,buffer,message->size,0);
		if(res <0){
			perror("ERROR ENVIANDO MENSAJE");
			res = -errno;
		}
		free(buffer);
	}else{
		perror("ERROR ENVIANDO MENSAJE");
		res = -errno;
	}
	free_t_message(message);

	return res;

}
void free_t_message(t_message* message){
	if(message != NULL){
		if(message->content != NULL){
			free(message->content);
		}
		free(message);
	}
}
t_message* recv_message(int socket){
	t_message * message = malloc(sizeof(t_message));

	int res = recv(socket,&message->size,sizeof(size_t),MSG_WAITALL);
	if (res <= 0 ){
		close(socket);
		free(message);
		return error(res);
	}

	void* buffer = malloc(message->size);
	res = recv(socket,buffer,message->size,MSG_WAITALL);


	if(res <= 0){
		close(socket);
		free(message);
		free(buffer);
		return error(res);
	}

	message->content = calloc(message->size - sizeof(t_header)+1,1);
	memcpy(&message->head, buffer, sizeof(t_header));
	memcpy(message->content,buffer + sizeof(t_header),message->size - sizeof(t_header));
	message->size = message->size - sizeof(t_header);

	free(buffer);
	return message;
}
t_message* create_t_message(t_header head, size_t size,const void* content){
	t_message* message = (t_message*)malloc(sizeof(t_message));
	message->head = head;
	message->content = malloc(size);
	message->size = size + sizeof(head);

	memset(message->content, 0, size);
	memcpy(message->content,content,size);

	return message;
}
t_message* error(int res){
	t_header header = res == 0? NO_CONNECTION : ERROR_RECV;
	int32_t err = -errno;
	return create_t_message(header,sizeof(err),&err);
}
int send_status(int sock,t_header head, int status){
	return send_message(sock,status,&status,sizeof(int32_t));
}
int get_status(t_message* message){
	return *((int*)message->content);
}


/* Lib implementation: It'll only schedule the last thread that was created */
int max_tid = 0;

int suse_create(int tid){
	send_message(conectadoAlServer, SUSE_CREATE, &tid, sizeof(int));
	return 0;
}

int suse_schedule_next(void) {
	start = (int) clock();
	end = (int) clock();

	int tiempo;

	if (primeraEjecucion) {
		primeraEjecucion = false;
		tiempo = 0;
	} else {
		time(&end);
		tiempo = (int) (end - start);

	}

	if (conectadoAlServer == -1) {
		printf("No se ha iniciado el servidor");
		return -1;
	}
	send_message(conectadoAlServer, SUSE_SCHEDULE_NEXT, &tiempo, sizeof(int));
	t_message* bufferLoco = recv_message(conectadoAlServer);
	int numeroDeProceso = *(int*) bufferLoco->content;
	free_t_message(bufferLoco);
	return numeroDeProceso;
}

int suse_join(int tid){
	return 0;
}

int suse_close(int tid){
	send_message(conectadoAlServer, SUSE_CLOSE, &tid, sizeof(int));
	return 0;
}

int suse_wait(int tid, char *sem_name){
	send_message(conectadoAlServer, SUSE_WAIT, &tid, sizeof(int));
	send_message(conectadoAlServer, SUSE_CONTENT, sem_name, strlen(sem_name) + 1);
	return 0;
}

int suse_signal(int tid, char *sem_name){
	send_message(conectadoAlServer, SUSE_SIGNAL, &tid, sizeof(int));
	send_message(conectadoAlServer, SUSE_CONTENT, sem_name, strlen(sem_name) + 1);
	return 0;
}

static struct hilolay_operations hiloops = {
		.suse_create = &suse_create,
		.suse_schedule_next = &suse_schedule_next,
		.suse_join = &suse_join,
		.suse_close = &suse_close,
		.suse_wait = &suse_wait,
		.suse_signal = &suse_signal
};

void hilolay_init(void){
	conectadoAlServer = connect_to_server(ipServidor, puertoServidor, NULL);
	printf("Se conectó al server\n");
	start = time(NULL);
	init_internal(&hiloops);
}
