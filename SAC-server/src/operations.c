/*
 * operations.c
 *
 *  Created on: 16 sep. 2019
 *      Author: utnso
 */


#include "operations.h"


int get_size_bytes_gFile(GFile node){

	return sizeof(GFile) - strlen(node.file_name) - 1000;
}

int sac_getattr(int socket,const char* path){
	int index_node = search_node(path);
	if(index_node<0){
		return -1;
	}
	GFile node =  nodes_table[index_node];
	int32_t links= get_number_links(node,index_node);
	size_t size = get_size_bytes_gFile(node) + sizeof(int32_t);
	void*buf = malloc(size);
	void*aux = buf;
	memcpy(buf ,&node.size,sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memcpy(buf,&node.creation_date,8);
	buf += sizeof(uint64_t);
	memcpy(buf + 12,node.modification_date,8);
	buf += sizeof(uint64_t);
	memcpy(buf + 20,&node.status,1);
	buf += sizeof(char);
	memcpy(buf + 21,&links,4);
	buf += sizeof(uint32_t);
	send_message(socket,TEST,aux,size);
	free(buf);
	return 0;
}

int get_number_links(GFile node,int index){
	int n_links = node.status;

	if(node.status == 2){
		n_links+= get_subdirectories(index);
	}

	return n_links;

}

int get_subdirectories(int node){
	int n;
	for(int i = 0; i<BLOCKS_NODE; i++){
		if(nodes_table[i].root == node && nodes_table[i].status == 2)
			n++;
	}
	return n;
}


int sac_mknod(char* path){
	if(!search_node(path))
		return -1;


	char* file_name = get_name(path);
	char* directory = get_directory(path);
	int root = search_node(directory);
	int index_node = search_first_free_node();
	GFile node = nodes_table[index_node];

	node.size = 0;
	node.root = root;
	node.status = T_FILE;
	int free_block = search_and_test_first_free_block();
	if(free_block < 0)
		return free_block;

	node.blocks_ptr[0] =free_block;
	node.blocks_ptr[1] = 0;
	node.creation_date = node.modification_date = time(NULL);
	strcpy(node.file_name,file_name);

	char* data = get_block_data(free_block);
	memset(data,'\0', BLOCK_SIZE);
	free(file_name);
	free(directory);
	return 0;

}
