/*
 * operations.c
 *
 *  Created on: 16 sep. 2019
 *      Author: utnso
 */


#include "operations.h"


int get_size_bytes_gFile(GFile node){

	return 25 + strlen(node.file_name);;
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
	memcpy(buf ,&node.size,4);
	memcpy(buf + 4,node.creation_date,8);
	memcpy(buf + 12,node.modification_date,8);
	memcpy(buf + 20,&node.status,1);
	memcpy(buf + 21,&links,4);
	memcpy(buf + 25,(int32_t)strlen(node.file_name),4);
	memcpy(buf + 29,node.file_name,strlen(node.file_name));
	send_message(socket,TEST,buf,size);
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
	char *today = get_date();

	node.size = 0;
	node.root = root;
	node.status = T_FILE;
	int free_block = search_and_test_first_free_block();
	if(free_block < 0)
		return free_block;

	node.blocks_ptr[0] =free_block;
	node.blocks_ptr[1] = 0;
	memcpy(node.creation_date,today,8);
	memcpy(node.modification_date,today,8);
	strcpy(node.file_name,file_name);

	char* data = get_block_data(free_block);
	memset(data,'\0', BLOCK_SIZE);
	free(file_name);
	free(directory);
	free(today);
	return 0;

}
