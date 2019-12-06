/*
 * SAC-cli.c
 *
 *  Created on: 28 sep. 2019
 *      Author: utnso
 */

#include <shared/protocol.h>
#include "operations_client.h"
#include <shared/net.h>
#include <stdint.h>
#include <commons/log.h>

//Variable global para la conexión con el SAC-server
static int sock;
static t_log* log;
/*
 * Esta es una estructura auxiliar utilizada para almacenar parametros
 * que nosotros le pasemos por linea de comando a la funcion principal
 * de FUSE. No comprendo bien este struct
 */
struct t_runtime_options {
	char* welcome_msg;
} runtime_options;


/*
 * Esta Macro sirve para definir nuestros propios parametros que queremos que
 * FUSE interprete. Esta va a ser utilizada mas abajo para completar el campos
 * welcome_msg de la variable runtime_options
 */
#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }


/*
 * Implementaciones de FUSE. Hay que definir si es necesario mandar ciertos parametros en el contenido o son al pedo.
 * Falta definir que se hace con lo recibido (cómo deserializarlo y en qué casos vale la pena), cómo implementar hilos para que sean cosas en paralelo, ver tema de la función main.
 *
 */

static int do_getattr(const char *path, struct stat *st) {
	int op_res;
	if(strcmp(path,"/") == 0){
		op_res = send_message(sock, GET_ATTR, path,
				strlen(path));

		if (op_res >= 0) {
			t_message* message = recv_message(sock); // Acá me va a responder el server algo,
			if(message->head == OK){
				st->st_nlink = *((int32_t*)message->content);
				st->st_mode = S_IFDIR | 0755;
				return 0;
			}else{
				return get_status(message);
			}
		}
	}

	int res= 0;
	memset(st, 0, sizeof(struct stat));

	uint32_t size=0;
	int32_t hardlinks=0;
	uint64_t creation_date, modification_date;
	uint8_t status=0;
	op_res = send_message(sock, GET_ATTR, path,
			strlen(path));

	if (op_res >= 0) {
		t_message* message = recv_message(sock); // Acá me va a responder el server algo,
		if(message->head == OK){
			log_info(log,"Recibiendo atributos..");
			void* content = message->content;
			memcpy(&size,content,sizeof(uint32_t));
			content+=sizeof(uint32_t);

			memcpy(&creation_date,content,sizeof(uint64_t));
			content+=sizeof(uint64_t);

			memcpy(&modification_date,content, sizeof(uint64_t));
			content+=sizeof(uint64_t);

			memcpy(&status,content,sizeof(uint8_t));
			content+=sizeof(uint8_t);

			memcpy(&hardlinks,content,sizeof(uint32_t));
			content+=sizeof(uint32_t);

			log_info(log,"%s | %d | %i | %i | %i | %i",path,size,
					creation_date,modification_date,hardlinks,status);

			st->st_nlink = hardlinks;
			st->st_mtim.tv_sec = modification_date;
			st->st_ctim.tv_sec = creation_date;
			st->st_atim.tv_sec = time(NULL);

			if(status == T_DIR){
				st->st_mode = S_IFDIR | 0755;
			} else{
				st->st_size = size;
				st->st_mode = S_IFREG | 0777;
			}
		}else{
			res = get_status(message);
		}
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}

	return res;
}

