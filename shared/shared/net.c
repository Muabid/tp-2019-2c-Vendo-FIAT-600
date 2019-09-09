/*
 * net.c
 *
 *  Created on: 1 sep. 2019
 *      Author: utnso
 */
#include "net.h"

int connect_to_server(char* host,int port, void*(*callback)()) {
	int sock;

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host);
	server_addr.sin_port = htons(port);

	sock = create_socket();

	if(connect(sock,(struct sockaddr *)&server_addr, sizeof(server_addr))< 0){
		perror("ERROR CONECTAR SERVIDOR");
		exit(EXIT_FAILURE);
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
		perror("socket");
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
