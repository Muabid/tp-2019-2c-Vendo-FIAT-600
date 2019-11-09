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
#include <commons/collections/queue.h>
#include <signal.h>
#include "aux.h"

void init_semaphores() {
	pthread_rwlockattr_t attrib;
	pthread_rwlockattr_init(&attrib);
	pthread_rwlockattr_setpshared(&attrib, PTHREAD_PROCESS_SHARED);
	pthread_rwlock_init(&rwlock, &attrib);
}

void sig_term(int sig) {
	pthread_mutex_destroy(&bitarray_mutex);
	fdatasync(file_system_descriptor);
	munmap(file_system, size_file_system);
	close(file_system_descriptor);
	exit(0);
}

int main(int argc, const char* argv[]) {
	log = log_create("./resources/log", "SERVER", true, LOG_LEVEL_INFO);
	log_info(log, "%i", sizeof(GFile));
	log_info(log, "%i", sizeof(GHeader));
	log_info(log, "%i", sizeof(t_block));

	signal(SIGTERM, sig_term);
	signal(SIGABRT, sig_term);
	init_semaphores();

//	const char* path = argv[1];
	sac_load_config("./resources/sac.config");
	size_file_system = fsize(sac_config->file_system_path);
	log_info(log, "Filesystem Size: %i", size_file_system);

	if ((file_system_descriptor = open(sac_config->file_system_path, O_RDWR, 0))
			== -1)
		return -1;

	file_system = (t_block*) mmap(NULL, size_file_system + 1,
			PROT_WRITE | PROT_READ | PROT_EXEC,
			MAP_SHARED, file_system_descriptor, 0);
	gHeader = *((GHeader*) file_system);

	log_info(log,
			"Identifier: %s | Version: %i | Init Block: %i | Bitmap Size: %i",
			gHeader.identifier, gHeader.version, gHeader.init_block,
			gHeader.bit_map_size);

	GFile* bitmap_content = (GFile*) &file_system[HEADER_BLOCKS];
	bitmap = bitarray_create_with_mode((char*) bitmap_content,
			size_file_system / BLOCK_SIZE / CHAR_BIT, LSB_FIRST);
	nodes_table = (GFile*) &file_system[HEADER_BLOCKS + BLOCKS_BITMAP];
	blocks_data = (t_block*) &file_system[HEADER_BLOCKS + BLOCKS_BITMAP
			+ BLOCKS_NODE];

//	sac_mkdir(1,"/hola");
//	sac_mknod(1,"/hola/chau");
//	sac_getattr(0,"/hola");
//	sac_getattr(0,"/hola/chau");
	init_sac_server();
	exit(0);
}

void* listen_sac_cli(void* socket) {
	int sac_socket = (int) (socket);
	char path[71];
	while (1) {
		t_message* message = recv_message(sac_socket);
		switch (message->head) {
		case HI_PLEASE_BE_MY_FRIEND:
			send_message(sac_socket, HI_PLEASE_BE_MY_FRIEND, &sac_socket,
					sizeof(sac_socket));
			break;
		case GET_ATTR: {
			fill_path(path, message->content, 0);
			pthread_rwlock_rdlock(&rwlock);
			sac_getattr(sac_socket, path);
			pthread_rwlock_unlock(&rwlock);
			break;
		}
		case MKDIR: {
			fill_path(path, message->content, 0);
			pthread_rwlock_wrlock(&rwlock);
			sac_mkdir(sac_socket, path);
			pthread_rwlock_unlock(&rwlock);
			break;
		}
		case RMDIR: {
			fill_path(path, message->content, 0);
			pthread_rwlock_rdlock(&rwlock);
			sac_rmdir(sac_socket, path);
			pthread_rwlock_unlock(&rwlock);
			break;
		}
		case WRITE: {
			void * aux = message->content;
			fill_path(path, aux, 1);
			aux += sizeof(size_t);
			aux += strlen(path);
			size_t size;
			memcpy(&size, aux, sizeof(size_t));
			aux += sizeof(size_t);
			off_t offset;
			memcpy(&offset, aux, sizeof(off_t));
			aux += sizeof(off_t);
			char * data = malloc(size);
			memcpy(data, aux, size);
			pthread_rwlock_wrlock(&rwlock);
			sac_write(sac_socket, path, data, size, offset);
			pthread_rwlock_unlock(&rwlock);
		}
			break;
		case MKNODE: {
			fill_path(path, message->content, 0);
			pthread_rwlock_wrlock(&rwlock);
			sac_mknod(sac_socket, path);
			pthread_rwlock_unlock(&rwlock);
			break;
		}
		case CREATE: {
			fill_path(path, message->content, 0);
			pthread_rwlock_wrlock(&rwlock);
			sac_create(sac_socket, path);
			pthread_rwlock_unlock(&rwlock);
			break;
		}
		case READ: {
			void * aux = message->content;
			fill_path(path, aux, 1);
			aux += sizeof(size_t);
			aux += strlen(path);
			size_t size;
			memcpy(&size, aux, sizeof(size_t));
			aux += sizeof(size_t);
			off_t offset;
			memcpy(&offset, aux, sizeof(off_t));
			pthread_rwlock_rdlock(&rwlock);
			sac_read(sac_socket, path, size, offset);
			pthread_rwlock_unlock(&rwlock);
			break;
		}
		case UNLINK: {
			fill_path(path, message->content, 0);
			pthread_rwlock_wrlock(&rwlock);
			sac_unlink(sac_socket, path);
			pthread_rwlock_unlock(&rwlock);
			break;
		}
		case READDIR:
			fill_path(path, message->content, 0);
			pthread_rwlock_rdlock(&rwlock);
			sac_readdir(sac_socket, path, 0);
			pthread_rwlock_unlock(&rwlock);
			break;
		case NO_CONNECTION:
			log_info(log, "CLIENTE DESCONECTADO");
			free_t_message(message);
			pthread_exit(NULL);
			return NULL;
			break;
		case ERROR_RECV:
			free_t_message(message);
			log_info(log, "ERROR COMUNIACIÓN");
			pthread_exit(NULL);
			return NULL;
		}
		free_t_message(message);
	}
}

void init_sac_server() {
	listener_socket = init_server(8080);
	log_info(log, "Servidor levantado!!!");
	struct sockaddr sac_cli;
	socklen_t len = sizeof(sac_cli);
	do {
		int sac_socket = accept(listener_socket, &sac_cli, &len);
		if (sac_socket > 0) {
			log_info(log, "NUEVA CONEXIÓN");
			pthread_t sac_cli_thread;
			pthread_create(&sac_cli_thread, NULL, listen_sac_cli,
					(void*) (sac_socket));
		} else {
			log_error(log, "Error aceptando conexiones: %s", strerror(errno));
		}
	} while (1);
}

