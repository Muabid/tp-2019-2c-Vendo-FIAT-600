/*
 * nodes.c
 *
 *  Created on: 13 sep. 2019
 *      Author: utnso
 */
#include "nodes.h"

GFile create_GFile(char status,char file_name[71],
		int32_t root,int32_t size,char creation_date[8],
		char modification_date[8],int32_t blocks_ptr[1000]){

	GFile gFile ={
			.status =status,
			.root = root,
			.size = size
	};

	strcpy(gFile.file_name,file_name);
	strcpy(gFile.creation_date,creation_date);
	strcpy(gFile.modification_date,modification_date);
	for(int i=0; i<1000; i++){
		gFile.blocks_ptr[i] = blocks_ptr[i];
	}

	return gFile;
}

GHeader create_sac_header(char identifier[3],int32_t version,
		int32_t init_block,int32_t bit_map_size){
	GHeader header = {
			.version = version,
			.init_block = init_block,
			.bit_map_size = bit_map_size,
	};

	strcpy(header.identifier,identifier);

	return header;
}

int search_node(const char* path){
	if(strcmp(path,"/"))
		return 0;
	int index;
	for(index= 1; strcmp(nodes_table[index].file_name,path) !=0 && index<BLOCKS_NODE ;index++);

	if(index>=BLOCKS_NODE)
		return -1;
	else{
		return index;
	}

}


