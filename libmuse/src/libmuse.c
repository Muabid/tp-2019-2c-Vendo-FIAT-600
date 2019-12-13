
#include "libmuse.h"

int muse_init(int id, char* ip, int puerto){
	if(initialized <  0){
		int sock = connect_to_server(ip, puerto, NULL);
		if(sock == -1){
			puts("Error al conectar al servidor");
			return -1;
		}
		socketMuse = sock;

	}
	return initialized;
}
//	int sock = connect_to_server(ip, puerto, NULL);
//	if(sock>0){
//		socketMuse = sock;
//		uint32_t _id = id;
//		initialized = send_message(socketMuse,MUSE_INIT,&_id,sizeof(uint32_t));
//		if(initialized <0){
//
//		}else{
//
//		}
//	}

void muse_close(){
	if(initialized){//si ya esta cerrado no hace nada
		return;
	}
	send_status(socketMuse,MUSE_CLOSE,"0",1);
	close(socketMuse);
	initialized = false;
	free(id_muse);
	puts("Chau muse  :´(\n");
}

//int muse_alloc(char* id, uint32_t tamanio);
uint32_t muse_alloc(uint32_t tam){
	size_t sizeT = sizeof(tam) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void* aux = content;
	memcpy(aux,&tam,sizeof(tam));
	aux+=sizeof(tam);
	size_t len= strlen(id_muse);
	memcpy(aux,&len,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_ALLOC,content,sizeT);
	free(content);
	t_message* message = recv_message(socketMuse);
	int res;
	if(message->head == ERROR){
		//log
	}
	res = get_status(message);
	return res;
}

//int muse_free(char* id, uint32_t dir);
void muse_free(uint32_t dir){
	size_t sizeT = sizeof(dir) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void* aux = content;
	memcpy(aux,&dir,sizeof(dir));
	aux += sizeof(dir);
	memcpy(aux,strlen(id_muse),sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_FREE,content,sizeT);
	free(content);
	t_message* message = recv_message(socketMuse);
	int res;
	if(message->head == ERROR){
		puts("la re puta que te pario");
		raise(SIGSEGV);
	}
	if(message->content == -1){
		raise(SIGSEGV);
		//raise(5); ?
	}
	res = get_status(message);
	printf("Free realizado para %zu\n",dir);
}

//void* muse_get(char* id, uint32_t src, size_t n);
int muse_get(void* dst, uint32_t src, size_t n){
	size_t sizeT = sizeof(src) + sizeof(n) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void* aux = content;
	memcpy(aux,&src,sizeof(uint32_t));
	aux += sizeof(uint32_t);
	memcpy(aux,&n,sizeof(size_t));
	aux += sizeof(size_t);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_GET,content,sizeT);
	free(content);
	t_message* message = recv_message(socketMuse);
	int res;
	if(message->head == ERROR){
		return -1;
	}else {
		memcpy(dst,message->content,message->size);
		return 0;
	}
}

//void* muse_cpy(char* id, uint32_t dst, void* src, size_t n);
int muse_cpy(uint32_t dst, void* src, int n){
	size_t size = sizeof(uint32_t) + n + sizeof(int) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(size);
	void*aux = content;
	memcpy(aux,&dst,sizeof(uint32_t));
	aux+=sizeof(uint32_t);
	memcpy(aux,src,n);
	aux+=n;
	memcpy(aux,&n,sizeof(int));
	aux+=sizeof(int);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_CPY,content,size);
	t_message* message = recv_message(socketMuse);
	if(message->head == ERROR){
		raise(11);
	}else{
		//algo
	}
	return 0;

}

//int muse_map(char* id, char* path, uint32_t length, uint32_t flag);
uint32_t muse_map(char *path, size_t length, int flags){
	size_t sizeT = strlen(path) + sizeof(length) + sizeof(int) + sizeof(size_t)
			+ strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void*aux = content;
	size_t len= strlen(path);
	memcpy(aux,&len,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,path,len);
	aux+=strlen(path);
	memcpy(aux,&length,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,&flags,sizeof(int));
	aux+=sizeof(int);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_MAP,content,sizeT);
	t_message* message = recv_message(socketMuse);

	if(message->head == ERROR){
		return -1;
	}else{
		return *((uint32_t*)message->content);
	}
}

//int muse_sync(char* id, uint32_t addr, size_t len);
int muse_sync(uint32_t addr, size_t len){
	size_t sizeT = sizeof(uint32_t) + sizeof(size_t) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void*aux = content;
	memcpy(aux,&addr,sizeof(addr));
	aux+=sizeof(addr);
	memcpy(aux,&len,sizeof(len));
	aux+=sizeof(len);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_SYNC,content,sizeT);
	t_message* message = recv_message(socketMuse);
	if(message->head == ERROR){
		return -1;
	} else {
		// algo
	}
	return 0;
}

//int muse_unmap(char* id, uint32_t dir);
int muse_unmap(uint32_t dir){
	size_t sizeT = sizeof(uint32_t) + strlen(id_muse) + sizeof(size_t);
	void* content = malloc(sizeT);
	void*aux = content;
	memcpy(aux,&dir,sizeof(dir));
	aux+=sizeof(dir);
	size_t len_id = strlen(id_muse);
	memcpy(aux, len_id, sizeof(size_t));
	aux += sizeof(size_t);
	memcpy(aux,id_muse,strlen(id_muse));
	send_message(socketMuse,MUSE_SYNC,content,sizeT);
	t_message* message = recv_message(socketMuse);
	if(message->head == ERROR){
		return -1;
	} else {
		// algo
	}
	return 0;
}
