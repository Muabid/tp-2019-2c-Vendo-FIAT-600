/*
 * net.h
 *
 *  Created on: 1 sep. 2019
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#ifndef NET_H_
#define NET_H_
#define MAX_CLIENTS 128


int connect_to_server(char* host, int port, void*(*callback)());
int create_socket();
int init_server(int port);
#endif /* NET_H_ */
