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

int sac_getattr(int socket,const char* path){

	GFile node;
//	int index_node = search_node(path);
//	if(index_node<0){
//		return -1;
//	}
//	GFile node =  nodes_table[index_node];
//	int32_t links= get_number_links(node,index_node);
	int32_t links = 1;
	if (strcmp(path, "/") == 0) {
		node = create_GFile(2,"/",1,100,time(NULL),time(NULL));
		links = 2;
	}else if (strcmp(path, "/hola") == 0) {
		node = create_GFile(1,"hola",1,100,time(NULL),time(NULL));
	}else if(strcmp(path, "/chau") == 0){
		node = create_GFile(1,"chau",1,100,time(NULL),time(NULL));
	}else{
		send_status(socket,ERROR,-ENOENT);
		return 0;
	}


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
	send_message(socket,OK,buf,size);
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


int sac_mknod(int sock, const char* path){
//	if(!search_node(path))
//		return -1;
//
//
//	char* file_name = get_name(path);
//	char* directory = get_directory(path);
//	int root = search_node(directory);
//	int index_node = search_first_free_node();
//	GFile node = nodes_table[index_node];
//
//	node.size = 0;
//	node.root = root;
//	node.status = T_FILE;
//	int free_block = search_and_test_first_free_block();
//	if(free_block < 0)
//		return free_block;
//
//	node.blocks_ptr[0] =free_block;
//	node.blocks_ptr[1] = 0;
//	node.creation_date = node.modification_date = time(NULL);
//	strcpy(node.file_name,file_name);
//
//	char* data = get_block_data(free_block);
//	memset(data,'\0', BLOCK_SIZE);
//	free(file_name);
//	free(directory);

//	log_info(log,"Creando %s",path);
//	FILE * f = fopen(path, "r+");
//	t_header head;
//	if(f == NULL){
//		f = fopen(path,"w+");
//		head = OK;
//		log_info(log,"Archivo %s creado");
//	}else{
//		log_info(log,"Archivo %s ya existe");
//		head = FILE_ALREADY_EXISTS;
//	}
//	send_header(sock,head);
//	fclose(f);
	return 0;

}

int sac_write(int socket,const char* path,char* data, size_t size, off_t offset){
	FILE * f = fopen(path, "wb");
	fseek(f,offset,SEEK_SET);
	fwrite(data,size,sizeof(char),f);
	fclose(f);
	log_info(log,"Se escribio %s en el archivo %s.",data,path);
	send_status(socket,OK,size);
	return 0;
}
int sac_unlink(int socket,const char* path){
	remove(path);
	send_status(socket,OK,0);
	return 0;
}
int sac_readdir(int socket,const char* path, off_t offset){
	log_info(log,"Leyendo %s",path);
//	DIR *dirp;
//	struct dirent *direntp;
//
//	dirp = opendir(path);
//	if (dirp == NULL){
//		send_message(socket,DIRECTORY_NOT_FOUND,NULL,0);
//	}
//
//	while ((direntp = readdir(dirp)) != NULL) {
//		log_info(log,"Leyendo %s",direntp->d_name);
//		send_message(socket,DIR_NAME,direntp->d_name,strlen(direntp->d_name));
//	}

//	send_header(socket,OK);
//	int nodo = search_node(path), res = 0;
//	GFile *node;
//
//	if (nodo == -1){
//		send_status(socket,ERROR,-ENOENT);
//		return -1;
//	}
//
//	node = nodes_table;
//
//	for (int i = 0; i < BLOCKS_NODE;  (i++)){
//		if ((nodo==(node->root)) & (((node->status) == T_DIR) | ((node->status) == T_FILE))){
//			char file[71];
//			fill_path(file,(node[0]).file_name,0);
//			send_message(socket,DIR_NAME,file,strlen(file));
//		}
//		node++;
//	}
	send_message(socket,DIR_NAME,"hola",4);
	send_message(socket,DIR_NAME,"chau",4);
	send_status(socket,OK,0);
	log_info(log,"Directorio %s leído",path);
	return 0;
}

int sac_read(int socket,const char* path, size_t size, off_t offset){
//	log_info(log,"Leyendo archivo: %s - size: %i - offset: %i",path,size,offset);
//	char* buff = malloc(size + 1);
//	FILE * f = fopen(path, "r+");
//	fseek(f,offset,SEEK_SET);
//	fread(buff,size,sizeof(char),f);
//	buff[size] = '\0';
//	log_info(log,"Archivo %s leído - Contenido: %s - Bytes leídos %i",path,buff,size);
//	send_message(socket,OK,buff,size);
//	return 0;

	send_message(socket,OK,"HOLA",4);
	return 0;
}

int sac_mkdir(int socket,const char* path){
//	struct stat st = {0};
//	t_header res = OK;
//	if (stat(path, &st) == -1) {
//	    mkdir(path, 0700);
//	    log_info(log,"Directorio %s creado.",path);
//	}else{
//		res = FILE_ALREADY_EXISTS;
//		log_info(log,"Directorio %s ya existe.",path);
//	}
//	send_header(socket,res);
	return 0;
}

int sac_rmdir(int socket,const char* path){
	return 0;
}
