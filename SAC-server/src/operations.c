/*
 * operations.c
 *
 *  Created on: 16 sep. 2019
 *      Author: utnso
 */


#include "operations.h"
#include <commons/log.h>
#include <commons/string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "aux.h"

int get_size_bytes_gFile(GFile node){

//	return sizeof(GFile) - sizeof(node.file_name) - sizeof(node.blocks_ptr);
	return 25;
}

int32_t get_number_links(uint8_t status,int index){
	if(status == T_FILE){
		return 1;
	}else{
		return get_subdirectories(index) + 2;
	}
}

int32_t get_subdirectories(int node){
	int32_t n=0;
	for(int i = 0; i<BLOCKS_NODE; i++){
		if(nodes_table[i].root == node && nodes_table[i].status == 2)
			n++;
	}
	return n;
}

int sac_getattr(int socket,const char* path){
	int index_node = search_node(path);
	if(index_node<0){
		send_status(socket,ERROR,-ENOENT);

		return -1;
	}
	int32_t links;
	if(strcmp(path,"/") == 0){
		links = get_number_links(T_DIR,0);
		send_message(socket,OK,&links,sizeof(int32_t));
		return 0;
	}
	GFile node =  nodes_table[index_node-1];
	links = get_number_links(node.status,index_node);

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
	memcpy(buf,&node.status,sizeof(uint8_t));
	buf += sizeof(uint8_t);
	memcpy(buf,&links,sizeof(int32_t));
	buf = aux;
	send_message(socket,OK,buf,size);
	free(buf);
	return 0;
}


int sac_mknod(int sock, const char* path){
	if(search_node(path) !=-1 ){
		log_error(log,"Archivo %s ya existe",path);
		send_status(sock,ERROR,-EEXIST);
		return -1;
	}

	char* file_name = get_name(path);
	char* directory = get_directory(path);
	int root = search_node(directory);
	if(root == -1){
		log_error(log,"Root %s no existe",directory);
		send_status(sock,ERROR,-ENOENT);
		return -1;
	}
	int index_node = search_first_free_node();
	GFile* node = &nodes_table[index_node];

	node->size = 0;
	node->root = root;
	node->status = T_FILE;
	int free_block = search_and_test_first_free_block();
	if(free_block < 0)
		return free_block;

	node->blocks_ptr[0] =free_block;
	node->blocks_ptr[1] = 0;
	node->creation_date = node->modification_date = time(NULL);
	memset(node->file_name,0,71);
	strcpy((char*)node->file_name,file_name);

	char* data = get_block_data(free_block);
	memset(data,'\0', BLOCK_SIZE);
	free(file_name);
	free(directory);
	log_info(log,"Archivo %s creado exitósamente",path);
	send_status(sock,OK,0);
	return 0;

}

int sac_create(int sock, const char* path){
	if(search_node(path) !=-1 ){
		log_error(log,"Archivo %s ya existe",path);
		send_status(sock,ERROR,-EEXIST);
		return -1;
	}

	char* file_name = get_name(path);
	char* directory = get_directory(path);
	int root = search_node(directory);
	if(root == -1){
		log_error(log,"Root %s no existe",directory);
		send_status(sock,ERROR,-ENOENT);
		return -1;
	}
	int index_node = search_first_free_node();
	GFile* node = &nodes_table[index_node];

	node->size = 0;
	node->root = root;
	node->status = T_FILE;
	int free_block = search_and_test_first_free_block();
	if(free_block < 0){
		send_status(socket,ERROR,-EDQUOT);
		return free_block;
	}

	node->blocks_ptr[0] =free_block;
	node->blocks_ptr[1] = 0;
	t_block_ptr* block_ptr =(t_block_ptr*)get_block_data(free_block);
	int free_block_data = search_and_test_first_free_block();

	if(free_block_data < 0){
		send_status(socket,ERROR,-EDQUOT);
		return free_block;
	}
	t_block* data = (t_block*)get_block_data(free_block);
	memset(data->data,'\0', BLOCK_SIZE);

	block_ptr->blocks_ptr[0] = free_block_data;
	block_ptr->blocks_ptr[1] = -1;
	data = (t_block*)get_block_data(free_block_data);
	memset(data->data,'\0', BLOCK_SIZE);
	node->creation_date = node->modification_date = time(NULL);
	memset(node->file_name,0,71);
	strcpy((char*)node->file_name,file_name);

	free(file_name);
	free(directory);
	log_info(log,"Archivo %s creado exitósamente",path);
	send_status(sock,OK,0);
	return 0;

}

