/*
 * nodes.h
 *
 *  Created on: 13 sep. 2019
 *      Author: utnso
 */

#ifndef NODES_H_
#define NODES_H_


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <commons/string.h>
#include <commons/log.h>

#define PTRGBLOQUE_SIZE 4
#define BLOCK_SIZE 4096
#define HEADER_BLOCKS 1
#define BLOCKS_NODE 1024
#define FILESYSTEM_SIZE 268435456
#define BLOCKS_TOTAL FILESYSTEM_SIZE/BLOCK_SIZE
#define BLOCKS_BITMAP gHeader.bit_map_size
#define BLOCKS_DATA BLOCKS_TOTAL -1 -BLOCKS_BITMAP - 1024
#define BITMAP_SIZE_BITS bitmap->size * 8
#define BLOCKS_FILESYSTEM FILESYSTEM_SIZE / BLOCK_SIZE
#define T_FILE 2
#define T_DIR 1

typedef struct{
	unsigned char identifier[3];
	uint32_t version;
	uint32_t init_block;
	uint32_t bit_map_size;
	unsigned char padding[4081];
}GHeader;

typedef struct{
	unsigned char status;
	char file_name[71];
	uint32_t root;
	uint32_t size;
	uint64_t creation_date;
	uint64_t modification_date;
	int32_t blocks_ptr[1000];
}GFile ;


typedef struct{
	char data[4096];
}t_block;

t_bitarray* bitmap;
GHeader gHeader;
GFile* nodes_table;
t_block* blocks_data;
char config_path[1000];
t_log * log;
int file_system_descriptor;
int size_file_system;

pthread_mutex_t bitarray_mutex;

GFile create_GFile(char status, char file_name[71], int32_t root, int32_t size,
		uint64_t creation_date, uint64_t modification_date);

GHeader create_sac_header(char identifier[3],int32_t version,
		int32_t init_block,int32_t bit_map_size);

GFile read_GFile(void* cachoOfMemory);

int search_node(const char* path);

GHeader* file_system;

char* get_name(const char* path);
char* get_directory(const char* path);
int search_first_free_node();
int search_first_free_block();
int search_and_test_first_free_block();
int* get_position(off_t offset);
int free_blocks();
int allocate_node(GFile* node);
char* get_block_data(int index_block);
int fsize(char* path);
#endif /* NODES_H_ */
