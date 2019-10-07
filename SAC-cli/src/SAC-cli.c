/*
 * SAC-cli.c
 *
 *  Created on: 28 sep. 2019
 *      Author: utnso
 */

#include "protocol.h"
#include "operations_client.h"
#include "net.h"

//Variable global para la conexión con el SAC-server
int socket;


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
	void *buffer = malloc(strlen(path));
	memcpy(buffer, path, strlen(path));

	int operation_response = send_message(socket, GET_ATTR, buffer,
			strlen(path));

	free(buffer);

	if (operation_response >= 0) {
		t_message* message = recv_message(socket); // Acá me va a responder el server algo,

		return 0; // 0 for success
	} else {
		//Falló. Hacer algo.
		return 1;
	}

}

static int do_readLink(const char *path, char *buf, size_t len) {
	void *buffer = malloc(strlen(path) + sizeof(buf));
	memcpy(buffer, path, strlen(path));
	memcpy(buffer + strlen(path), buf, sizeof(buf));

	int operation_response = send_message(socket, READ_LINK, buffer,
			sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message* message = recv_message(socket);

		return 0;

	} else {
		return 1;
	}
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	void *buffer = malloc(strlen(path) + sizeof(mode));
	memcpy(buffer, path, strlen(path));
	memcpy(buffer + strlen(path), mode, sizeof(mode));

	int operation_response = send_message(socket, CREATE, buffer,
			sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message *message = recv_message(socket);
		return 0;
	} else {
		return 1;
	}
}

static int do_read(const char *path, char *buf, size_t size, off_t off,
		struct fuese_file_info *fi) {
	void *buffer = malloc(strlen(path) + sizeof(buf) + sizeof(size));
	memcpy(buffer, path, strlen(path));
	memcpy(buffer + strlen(path), buf, sizeof(buf));
	memcpy(buffer + strlen(path) + sizeof(buf), size, sizeof(size));

	int operation_response = send_message(socket, READ, buffer, sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message *message = recv_message(socket);
		return 0;
	} else {
		return 1;
	}
}

static int do_unlink(const char *path) {
	void *buffer = malloc(strlen(path));
	memcpy(buffer, path, strlen(path));

	int operation_response = send_message(socket, UNLINK, buffer,
			sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message *message = recv_message(socket);
		return 0;
	} else {
		return 1;
	}
}

static int do_mkdir(const char *path, mode_t mode) {
	void *buffer = malloc(strlen(path) + sizeof(mode));
	memcpy(buffer, path, strlen(path));

	int operation_response = send_message(socket, MKDIR, buffer,
			sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message *message = recv_message(socket);
		return 0;
	} else {
		return 1;
	}
}

static int do_opendir(const char *path, struct fuse_file_info *fi) {
	void *buffer = malloc(strlen(path));
	memcpy(buffer, path, strlen(path));

	int operation_response = send_message(socket, OPENDIR, buffer,
			sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message *message = recv_message(socket);
		return 0;
	} else {
		return 1;
	}
}

static int do_rmdir(const char *path) {
	void *buffer = malloc(strlen(path));
	memcpy(buffer, path, strlen(path));

	int operation_response = send_message(socket, RMDIR, buffer,
			sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message *message = recv_message(socket);
		return 0;
	} else {
		return 1;
	}
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t off, struct fuse_file_info *fi) {

	void *buffer = malloc(strlen(path) + sizeof(buf));
	memcpy(buffer, path, strlen(path));
	memcpy(buffer + strlen(path), buf, sizeof(buf));

	int operation_response = send_message(socket, READDIR, buffer,
			sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message *message = recv_message(socket);
		return 0;
	} else {
		return 1;
	}

}

static int do_mknod(const char *path, mode_t mode, dev_t rdev) {
	void *buffer = malloc(strlen(path) + sizeof(mode));
	memcpy(buffer, path, strlen(path));
	memcpy(buffer + strlen(path), mode, sizeof(mode));

	int operation_response = send_message(socket, MKNODE, buffer,
			sizeof(buffer));

	free(buffer);

	if (operation_response >= 0) {
		t_message *message = recv_message(socket);
		return 0;
	} else {
		return 1;
	}

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
		.mknod = do_mknod
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

int main(int argc, const char* argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
		if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
			/** error parsing options */
			perror("Invalid arguments!");
			return EXIT_FAILURE;
		}

	socket = connect_to_server("LOCALHOST", 8080, NULL); // Se establece la conexión

	int communication_response = send_message(socket, HI_PLEASE_BE_MY_FRIEND,
			"HI");

	if (communication_response >= 0) { // Comunicación ok.
		/* Cuando la comunicación ya se estableció, tengo que hacer una espera activa para las operaciones del FS? o no es necesario?
		 Cómo hago para vincular las operaciones de fuse con el archivo operations_client.c, o tiene que estar todo en el main?
		 */
		while (1) {

		}
	} else {

	}

	return fuse_main(args.argc, args.argv, &do_operations, NULL);

}