void update_params(char** data,off_t*off,size_t* size,size_t* offset_in_block,int offset){
	*data +=offset;
	*off+=offset;
	*size+=offset;
	*offset_in_block=0;
}

bool need_new_block(off_t offset, int32_t file_size, size_t space_in_block) {
	return (offset >= (file_size + space_in_block)) & (file_size != 0);
}

int sac_write(int socket,const char* path,char* data, size_t size, off_t offset){
	log_info(log,"EXECUTING WRITE [%s]",path);

	if(size+offset > MAX_SIZE){
		log_info(log,"NO PODES DIRECCIONAR TANTO ESPACIO CAPO");
		send_status(socket,ERROR,-EDQUOT);
		return -1;
	}else if(free_blocks() < size/BLOCK_SIZE){
		log_info(log,"NO HAY BLOQUES DISPONIBLES");
		send_status(socket,ERROR,-EDQUOT);
		return -1;
	}

	int index_node = search_node(path);
	GFile* node = &nodes_table[index_node-1];
	int32_t file_size = node->size;
	size_t offset_in_block = offset % BLOCK_SIZE;
	int res;
	ptrGBloque 	block_ptr;
	ptrGBloque ptr_block_data;
	t_block* block_data;
	t_block_ptr* pointer_block;
	while (size != 0){

		// Actualiza los valores de espacio restante en bloque.
		size_t space_in_block = BLOCK_SIZE - (file_size % BLOCK_SIZE);
		if (space_in_block == BLOCK_SIZE) (space_in_block = 0); // Porque significa que el bloque esta lleno.
		if (file_size == 0) space_in_block = BLOCK_SIZE; /* Significa que el archivo esta recien creado y ya tiene un bloque de datos asignado */

		// Si el offset es mayor que el tamanio del archivo mas el resto del bloque libre, significa que hay que pedir un bloque nuevo
		// file_size == 0 indica que es un archivo que recien se comienza a escribir, por lo que tiene un tratamiento distinto (ya tiene un bloque de datos asignado).
		if (need_new_block(offset, file_size, space_in_block)) {

			// Si no hay espacio en el disco, retorna error.
			if (free_blocks() == 0){
				//ERROR
			}
			res = allocate_node(node);
			if (res != 0){
				//ERROR
			}

			block_data = (t_block*)get_block_data(res);

			// Actualiza el espacio libre en bloque.
			space_in_block = BLOCK_SIZE;

		} else {
			int* positions = get_position(offset);
			block_ptr = node->blocks_ptr[positions[0]];
			pointer_block = (t_block_ptr*) get_block_data(block_ptr);

			ptr_block_data = pointer_block->blocks_ptr[positions[1]];
			block_data = (t_block*) get_block_data(ptr_block_data);
		}
		// Escribe en ese bloque de datos.
		if (size >= BLOCK_SIZE){
			memcpy(block_data->data, data, BLOCK_SIZE);
			if ((node->size) <= (offset)){
				file_size = node->size += BLOCK_SIZE;
			}
			update_params(&data,&offset,&size,&offset_in_block,offset);
		} else if (size <= space_in_block){ /*Hay lugar suficiente en ese bloque para escribir el resto del archivo */
			memcpy(block_data->data + offset_in_block, data, size);
			if (node->size <= offset){
				file_size = node->size += size;
			}
			else if (node->size <= (offset + size)){
				file_size = node->size += (offset + size - node->size);
			}
			size = 0;
		} else { /* Como no hay lugar suficiente, llena el bloque y vuelve a buscar uno nuevo */
			memcpy(block_data->data + offset_in_block, data, space_in_block);
			file_size = node->size += space_in_block;
			update_params(&data,&offset,&size,&offset_in_block,offset);
		}

	}

	node->modification_date= time(NULL);

	send_status(socket,OK,0);

	return 0;
}

