/*
 * operations.c
 *
 *  Created on: 16 sep. 2019
 *      Author: utnso
 */

#include "operations.h"


int getattr(const char* path){
	int index_node = search_node(path);
	GFile node =  nodes_table[index_node];
	int32_t subdirectories= get_subdirectories(index_node);
	void* content = malloc(92);
	memcpy(content,node.file_name,71);
	memcpy(content + 71,node.size,4);
	memcpy(content + 75,node.creation_date,8);
	memcpy(content + 83,node.modification_date,8);
	memcpy(content + 87,node.status,1);
	memcpy(content + 88,subdirectories,4);
	//aca se envia el cacho de memoria al sac cli con la metadata
	//send_message()
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
