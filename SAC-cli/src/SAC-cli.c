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
	int res=0;
	int op_res = send_message(sock, READ_LINK, path,
			strlen(path));

	if (op_res >= 0) {
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}

	return res;
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	int res=0;
	int op_res = send_message(sock, CREATE, path,
			strlen(path));

	if (op_res >= 0) {
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	return res;
}
//ENAMETOOLONG
static int do_read(const char *path, char *buf, size_t size, off_t off,
		struct fuse_file_info *fi){
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
	int op_res = send_message(sock, READ, cont,size_cont);
	free(cont);
	if(op_res >=0){
		t_message* message = recv_message(sock);
		if(message->head == OK){
			log_info(log,"Leido: %s - size: %i",message->content,message->size);
			memcpy(buf, message->content , message->size);
			res = message->size;
		}else{
			res = get_status(message);
			if(res == -1){
				strcpy(buf,"");
				res = size;
			}
		}
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	return res;
}

static int do_unlink(const char *path) {
	int res = send_message(sock, UNLINK, path,
			strlen(path));
	if (res >= 0) {
		t_message *message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	return res;
}

static int do_mkdir(const char *path, mode_t mode) {
	int res=0;
	int op_res = send_message(sock, MKDIR, path,
			strlen(path));

	if (op_res >= 0) {
		t_message *message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}

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
	int op_res = send_message(sock, RMDIR, path,
			strlen(path));
	int res = 0;
	if (op_res >= 0) {
		t_message *message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}

	return res;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t off, struct fuse_file_info *fi) {

	int op_res = send_message(sock, READDIR, path,
			strlen(path));
	int res=0;
	if (op_res >= 0) {
		t_message * message = recv_message(sock);
		t_header header = message->head;

		if(header != ERROR){
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
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	return res;

}

static int do_access(const char* path, int mask){
	return 0;
}

static int do_mknod(const char *path, mode_t mode, dev_t rdev) {
	int op_res = send_message(sock, MKNODE, path,
			strlen(path));
	int res=0;
	if (op_res >= 0) {
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}

	return res;
}

static int do_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
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
	int op_res = send_message(sock,WRITE,cont,size_cont);
	op_res = send_message(sock,OK,buf,size);
	int res=0;
	free(cont);
	if(op_res >= 0){
		t_message* message = recv_message(sock);
		if(message->head == ERROR)
			res = get_status(message);
		else{
			res = size;
		}
		free_t_message(message);
	}else{
		sock =connect_to_server("127.0.0.1", 8080, NULL);
	}
	return res;
}

static int do_setxattr(const char *path, const char *name,
                    const void *value, size_t size, int flags){
	return 0;
}

static int do_utimens(const char* path, const struct timespec ts[2]){
	log_info(log,"Executing do_utimens...");
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
	int op_res = send_message(sock, UTIME, cont,
			size_cont);
	int res=0;
	if (op_res >= 0) {
		t_message* message = recv_message(sock);
		res = get_status(message);
		free_t_message(message);
	}else{
		sock = connect_to_server("127.0.0.1", 8080, NULL);
	}
	log_info(log,"Finishing do_utimens...");
	return res;
}

static int do_trucate(const char *filename, off_t offset){
	log_info(log,"TRUNCATE");
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
		.access = do_access
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

	sock = connect_to_server("127.0.0.1", 8080, NULL); // Se establece la conexión
	return fuse_main(args.argc, args.argv, &do_operations, NULL);
}
