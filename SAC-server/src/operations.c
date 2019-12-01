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
#include <shared/utils.h>
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

	log_info(logger,"%s | %d | %i | %i | %i | %i",node.file_name,node.size,
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

int sac_create(int sock, const char* path){
	if(search_node(path) !=-1 ){
		log_error(logger,"Archivo %s ya existe",path);
		send_status(sock,ERROR,-EEXIST);
		return -1;
	}

	char* file_name = get_name(path);
	char* directory = get_directory(path);
	int root = search_node(directory);
	if(root == -1){
		log_error(logger,"Root %s no existe",directory);
		send_status(sock,ERROR,-ENOENT);
		free(file_name);
		free(directory);
		return -1;
	}
	int index_node = search_first_free_node();
	GFile* node = &nodes_table[index_node];

	node->size = 0;
	node->root = root;
	node->status = T_FILE;
	int free_block = search_and_test_first_free_block();
	if(free_block < 0){
		free(file_name);
		free(directory);
		send_status(sock,ERROR,-EDQUOT);
		return free_block;
	}
	node->blocks_ptr[0] =free_block;
	node->blocks_ptr[1] = 0;
	t_block_ptr* block_ptr =(t_block_ptr*)get_block_data(free_block);
	int free_block_data = search_and_test_first_free_block();

	if(free_block_data < 0){
		send_status(sock,ERROR,-EDQUOT);
		free(file_name);
		free(directory);
		return free_block;
	}
	t_block* data = (t_block*)get_block_data(free_block);
	memset(data->data,'\0', BLOCK_SIZE);

	block_ptr->blocks_ptr[0] = free_block_data;
	block_ptr->blocks_ptr[1] = 0;
	data = (t_block*)get_block_data(free_block_data);
	memset(data->data,'\0', BLOCK_SIZE);
	node->creation_date = node->modification_date = time(NULL);
	memset(node->file_name,0,71);
	strcpy((char*)node->file_name,file_name);

	free(file_name);
	free(directory);
	log_info(logger,"Archivo %s creado exitósamente",path);
	send_status(sock,OK,0);
	return 0;

}

void update_params(char** data,off_t*off,size_t* size,size_t* offset_in_block,int offset){
	*data +=offset;
	*off+=offset;
	*size-=offset;
	*offset_in_block=0;
}

bool need_new_block(off_t offset, int32_t file_size, size_t space_in_block) {
	return (offset >= (file_size + space_in_block)) & (file_size != 0);
}

int sac_write(int socket,const char* path,char* data, size_t size, off_t offset){
	if(size+offset > MAX_SIZE){
		log_info(logger,"NO PODES DIRECCIONAR TANTO ESPACIO CAPO");
		send_status(socket,ERROR,-EDQUOT);
		return -1;
	}else if(free_blocks() < size/BLOCK_SIZE){
		log_info(logger,"NO HAY BLOQUES DISPONIBLES");
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

		size_t space_in_block = BLOCK_SIZE - (file_size % BLOCK_SIZE);
		if (space_in_block == BLOCK_SIZE) (space_in_block = 0);
		if (file_size == 0) space_in_block = BLOCK_SIZE;

		if (need_new_block(offset, file_size, space_in_block)) {

			if (free_blocks() == 0){
				send_status(0,ERROR,-EDQUOT);
				return -1;
			}
			res = allocate_node(node);
			if (res < 0){
				send_status(0,ERROR,-EDQUOT);
				return -1;
			}

			block_data = (t_block*)get_block_data(res);

			space_in_block = BLOCK_SIZE;

		} else {
			int* positions = get_position(offset);
			block_ptr = node->blocks_ptr[positions[0]];
			pointer_block = (t_block_ptr*) get_block_data(block_ptr);

			ptr_block_data = pointer_block->blocks_ptr[positions[1]];
			block_data = (t_block*) get_block_data(ptr_block_data);
			free(positions);
		}
		if (size >= BLOCK_SIZE){
			memcpy(block_data->data, data, BLOCK_SIZE);
			if ((node->size) <= (offset)){
				file_size = node->size += BLOCK_SIZE;
			}
			update_params(&data,&offset,&size,&offset_in_block,BLOCK_SIZE);
		} else if (size <= space_in_block){
			memcpy(block_data->data + offset_in_block, data, size);
			if (node->size <= offset){
				file_size = node->size += size;
			}
			else if (node->size <= (offset + size)){
				file_size = node->size += (offset + size - node->size);
			}
			size = 0;
		} else {
			memcpy(block_data->data + offset_in_block, data, space_in_block);
			file_size = node->size += space_in_block;
			update_params(&data,&offset,&size,&offset_in_block,space_in_block);
		}

	}
//	log_info(logger,"WRITED: [%s]",aux_data);
	node->modification_date= time(NULL);

	send_status(socket,OK,0);

	return 0;
}

void clean_bit(int index_block){
	bitarray_clean_bit(bitmap, index_block);
	log_info(logger,"Se libero el bloque [%i]",index_block);
}

void delete_blocks(GFile* node, int offset,bool delete) {
	int index_ptr, index_block;
	t_block_ptr* blocks_ptr;
	t_block* data;
	int* positions = get_position(offset);
	for (int i = positions[0]; i < BLOCKS_INDIRECT && node->blocks_ptr[i] != 0;
			i++) {
		index_ptr = node->blocks_ptr[i];
		blocks_ptr = (t_block_ptr*) get_block_data(index_ptr);
		for (int j = positions[1];
				j < BLOCKS_NODE && blocks_ptr->blocks_ptr[j] != 0; j++) {
			index_block = blocks_ptr->blocks_ptr[j];
			data = (t_block*) get_block_data(index_block);
			memset(data->data, 0, BLOCK_SIZE);
			if(j != 0 || delete){
				clean_bit(index_block);
				blocks_ptr->blocks_ptr[j] = 0;
			}
		}
		if(i != 0 || delete){
			node->blocks_ptr[i] = 0;
			clean_bit(index_ptr);
		}
	}
	free(positions);
	node->size = offset;
}

int sac_truncate(int socket,const char* path, off_t offset){
	log_info(logger,"TRUNCATE OFFSET [%i] - PATH [%s]");
	int index_node = search_node(path);

	if (index_node == -1){
		send_status(socket,ERROR,-ENOENT);
		return -1;
	}

	GFile* node = &nodes_table[index_node - 1];
	delete_blocks(node, offset,0);
	send_status(socket,OK,0);
	return 0;
}

int sac_unlink(int socket,const char* path){
	log_info(logger,"UNLINK PATH [%s]");
	int index_node = search_node(path);

	if (index_node == -1){
		send_status(socket,ERROR,-ENOENT);
		return -1;
	}
	GFile* node = &nodes_table[index_node - 1];

	delete_blocks(node,0,1);
	node->status = T_DELETED;
	memset(node->file_name,0,71);
	send_status(socket,OK,0);
	return 0;
}

int sac_readdir(int socket,const char* path, off_t offset){
	int index_nodo = search_node(path);
	GFile *node;
	if (index_nodo == -1){
		send_status(socket,ERROR,-ENOENT);
		return -1;
	}

	node = nodes_table;

	for (int i = 0; i < BLOCKS_NODE; i++){
		if ((index_nodo==(node->root)) & (((node->status) == T_DIR) | ((node->status) == T_FILE))){
			log_info(logger,"File: %s",node->file_name);
			char file[71];
			fill_path(file,(char*)node->file_name,0);
			send_message(socket,DIR_NAME,file,strlen(file));
		}
		node++;
	}
	send_status(socket,OK,0);
	return 0;
}

int sac_read(int socket,const char* path, size_t size, off_t offset){
	int o_size = size;
	char* data = malloc(size+1);
	char* aux_data = data;
	int* positions = get_position(offset);
	int index_node = search_node(path);
	GFile* node = &nodes_table[index_node-1];
	t_block_ptr* blocks_ptr;
	t_block* block_data;
	ptrGBloque block_data_ptr;
	ptrGBloque block_ptr;
	size_t offset_in_block = offset % BLOCK_SIZE;
	if(node->size <= offset){
		log_error(logger, "Fuse intenta leer un offset mayor o igual que el tamanio de archivo.File: %s, Size: %d",path,node->size);
		send_status(socket,ERROR,-1);
		return -1;
	} else if (node->size < (offset+size)){
		size = ((node->size)-(offset));
		log_error(logger, "Fuse intenta leer una posicion mayor que el tamanio de archivo.File: %s, Size: %d",path,node->size);
	}

	for(int ptr_ind=positions[0]; ptr_ind<BLOCKS_INDIRECT && size>0;ptr_ind++){
		block_ptr=node->blocks_ptr[ptr_ind];
		blocks_ptr = (t_block_ptr*)get_block_data(block_ptr);
		for(int j=positions[1];j<BLOCKS_NODE && size >0;j++){
			block_data_ptr = blocks_ptr->blocks_ptr[j];
			block_data = (t_block*) get_block_data(block_data_ptr);

			if(j==positions[1]){
				int bytes_readed = min(BLOCK_SIZE-offset_in_block,size);
				memcpy(aux_data,block_data->data+offset_in_block,bytes_readed);
				size-=bytes_readed;
				aux_data+=bytes_readed;
				offset_in_block=0;
			}else if(size <BLOCK_SIZE){
				memcpy(aux_data,block_data->data,size);
				aux_data+=size;
				size=0;
			}else{
				memcpy(aux_data,block_data->data,BLOCK_SIZE);
				aux_data+=size;
				size-=BLOCK_SIZE;
			}

		}
	}
	data[o_size]='\0';
	log_info(logger,"DATA: [%s]",data);
	send_message(socket,OK,data,o_size);
	free(data);
	return 0;
}

int sac_mkdir(int socket,const char* path){
	if(search_node(path) !=-1 ){
		log_error(logger,"Directorio %s ya existe",path);
		send_status(socket,ERROR,-EEXIST);
		return -1;
	}

	char* file_name = get_name(path);
	char* directory = get_directory(path);
	int root = search_node(directory);
	if(root == -1){
		log_error(logger,"Root %s no existe",directory);
		send_status(socket,ERROR,-ENOENT);
		free(file_name);
		free(directory);
		return -1;
	}
	int index_node = search_first_free_node();
	if(index_node == -1){
		send_status(socket,ERROR,-EDQUOT);
		free(file_name);
		free(directory);
		return -1;
	}
	GFile* node = &nodes_table[index_node];
	int free_block = search_and_test_first_free_block();
	if(free_block < 0){
		send_status(socket,ERROR,-EDQUOT);
		free(file_name);
		free(directory);
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
	log_info(logger,"Directorio %s creado exitósamente",path);
	send_status(socket,OK,0);
	return 0;
}

int sac_rmdir(int socket,const char* path){
	log_info(logger,"Borrando directorio %s",path);
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
	log_info(logger,"Directorio %s borrado exitósamente", path);
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

int sac_rename(int socket,const char* old_path,const char* new_path){
	log_info(logger, "OLD PATH [%s] - NEW PATH [%s]",old_path,new_path);
	int index_node = search_node(old_path);
	char* file_name = get_name(new_path);
	char* directory = get_directory(new_path);
	int root = search_node(directory);
	if(root < 0){
		send_status(socket,ERROR,-ENOENT);
		free(directory);
		return -1;
	}
	GFile* node = &nodes_table[index_node-1];
	memset(node->file_name,0,71);
	memcpy(node->file_name,file_name,71);
	node->modification_date = time(NULL);
	node->root = root;
	free(directory);
	free(file_name);
	send_status(socket,OK,0);
	return 0;
}
