/*
 * protocol.c
 *
 *  Created on: 8 sep. 2019
 *      Author: utnso
 */

#include "protocol.h"

int send_message(int socket, t_header head, void* content, size_t size){
	t_message* message = create_t_message(head,size,content);
	int res = send(socket, &(message->size) , sizeof(size_t), 0);

	if(res <0){
		//Hacer algo

	}else{
		void* buffer = malloc(message->size);
		memcpy(buffer,&message->head,sizeof(t_header));
		memcpy(buffer + sizeof(t_header),message->content,size);
		res = send(socket,buffer,message->size,0);
		if(res <0){
			//Hacer algo
		}
		free(buffer);
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
	if (res== -1 ){
		close(socket);
		free(message);
		return error_recv();
	}

	void* buffer = malloc(message->size);
	res = recv(socket,buffer,message->size,MSG_WAITALL);

	if( res == -1 ){
		close(socket);
		free(message);
		free(buffer);
		return error_recv();
	}

	if( res == 0 ){
		close(socket);
		free(message);
		free(buffer);
		return no_connection();
	}

	message->content = malloc(message->size - sizeof(t_header));
	memcpy(&message->head, buffer, sizeof(t_header));
	memcpy(message->content,buffer + sizeof(t_header),message->size - sizeof(t_header));
	message->size = message->size - sizeof(t_header);

	free(buffer);
	return message;
}


t_message* create_t_message(t_header head, size_t size, void* content){
	t_message* message = (t_message*)malloc(sizeof(t_message));
	message->head = head;
	message->content = malloc(size);
	message->size = size + sizeof(head);

	memset(message->content, 0, size);
	memcpy(message->content,content,size);

	return message;
}

t_message* no_connection(){
	return create_t_message(NO_CONNECTION,0,NULL);
}

t_message* error_recv(){
	return create_t_message(ERROR_RECV,0,NULL);
}
