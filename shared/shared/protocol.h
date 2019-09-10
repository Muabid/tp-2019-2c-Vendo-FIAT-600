/*
 * protocol.h
 *
 *  Created on: 8 sep. 2019
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef PROTOCOL_H_
#define PROTOCOL_H_


typedef enum{
	HOLA,
	ERROR_RECV,
	NO_CONNECTION
}t_header;

typedef struct{
	t_header head;
	size_t size;
	void* content;
}t_message;

t_message* create_t_message(t_header head, size_t size, void* content);

int send_message(int socket, t_header head, void* content, size_t size);

void free_t_message(t_message* message);

t_message* no_connection();
t_message* error_recv();
#endif /* PROTOCOL_H_ */
