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
#include "libmuse.h"

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
	HEAD,
	GET_ATTR,
	READ_LINK,
	CREATE,
	UNLINK,
	MKDIR,
	OPENDIR,
	RMDIR,
	READDIR,
	MKNODE,
	UTIME,
	READ,
	WRITE,
	TRUNCATE,
	RENAME,
	OK,
	FILE_NAME,
	DIR_NAME,
	MUSE_ALLOC,
	MUSE_FREE,
	MUSE_GET,
	MUSE_CPY,
	MUSE_MAP,
	MUSE_SYNC,
	MUSE_UNMAP,
	MUSE_INIT,
	MUSE_CLOSE,
	ERROR,
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


int muse_init(int id, char* ip, int puerto){
	if(!initialized){
		int sock = connect_to_server(ip, puerto, NULL);
		if(sock < 0){
			puts("Error al conectar al servidor");
			return -1;
		}
		socketMuse = sock;

	}
	return initialized;
}
//	int sock = connect_to_server(ip, puerto, NULL);
//	if(sock>0){
//		socketMuse = sock;
//		uint32_t _id = id;
//		initialized = send_message(socketMuse,MUSE_INIT,&_id,sizeof(uint32_t));
//		if(initialized <0){
//
//		}else{
//
//		}
//	}

void muse_close(){
	if(initialized){//si ya esta cerrado no hace nada
		return;
	}
	send_status(socketMuse,MUSE_CLOSE,1);
	close(socketMuse);
	initialized = false;
	free(id_muse);
	puts("F MUSE\n");
}

//int muse_alloc(char* id, uint32_t tamanio);
uint32_t muse_alloc(uint32_t tam){
	size_t sizeT = sizeof(tam) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void* aux = content;
	memcpy(aux,&tam,sizeof(tam));
	aux+=sizeof(tam);
	size_t len= strlen(id_muse);
	memcpy(aux,&len,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_ALLOC,content,sizeT);
	free(content);
	t_message* message = recv_message(socketMuse);
	int res;
	if(message->head == ERROR){
		//log
	}
	res = get_status(message);
	return res;
}

//int muse_free(char* id, uint32_t dir);
void muse_free(uint32_t dir){
	size_t sizeT = sizeof(dir) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void* aux = content;
	memcpy(aux,&dir,sizeof(dir));
	aux += sizeof(dir);
	memcpy(aux,strlen(id_muse),sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_FREE,content,sizeT);
	free(content);
	t_message* message = recv_message(socketMuse);
	int res;
	if(message->head == ERROR){
		puts("la re puta que te pario");
		raise(SIGSEGV);
	}
	if(message->content == -1){
		raise(SIGSEGV);
		//raise(5); ?
	}
	res = get_status(message);
	printf("Free realizado para %zu\n",dir);
}

//void* muse_get(char* id, uint32_t src, size_t n);
int muse_get(void* dst, uint32_t src, size_t n){
	size_t sizeT = sizeof(src) + sizeof(n) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void* aux = content;
	memcpy(aux,&src,sizeof(uint32_t));
	aux += sizeof(uint32_t);
	memcpy(aux,&n,sizeof(size_t));
	aux += sizeof(size_t);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_GET,content,sizeT);
	free(content);
	t_message* message = recv_message(socketMuse);
	int res;
	if(message->head == ERROR){
		return -1;
	}else {
		memcpy(dst,message->content,message->size);
		return 0;
	}
}

//void* muse_cpy(char* id, uint32_t dst, void* src, size_t n);
int muse_cpy(uint32_t dst, void* src, int n){
	size_t size = sizeof(uint32_t) + n + sizeof(int) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(size);
	void*aux = content;
	memcpy(aux,&dst,sizeof(uint32_t));
	aux+=sizeof(uint32_t);
	memcpy(aux,src,n);
	aux+=n;
	memcpy(aux,&n,sizeof(int));
	aux+=sizeof(int);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_CPY,content,size);
	t_message* message = recv_message(socketMuse);
	if(message->head == ERROR){
		raise(11);
	}else{
		//algo
	}
	return 0;

}

//int muse_map(char* id, char* path, uint32_t length, uint32_t flag);
uint32_t muse_map(char *path, size_t length, int flags){
	size_t sizeT = strlen(path) + sizeof(length) + sizeof(int) + sizeof(size_t)
			+ strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void*aux = content;
	size_t len= strlen(path);
	memcpy(aux,&len,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,path,len);
	aux+=strlen(path);
	memcpy(aux,&length,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,&flags,sizeof(int));
	aux+=sizeof(int);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_MAP,content,sizeT);
	t_message* message = recv_message(socketMuse);

	if(message->head == ERROR){
		return -1;
	}else{
		return *((uint32_t*)message->content);
	}
}

//int muse_sync(char* id, uint32_t addr, size_t len);
int muse_sync(uint32_t addr, size_t len){
	size_t sizeT = sizeof(uint32_t) + sizeof(size_t) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void*aux = content;
	memcpy(aux,&addr,sizeof(addr));
	aux+=sizeof(addr);
	memcpy(aux,&len,sizeof(len));
	aux+=sizeof(len);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_SYNC,content,sizeT);
	t_message* message = recv_message(socketMuse);
	if(message->head == ERROR){
		return -1;
	} else {
		// algo
	}
	return 0;
}

//int muse_unmap(char* id, uint32_t dir);
int muse_unmap(uint32_t dir){
	size_t sizeT = sizeof(uint32_t) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void*aux = content;
	memcpy(aux,&dir,sizeof(dir));
	aux+=sizeof(dir);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_SYNC,content,sizeT);
	t_message* message = recv_message(socketMuse);
	if(message->head == ERROR){
		return -1;
	} else {
		// algo
	}
	return 0;
}
