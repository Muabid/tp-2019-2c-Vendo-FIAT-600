/*
 * nodes.c
 *
 *  Created on: 13 sep. 2019
 *      Author: utnso
 */
#include "nodes.h"

int isLastchar(const char* str, char chr){
	if ( ( str[strlen(str)-1]  == chr) ) return 1;
	return 0;
}

int search_node(const char* path) {
	if (!strcmp(path, "/"))
		return 0;
	char* name = get_name(path);
	char* directory = get_directory(path);
	int root = search_node(directory);
	int index;
	if(root>=0){
		for (index = 0;
				(nodes_table[index].root != root
						|| strcmp(nodes_table[index].file_name, name) != 0)
							&& index < BLOCKS_NODE; index++);

		if (index >= BLOCKS_NODE){
			return -1;
		}
	}

	free(name);
	free(directory);
	return index+1;
}

char* get_name(const char* path) {
	char** aux = string_split((char*) path, "/");
	int i;
	for (i = 0; aux[i] != NULL; i++)
		;
	char * name = malloc(strlen(aux[i - 1]) + 1);
	strcpy(name, aux[i - 1]);
	name[strlen(aux[i - 1])] = '\0';
	for (int j = 0; aux[j] != NULL; j++)
		free(aux[j]);
	free(aux);
	return name;
}

char* get_directory(const char* path) {
	char* file = get_name(path);
	char* directory = malloc(strlen(path) + 1);
	strcpy(directory, path);
	if (isLastchar(path, '/')) {
		directory[strlen(directory)-1] = '\0';
	}
	int i = strlen(directory) - strlen(file);
	directory[i] = '\0';

	free(file);
	return directory;
}

int search_first_free_node(){
	int i;
	for(i= 0; nodes_table[i].status !=0 && i < BLOCKS_NODE; i++);
	if(i>=BLOCKS_NODE)
		return -1;
	else
		return i;
}

int search_and_test_first_free_block(){
	int res = -1;
	for(int i = 0; i < BITMAP_SIZE_BITS && res == -1; i++){
		if(bitarray_test_bit(bitmap,i) == 0){
			bitarray_set_bit(bitmap,i);
			res = i;
		}
	}
	log_info(logger,"Se reservo el bloque %i",res);
	return res;
}

int free_blocks(){
	int free_nodes=0;
	for (int i = 0; i < BITMAP_SIZE_BITS; i++){
		if (bitarray_test_bit(bitmap, i) == 0)
			free_nodes++;
	}
	log_info(logger,"BLOQUES LIBRES: [%i]",free_nodes);
	return free_nodes;
}

int32_t* get_position(off_t offset){
	div_t divi = div(offset, (BLOCK_SIZE*BLOCKS_NODE));
	int32_t* position = malloc(sizeof(int32_t)*2);
	position[0] = divi.quot;
	position[1] = divi.rem/BLOCK_SIZE;
	//La primera posición indica el bloque de puntero, la segunda el puntero de bloque de datos
	return position;
}

char* get_block_data(int index_block){
	return (char*)&blocks_data[index_block - HEADER_BLOCKS - BLOCKS_NODE - BLOCKS_BITMAP];
}

int allocate_node(GFile* node){
	int free_block = search_and_test_first_free_block();
	if(free_block >0){
		int new_node;
		int* position = get_position(node->size);
		int indirect_pointer_block = position[0];
		int pointer_data = position[1];

		if ((node->blocks_ptr[indirect_pointer_block] != 0)){
			if (pointer_data == 1024) {
				pointer_data = 0;
				indirect_pointer_block++;
			}
		}

		if(pointer_data == 0){
			new_node = search_and_test_first_free_block();
			if(new_node <0){
				return new_node;
			}
			node->blocks_ptr[indirect_pointer_block] = new_node;
			node->blocks_ptr[indirect_pointer_block + 1] = 0;
		}else{
			new_node = node->blocks_ptr[indirect_pointer_block];
		}

		t_block_ptr* nodes_pointers = (t_block_ptr*)get_block_data(new_node);
		nodes_pointers->blocks_ptr[pointer_data] = free_block;
	}
	return free_block;
}
