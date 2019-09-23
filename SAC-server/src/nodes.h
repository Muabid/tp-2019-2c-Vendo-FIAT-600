/*
 * nodes.h
 *
 *  Created on: 13 sep. 2019
 *      Author: utnso
 */

#ifndef NODES_H_
#define NODES_H_

#define BLOCK_SIZE 4096
#define HEADER_BLOCKS 1
#define BLOCKS_NODE 1024
#define FILESYSTEM_SIZE 8388608
#define BLOCKS_TOTAL FILESYSTEM_SIZE/BLOCK_SIZE
#define BLOCKS_BITMAP BLOCKS_TOTAL/8
#define BLOCKS_DATA BLOCKS_TOTAL -1 -BLOCKS_BITMAP - 1024
#include "stdint.h"
#include <stdlib.h>
#include <string.h>
#include <commons/bitarray.h>
#include <commons/string.h>


typedef struct{
	unsigned char identifier[3];
	int32_t version;
	int32_t init_block;
	int32_t bit_map_size;
	unsigned char padding[4081];
}GHeader;

typedef struct{
	unsigned char status;
	char file_name[71];
	int32_t root;
	int32_t size;
	char creation_date[8];
	char modification_date[8];
	int32_t blocks_ptr[1000];
}GFile ;

t_bitarray* bitmap;
GFile nodes_table[1024];

typedef struct{
	char data[4096];
}t_block;

GFile create_GFile(char status,char file_name[71],
		int32_t root,int32_t size,char creation_date[8],
		char modification_date[8]);

GHeader create_sac_header(char identifier[3],int32_t version,
		int32_t init_block,int32_t bit_map_size);

GFile read_GFile(void* cachoOfMemory);

int search_node(const char* path);

char* get_name(const char* path);
char* get_directory(const char* path);
int search_first_free_node();
int search_first_free_block();
#endif /* NODES_H_ */
