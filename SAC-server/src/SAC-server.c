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


int main(int argc, const char* argv[]) {
	//Esto es para crear el filesystem y poder probar, después vuela a la mierda
	if(argc == 2 && 1 ==atoi(argv[1])){
			create_file_system();
	} else {
		sac_load_config((char*)argv[1]);
		int fd = open("test",O_RDWR,0);

		GHeader* mount = (GHeader*)mmap(NULL,FILESYSTEM_SIZE , PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
		GHeader h = *mount;

		printf("identifier: %s | version: %i \n",h.identifier,h.version);
		t_block* block = (t_block *)(mount + HEADER_BLOCKS);

		int bitmap_size_in_bits = BLOCKS_TOTAL;

		t_bitarray * bitmap = bitarray_create_with_mode((char*)block,BLOCKS_BITMAP,LSB_FIRST);

		for (int i = 0; i < bitmap_size_in_bits; i++){
			printf("%i",bitarray_test_bit(bitmap, i));
		}

		bitarray_destroy(bitmap);

//		nodes_table =(GFile*)(mount + HEADER_BLOCKS + BLOCKS_BITMAP);

		close(fd);
	}

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
	for (int i = 0; i < BLOCKS_TOTAL; i++) {
		printf("%i", bitarray_test_bit(bi, i));
	}
	fwrite(bi->bitarray, 1, BLOCKS_BITMAP, pFile);
	fseek(pFile, BLOCKS_BITMAP * BLOCK_SIZE, SEEK_SET);
	GFile nodes[BLOCKS_NODE];
	fwrite(&nodes, sizeof(GFile), BLOCKS_NODE, pFile);
	fseek(pFile, sizeof(GFile) * BLOCKS_NODE, SEEK_SET);
	t_block basura[BLOCKS_DATA];
	fwrite(&basura, sizeof(t_block), BLOCKS_DATA, pFile);
	fclose(pFile);
}

void listen_sac_cli(void* socket){
	int sac_socket = (int)(socket);
	t_message* message = recv_message(sac_socket);
	while(1){
		switch(message->head){
			case HI_PLEASE_BE_MY_FRIEND:
				send_message(sac_socket,HI_PLEASE_BE_MY_FRIEND,"HI",2);
		}
	}
}

void init_sac_server(){
	listener_socket = init_server(sac_config->listen_port);
	struct sockaddr sac_cli;
	int sac_socket;
	socklen_t len = sizeof(sac_cli);

	while((sac_socket = accept(listener_socket,&sac_cli, &len)) >0){
		pthread_t sac_cli_thread;
		pthread_create(&sac_cli_thread,NULL,(void*) listen_sac_cli,(void*)(sac_socket));
	}
}