int sac_unlink(int socket,const char* path){
	send_status(socket,OK,0);
	return 0;
}

int sac_readdir(int socket,const char* path, off_t offset){
	log_info(log,"Leyendo %s",path);
	int index_nodo = search_node(path);
	GFile *node;
	if (index_nodo == -1){
		send_status(socket,ERROR,-ENOENT);
		return -1;
	}

	node = nodes_table;

	for (int i = 0; i < BLOCKS_NODE; i++){
		if ((index_nodo==(node->root)) & (((node->status) == T_DIR) | ((node->status) == T_FILE))){
			log_info(log,"File: %s",node->file_name);
			char file[71];
			fill_path(file,(char*)node->file_name,0);
			send_message(socket,DIR_NAME,file,strlen(file));
		}
		node++;
	}
	send_status(socket,OK,0);
	log_info(log,"Directorio %s leído",path);
	return 0;
}

int sac_read(int socket,const char* path, size_t size, off_t offset){
//	char* data = malloc(size);
//	int* positions = get_position(offset);
//	int index_node = search_node(path);
//	GFile* node = &nodes_table[index_node-1];
//	int ptr = node->blocks_ptr[positions[0]];
//	t_block_ptr* block = (t_block_ptr*)get_block_data(ptr);
//	int ptr_data = block->blocks_ptr[positions[1]];
//
//	if(node->size <= offset){
//		log_error(log, "Fuse intenta leer un offset mayor o igual que el tamanio de archivo.File: %s, Size: %d",path,node->size);
//		//ERROR
//	} else if (node->size <= (offset+size)){
//		size = ((node->size)-(offset));
//		log_error(log, "Fuse intenta leer una posicion mayor o igual que el tamanio de archivo.File: %s, Size: %d",path,node->size);
//	}
//
//	for(int i=0; i<BLOCKS_INDIRECT;i++){
//		for(int j=0;j<BLOCKS_NODE;j++){
//
//		}
//	}
//
//	send_message(socket,OK,data,size);
//	free(data);
	return 0;
}

int sac_mkdir(int socket,const char* path){
	if(search_node(path) !=-1 ){
		log_error(log,"Directorio %s ya existe",path);
		send_status(socket,ERROR,-EEXIST);

		return -1;
	}

	char* file_name = get_name(path);
	char* directory = get_directory(path);
	int root = search_node(directory);
	if(root == -1){
		log_error(log,"Root %s no existe",directory);
		send_status(socket,ERROR,-ENOENT);
		return -1;
	}
	int index_node = search_first_free_node();
	if(index_node == -1){
		send_status(socket,ERROR,-EDQUOT);
		return -1;
	}
	GFile* node = &nodes_table[index_node];
	int free_block = search_and_test_first_free_block();
	if(free_block < 0){
		send_status(socket,ERROR,-EDQUOT);
		return free_block;
	}

	node->status = T_DIR;
	memset(node->file_name,0,71);
	strcpy(node->file_name,file_name);
	node->size = 0;
	node->root = root;
	node->modification_date = node->creation_date = time(NULL);
	free(file_name);
	free(directory);
	log_info(log,"Directorio %s creado exitósamente",path);
	send_status(socket,OK,0);
	return 0;
}

int sac_rmdir(int socket,const char* path){
	log_info(log,"Borrando directorio %s",path);
	int index_node = search_node(path);
	GFile* node = nodes_table;

	for (int i = 0; i < BLOCKS_NODE; i++){
		if ((index_node==(node->root)) & (((node->status) == T_DIR) | ((node->status) == T_FILE))){
			send_status(socket,ERROR,-ENOTEMPTY);
			return -1;
		}
		node++;
	}
	node = &nodes_table[index_node-1];
	node->status = 0;
	log_info(log,"Directorio %s borrado exitósamente", path);
	send_status(socket,OK,0);
	return 0;
}

int sac_utimens(int socket,const char*path,uint64_t last_mod ){
	int index_node = search_node(path);
	GFile* node = &nodes_table[index_node-1];
	node->modification_date = last_mod;
	send_status(socket,OK,0);
	return 0;
}
