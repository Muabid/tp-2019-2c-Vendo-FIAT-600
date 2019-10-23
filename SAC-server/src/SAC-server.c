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
#include <commons/log.h>
#include <signal.h>
void init_semaphores(){
	pthread_mutex_init(&bitarray_mutex,NULL);
}

void sig_term(int sig){
	pthread_mutex_destroy(&bitarray_mutex);
	fdatasync(file_system_descriptor);
	munmap(file_system, size_file_system);
	close(file_system_descriptor);
	exit(0);
}

int main(int argc, const char* argv[]) {
	log = log_create("./log","SERVER",true,LOG_LEVEL_INFO);
//	create_file_system();
	signal(SIGTERM, sig_term);

	const char* path = argv[1];
	sac_load_config(path);
	size_file_system = fsize(sac_config->file_system_path);
	log_info(log,"Filesystem Size: %i",size_file_system);

	if ((file_system_descriptor = open(sac_config->file_system_path, O_RDWR, 0)) == -1)
		return -1;

	file_system = (GHeader*) mmap(NULL, size_file_system +1 , PROT_WRITE | PROT_READ | PROT_EXEC,
			MAP_SHARED,file_system_descriptor, 0);
	gHeader = file_system[0];

	log_info(log,"Identifier: %s | Version: %i | Init Block: %i | Bitmap Size: %i",
			gHeader.identifier,gHeader.version, gHeader.init_block,gHeader.bit_map_size);

	char* bitmap_content = &file_system[HEADER_BLOCKS];
	bitmap =  bitarray_create_with_mode(bitmap_content,BLOCKS_BITMAP * BLOCK_SIZE,LSB_FIRST);

	log_info(log,"Free blocks %i",free_blocks());
	int i =search_and_test_first_free_block();
	log_info(log,"Block Tested %i",i);
	fdatasync(file_system_descriptor);
	munmap(file_system, size_file_system);
	close(file_system_descriptor);
//	init_sac_server();
}

void create_file_system() {
	FILE* pFile = fopen("test", "wb");

	GHeader* header = calloc(sizeof(GHeader),1);
	header->bit_map_size = 2;
	memcpy(header->identifier,"SAC",3);
	header->init_block = 1;
	memset(header->padding,0,4080);
	header->version = 1;
	size_t bitmap_size_b =  header->bit_map_size * BLOCK_SIZE;
	char* bitarray = calloc(bitmap_size_b,1);
	t_bitarray* bi = bitarray_create_with_mode(bitarray, header->bit_map_size * BLOCK_SIZE,
			LSB_FIRST);
//	for (int i = 0; i < header->bit_map_size * BLOCK_SIZE * 8; i++) {
//		bitarray_set_bit(bi, i);
//	}

	fwrite(header, sizeof(GHeader), HEADER_BLOCKS, pFile);

	fwrite(bi->bitarray, BLOCK_SIZE * 2,1, pFile);
	free(bitarray);
	bitarray_destroy(bi);
	free(header);
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
				log_info(log,"CLIENTE DESCONECTADO");
				free_t_message(message);
				return NULL;
				break;
			case ERROR_RECV:
				free_t_message(message);
				log_info(log,"ERROR COMUNIACIÓN");
				return NULL;
		}
		free(message->content);
		free(message);
	}
}

void init_sac_server(){
	listener_socket = init_server(8080);
	struct sockaddr sac_cli;
	int sac_socket;
	socklen_t len = sizeof(sac_cli);

	while((sac_socket = accept(listener_socket,&sac_cli, &len)) >0){
		log_info(log,"NUEVA CONEXIÓN");
		pthread_t sac_cli_thread;
		pthread_create(&sac_cli_thread,NULL,listen_sac_cli,(void*)(sac_socket));
	}
}

