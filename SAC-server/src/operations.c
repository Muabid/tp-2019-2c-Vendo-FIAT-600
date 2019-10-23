/*
 * operations.c
 *
 *  Created on: 16 sep. 2019
 *      Author: utnso
 */


#include "operations.h"
#include <commons/log.h>
#include <commons/string.h>

int get_size_bytes_gFile(GFile node){

//	return sizeof(GFile) - sizeof(node.file_name) - sizeof(node.blocks_ptr);
	return 25;
}

int sac_getattr(int socket,const char* path){
	int index_node = search_node(path);
	if(index_node<0){
		return -1;
	}
	GFile node =  nodes_table[index_node];
	int32_t links= get_number_links(node,index_node);
//	GFile node = create_GFile(1,"hola",1,100,time(NULL),time(NULL));
	size_t size = get_size_bytes_gFile(node);

	log_info(log,"%s | %d | %i | %i | %i | %i",node.file_name,node.size,
			node.creation_date,node.modification_date,links,node.status);

	void*buf = malloc(size);
	void*aux = buf;
	memcpy(buf ,&node.size,sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memcpy(buf,&node.creation_date,sizeof(uint64_t));
	buf += sizeof(uint64_t);
	memcpy(buf,&node.modification_date,sizeof(uint64_t));
	buf += sizeof(uint64_t);
	memcpy(buf,&node.status,sizeof(char));
	buf += sizeof(char);
	memcpy(buf,&links,sizeof(int32_t));
	buf = aux;
	send_message(socket,GET_ATTR,buf,size);
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
