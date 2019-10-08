/*
 ============================================================================
 Name        : SAC-server.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <shared/net.h>
#include <shared/protocol.h>
#include "sac_config.h"
#include "nodes.h"
#include "sac.h"
#include "operations.h"
#include <shared/utils.h>
int main(int argc, const char* argv[]) {

	void* bitarray = malloc(1);
	bitmap = bitarray_create_with_mode(bitarray, 1,
			LSB_FIRST);

//	for (int i=0; i < 8; i++) {
//		if(i != 5)
//			bitarray_set_bit(bitmap, i);
//	}
	for(int i = 0; i<10; i++){
		int x = search_and_test_first_free_block();
		printf("%i",free_blocks());
		printf("%i",x);
	}

//	nodes_table[0] = create_GFile(2,"hola",0,5,"12345678","12345678");
//	sac_config = malloc(sizeof(t_sac_config));
//	sac_config->listen_port=8080;
//	init_sac_server();
//	char* name = get_name("/hola/");
//	char* x = get_directory("/hola");
//	printf("%s %s", name,x);
}

void create_file_system() {
	FILE* pFile;
	pFile = fopen("test", "wb");
	GHeader header = create_sac_header("SAC", 1, 1, 2);
	fwrite(&header, sizeof(header), HEADER_BLOCKS, pFile);
	fseek(pFile, sizeof(header), SEEK_SET);
	char* bitarray = malloc(BLOCKS_BITMAP);
	t_bitarray* bi = bitarray_create_with_mode(bitarray, BLOCKS_BITMAP,
			LSB_FIRST);
	for (int i = 0; i < HEADER_BLOCKS + BLOCKS_BITMAP + BLOCKS_NODE; i++) {
		bitarray_set_bit(bi, i);
	}

	t_block block;
	memcpy(block.data,bi->bitarray,BLOCKS_BITMAP);
	fwrite(&block, BLOCK_SIZE,1, pFile);
	fseek(pFile,BLOCK_SIZE, SEEK_SET);
	GFile nodes[BLOCKS_NODE];
	nodes[0] = create_GFile(2,"/",0,1024,"19092019","19092019");
	nodes[1] = create_GFile(2,"HOLA",0,1024,"19092019","19092019");
	nodes[2] = create_GFile(2,"XD",1,1024,"19092019","19092019");
	fwrite(&nodes, sizeof(GFile), BLOCKS_NODE, pFile);
	fseek(pFile, sizeof(GFile) * BLOCKS_NODE, SEEK_SET);
	t_block basura[BLOCKS_DATA];
	fwrite(&basura, sizeof(t_block), BLOCKS_DATA, pFile);
	fclose(pFile);
}

void listen_sac_cli(void* socket){
	int sac_socket = (int)(socket);
	while(1){
	t_message* message = recv_message(sac_socket);
		switch(message->head){
			case HI_PLEASE_BE_MY_FRIEND:
				send_message(sac_socket,HI_PLEASE_BE_MY_FRIEND,"HIIIIIIIII",strlen("HIIIIIIIII"));
				break;
			case GET_ATTR:
				getattr(sac_socket,message->content);
				break;
		}
		free(message->content);
		free(message);
	}
}

void init_sac_server(){
	listener_socket = init_server(sac_config->listen_port);
	struct sockaddr sac_cli;
	int sac_socket;
	socklen_t len = sizeof(sac_cli);

	while((sac_socket = accept(listener_socket,&sac_cli, &len)) >0){
		printf("%s","Nueva conexión");
		pthread_t sac_cli_thread;
		pthread_create(&sac_cli_thread,NULL,(void*) listen_sac_cli,(void*)(sac_socket));
	}
}

