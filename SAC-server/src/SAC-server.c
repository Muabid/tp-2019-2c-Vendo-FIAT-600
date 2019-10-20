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
	create_file_system();
	const char* path = argv[1];
	sac_load_config(path);
	int size_file_system = fsize(sac_config->file_system_path);
	int fd;
	if ((fd = open(sac_config->file_system_path, O_RDWR, 0)) == -1)
		return -1;

	GHeader* file_system = (GHeader*) mmap(NULL, size_file_system , PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
	gHeader = file_system[0];
	bitmap =  bitarray_create_with_mode((char*)&file_system[HEADER_BLOCKS],BLOCKS_BITMAP * BLOCK_SIZE,LSB_FIRST);
	nodes_table = (GFile*) &file_system[HEADER_BLOCKS + BLOCKS_BITMAP];
	blocks_data = (t_block*) &file_system[HEADER_BLOCKS + BLOCKS_BITMAP + BLOCKS_NODE];
	int x = free_blocks();
	init_sac_server();
}

void create_file_system() {
	FILE* pFile;
	pFile = fopen("test", "wb");
	gHeader = create_sac_header("SAC", 1, 1, 2);
	fwrite(&gHeader, sizeof(gHeader), HEADER_BLOCKS, pFile);
	fseek(pFile, sizeof(gHeader), SEEK_SET);
	char* bitarray = malloc(BLOCKS_BITMAP * BLOCK_SIZE);
	t_bitarray* bi = bitarray_create_with_mode(bitarray, BLOCKS_BITMAP * BLOCK_SIZE,
			LSB_FIRST);
	for (int i = 0; i < BLOCKS_BITMAP * BLOCK_SIZE; i++) {
		bitarray_set_bit(bi, i);
	}

	t_block block;
	memcpy(block.data,bi->bitarray,BLOCK_SIZE);
	fwrite(bi->bitarray, BLOCK_SIZE,2, pFile);
//	fseek(pFile,BLOCK_SIZE, SEEK_SET);
//	memcpy(block.data,bi->bitarray + BLOCK_SIZE,BLOCK_SIZE);
//	fwrite(&block, BLOCK_SIZE,1, pFile);
//	fseek(pFile,BLOCK_SIZE, SEEK_SET);
//	GFile nodes[BLOCKS_NODE];
//	nodes[0] = create_GFile(2,"/",0,1024,"19092019","19092019");
//	nodes[1] = create_GFile(2,"HOLA",0,1024,"19092019","19092019");
//	nodes[2] = create_GFile(2,"XD",1,1024,"19092019","19092019");
//	fwrite(&nodes, sizeof(GFile), BLOCKS_NODE, pFile);
//	fseek(pFile, sizeof(GFile) * BLOCKS_NODE, SEEK_SET);
//	t_block basura[BLOCKS_DATA];
//	fwrite(&basura, sizeof(t_block), BLOCKS_DATA, pFile);
	fclose(pFile);
}

void* listen_sac_cli(void* socket){
	int sac_socket = (int)(socket);
	while(1){
	t_message* message = recv_message(sac_socket);
		switch(message->head){
			case HI_PLEASE_BE_MY_FRIEND:
				send_message(sac_socket,HI_PLEASE_BE_MY_FRIEND,&sac_socket,sizeof(sac_socket));
				break;
			case GET_ATTR:
				sac_getattr(sac_socket,message->content);
				break;
			case NO_CONNECTION:
				return -1;
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
		pthread_create(&sac_cli_thread,NULL,listen_sac_cli,(void*)(sac_socket));
	}
}

