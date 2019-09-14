/*
 * nodes.h
 *
 *  Created on: 13 sep. 2019
 *      Author: utnso
 */

#ifndef NODES_H_
#define NODES_H_
#define BLOCK_SIZE 4096
#include "stdint.h"
#include <stdlib.h>

typedef struct __attribute__((__packed__)){
	unsigned char status;
	char file_name[71];
	int32_t root;
	int32_t size;
	char creation_date[8];
	char modification_date[8];
	int32_t blocks_ptr[1000];
}GFile ;

GFile create_GFile(unsigned char status,char file_name[71],
		int32_t root,int32_t size,char creation_date[8],
		char modification_date[8],int32_t blocks_ptr[1000]);

GFile read_GFile(void* cachoOfMemory);
#endif /* NODES_H_ */