static int do_readLink(const char *path, char *buf, size_t len){
	log_info(log, "[READ LINK]: Ejecutando do_readLink...");
	log_info(log, "[READ LINK]: ReadLink de %s\n [buf: %s]", path, buf);
	log_info(log, "[READ LINK]: Enviando operacion a SAC-server");
	int res=0;
	int op_res = send_message(sock, READ_LINK, path,
			strlen(path));

	if (op_res >= 0) {
		log_info(log, "[READ LINK]: Comunicacion exitosa con SAC-server");
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[READ LINK]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log, "[READ LINK]]: do_read finalizado");
	return res;
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	log_info(log, "[CREATE]: Ejecutando do_create...");
	log_info(log, "[CREATE]: Create de %s", path);
	log_info(log, "[CREATE]: Enviando operacion a SAC-server");
	int res=0;
	int op_res = send_message(sock, CREATE, path,
			strlen(path));

	if (op_res >= 0) {
		log_info(log, "[CREATE]: Comunicacion exitosa con SAC-server");
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[READ]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log, "[READ]: do_read finalizado");
	return res;
}
//ENAMETOOLONG
static int do_read(const char *path, char *buf, size_t size, off_t off,
		struct fuse_file_info *fi){
	log_info(log, "[READ]: Ejecutando do_read...");
	log_info(log, "[READ]: Read de %s\n", path); // no sé cómo chota imprimir size_t y off_t. Probe con varias cosas de stackoverflow y rompe todo.
	size_t len = strlen(path);
	size_t size_cont = sizeof(size_t) + len + sizeof(off) + sizeof(size);

	void * cont = malloc(size_cont);
	void*aux = cont;
	memcpy(aux,&len,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,path,strlen(path));
	aux+=strlen(path);
	memcpy(aux,&size,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,&off,sizeof(off_t));
	int res=0;

	log_info(log, "[READ]: Enviando operacion a SAC-server");
	int op_res = send_message(sock, READ, cont,size_cont);
	free(cont);
	if(op_res >=0){
		t_message* message = recv_message(sock);
		if(message->head == OK){
			log_info(log, "[READ]: Comunicacion exitosa con SAC-server");
			log_info(log,"[READ] Leido: %s - size: %i",message->content,message->size);
			memcpy(buf, message->content , message->size);
			res = message->size;
		}else{
			res = get_status(message);
			if(res == -1){
				memcpy(buf,"\0",1);
				res = 1;
			}
		}
		free_t_message(message);
	}else{
		log_error(log, "[READ]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log, "[READ]: do_read finalizado");
	return res;
}

static int do_unlink(const char *path) {
	log_info(log, "[UNLINK]: Ejecutando do_unlink...");
	log_info(log, "[UNLINK]: Unlink de %s", path);
	log_info(log, "[UNLINK]: Enviando operacion a SAC-server");
	int res = send_message(sock, UNLINK, path,
			strlen(path));
	if (res >= 0) {
		log_info(log, "[UNLINK]: Comunicacion exitosa con SAC-server");
		t_message *message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[UNLINK]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log, "[UNLINK]: do_mkdir finalizado");
	return res;
}

static int do_mkdir(const char *path, mode_t mode) {
	log_info(log, "[MKDIR]: Ejecutando do_mkdir...");
	log_info(log, "[MKDIR]: Mkdir de %s", path);
	log_info(log, "[MKDIR]: Enviando operacion a SAC-server");
	if(strlen(path) > 70){
		return -ENAMETOOLONG;
	}
	int res=0;
	int op_res = send_message(sock, MKDIR, path,
			strlen(path));

	if (op_res >= 0) {
		log_info(log, "[MKDIR]: Comunicacion exitosa con SAC-server");
		t_message *message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[MKDIR]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log, "[MKDIR]: do_mkdir finalizado");
	return res;
}

static int do_opendir(const char *path, struct fuse_file_info *fi) {
//	int op_res = send_message(sock, OPENDIR, path,
//			strlen(path));
//	int res=0;
//	if (op_res >= 0) {
//		t_message *message = recv_message(sock);
//		res = get_status(message);
//		free_t_message(message);
//	}else{
//		sock = connect_to_server("127.0.0.1", 8080, NULL);
//	}
	return 0;
}

static int do_rmdir(const char *path) {
	log_info(log, "[RMDIR]: Ejecutando do_rmdir...");
	log_info(log, "[RMDIR]: RMDIR de %s", path);
	log_info(log,"[RMDIR]: Enviando operacion a SAC-server");
	int op_res = send_message(sock, RMDIR, path,
			strlen(path));
	int res = 0;
	if (op_res >= 0) {
		log_info(log, "[RMDIR]: Comunicacion exitosa con SAC-server");
		t_message *message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[RMDIR]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log, "[RMDIR]: do_rmdir finalizado");
	return res;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t off, struct fuse_file_info *fi) {
	log_info(log, "[READDIR]: Ejecutando do_readdir...");
	log_info(log, "[READDIR]: Readdir de %s", path);
	log_info(log,"[READDIR]: Enviando operacion a SAC-server");
	int op_res = send_message(sock, READDIR, path,
			strlen(path));
	int res=0;
	if (op_res >= 0) {
		t_message * message = recv_message(sock);
		t_header header = message->head;

		if(header != ERROR){
			log_info(log, "[READDIR]: Comunicacion exitosa con SAC-server");
			filler(buf, ".", NULL, 0);
			filler(buf, "..", NULL, 0);
			while(header == DIR_NAME){
				filler(buf,message->content,NULL,0);
				free_t_message(message);
				message = recv_message(sock);
				header = message->head;
			}
		}
		res= get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[READDIR]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log,"[READDIR]: do_readdir finalizado");
	return res;

}

static int do_access(const char* path, int mask){
	return 0;
}

static int do_mknod(const char *path, mode_t mode, dev_t rdev) {
	log_info(log, "[MKNOD]: Ejecutando do_mknod...");
	log_info(log, "[MKNOD]: Mknod de: %s", path);
	log_info(log,"[MKNOD]: Enviando operacion a SAC-server");
	if(strlen(path) > 70){
		return -ENAMETOOLONG;
	}
	int op_res = send_message(sock, MKNODE, path,
			strlen(path));
	int res=0;
	if (op_res >= 0) {
		log_info(log, "[MKNOD]: Comunicacion exitosa con SAC-server");
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[MKNOD]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}

	log_info(log,"[MKNOD]: do_mknod finalizado");
	return res;
}

static int do_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	log_info(log, "[WRITE]: Ejecutando do_write...");

	log_info(log, "[WRITE]: Write de: %s\n [buf: %s]", path, buf);//[size: %lu] [offset: %lu]", path, (unsigned long)size, (unsigned long)off);

	size_t len = strlen(path);

	size_t size_cont = sizeof(size_t) + len + sizeof(off_t) + sizeof(size_t);


	void* cont = malloc(size_cont);
	void* aux = cont;
	memcpy(aux,&len,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,path,len);
	aux+=strlen(path);
	memcpy(aux,&size,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,&off,sizeof(off_t));
	aux+=sizeof(off_t);
	log_info(log,"[WRITE]: Enviando operacion a SAC-server");
	int op_res = send_message(sock,WRITE,cont,size_cont);
	op_res = send_message(sock,OK,buf,size);
	int res=0;
	free(cont);
	if(op_res >= 0){

		t_message* message = recv_message(sock);
		if(message->head == ERROR)
			res = get_status(message);
		else{
			log_info(log, "[WRITE]: Comunicacion exitosa con SAC-server");
			res = size;
		}
		free_t_message(message);
	}else{
		log_error(log, "[WRITE]: Comunicacion fallida con SAC-Server");
		sock =connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log,"[WRITE]: do_write finalizado");
	return res;
}

static int do_setxattr(const char *path, const char *name,
                    const void *value, size_t size, int flags){
	return 0;
}



static int do_utimens(const char* path, const struct timespec ts[2]){
	log_info(log,"[UTIMENS]: Ejecutando do_utimens...");
	log_info(log,"[UTIMENS]: Utimens de: %s", path);
	size_t len = strlen(path);
	uint64_t last_mod = ts[1].tv_sec;
	size_t size_cont = sizeof(size_t) + len + sizeof(uint64_t);
	void * cont = malloc(size_cont);
	void*aux = cont;
	memcpy(aux,&len,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,path,len);
	aux+=strlen(path);
	memcpy(aux,&last_mod,sizeof(uint64_t));
	log_info(log,"[UTIMENS]: Enviando operacion a SAC-server");
	int op_res = send_message(sock, UTIME, cont,
			size_cont);
	int res=0;
	if (op_res >= 0) {
		log_info(log, "[UTIMENS]: Comunicacion exitosa con SAC-server");
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[UTIMENS]: Comunicacion fallida con SAC-server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log,"[UTIMENS]: do_utimens finalizado");
	return res;
}

static int do_trucate(const char *filename, off_t offset){
	log_info(log, "[TRUNCATE]: Ejecutando do_truncate...");
	log_info(log,"[TRUNCATE]: Truncate de %s",filename); // No tiene sentido logear el offset si siempre da cero.,
	size_t len = strlen(filename);
	size_t size_cont = sizeof(size_t) + len + sizeof(off_t);
	void * cont = malloc(size_cont);
	void*aux = cont;
	memcpy(aux,&len,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,filename,len);
	aux+=strlen(filename);
	memcpy(aux,&offset,sizeof(off_t));
	log_info(log,"[TRUNCATE]: Enviando operacion a SAC-server");
	int op_res = send_message(sock, TRUNCATE, cont, size_cont);
	int res=0;
	if (op_res >= 0) {
		log_info(log, "[TRUNCATE]: Comunicacion exitosa con SAC-server");
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[TRUNCATE]: Comunicacion fallida con SAC-Server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log,"[TRUNCATE]: do_truncate finalizado");
	return 0;
}

static int do_rename(const char* old_path,const char* new_path){
	log_info(log, "[RENAME]: Ejecutando do_rename...");
	log_info(log, "OLD PATH [%s] - NEW PATH [%s]",old_path,new_path);
	if(strlen(new_path) > 70){
		return -ENAMETOOLONG;
	}
	size_t len_old = strlen(old_path);
	size_t len_new = strlen(new_path);
	size_t size_cont = sizeof(size_t) + sizeof(size_t) + len_old + sizeof(off_t) + len_new;
	void * cont = malloc(size_cont);
	void*aux = cont;
	memcpy(aux,&len_old,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,old_path,len_old);
	aux+=len_old;
	memcpy(aux,&len_new,sizeof(size_t));
	aux+=sizeof(size_t);
	memcpy(aux,new_path,len_new);
	int op_res = send_message(sock, RENAME, cont, size_cont);
	int res=0;
	if (op_res >= 0) {
		log_info(log, "[RENAME]: Comunicacion exitosa con SAC-server");
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		log_error(log, "[RENAME]: Comunicacion fallida con SAC-Server");
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log,"[RENAME]: do_rename finalizado");
	return 0;
}

static int do_release(const char *x, struct fuse_file_info *y){
	return 0;
}

/*
 * Estructura principal de FUSE
 */
static struct fuse_operations do_operations = {
		.getattr = do_getattr,
		.readlink = do_readLink,
		.create = do_create,
		.read = do_read,
		.unlink = do_unlink,
		.mkdir = do_mkdir,
		.opendir = do_opendir,
		.rmdir = do_rmdir,
		.readdir = do_readdir,
		.mknod = do_mknod,
		.write= do_write,
		.utimens = do_utimens,
		.truncate = do_trucate,
		.access = do_access,
		.rename = do_rename,
		.setxattr = do_setxattr,
		.release = do_release
};

enum {
	KEY_VERSION,
	KEY_HELP,
};

/*
 * Esta estructura es utilizada para decirle a la biblioteca de FUSE que
 * parametro puede recibir y donde tiene que guardar el valor de estos
 */
static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

//Main. Falta entender cómo es que el main no termina y recibe las funciones de FUSE.
//Falta entender dónde se crearían los hilos para las distintas requests de FUSE e implementarlo.

int main(int argc, char* argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	log = log_create("sac.log","SAC",1,LOG_LEVEL_INFO);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	sock = connect_to_server("192.168.3.35", 8003, NULL); // Se establece la conexión
	return fuse_main(args.argc, args.argv, &do_operations, NULL);
}
