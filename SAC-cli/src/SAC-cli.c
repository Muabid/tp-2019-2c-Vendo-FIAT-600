/*
 * SAC-cli.c
 *
 *  Created on: 28 sep. 2019
 *      Author: utnso
 */

#include "protocol.h"
#include "operations_client.h"
#include "net.h"

int socket;

int main(int argc, const char* argv[]) {
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

}




/*
 * Implementaciones de FUSE. Hay que definir si es necesario mandar ciertos parametros en el contenido o son al pedo.
 * Falta definir que se hace con lo recibido (cómo deserializarlo y en qué casos vale la pena), cómo implementar hilos para que sean cosas en paralelo, ver tema de la función main.
 *
 */

static int do_getattr(const char *path, struct stat *st) {
	void *buffer = malloc(sizeof(path));
	memcpy(buffer, path, sizeof(path));

	int operation_response = send_message(socket, GET_ATTR, buffer,
			sizeof(path));

	free(buffer);

	if (operation_response >= 0) {
		t_message* message = recv_message(socket); // Acá me va a responder el server algo,

		return 0; // 0 for success
	} else {
		//Falló. Hacer algo.
		return 1;
	}

}

static int do_readlink(const char *path, char *buf, size_t len) {
	void *buffer = malloc(sizeof(path) + sizeof(buf));
	memcpy(buffer, path, sizeof(path));
	memcpy(buffer + sizeof(path), buf, sizeof(buf));

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
	void *buffer = malloc(sizeof(path) + sizeof(mode));
	memcpy(buffer, path, sizeof(path));
	memcpy(buffer + sizeof(path), mode, sizeof(mode));

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
	void *buffer = malloc(sizeof(path) + sizeof(buf) + sizeof(size));
	memcpy(buffer, path, sizeof(path));
	memcpy(buffer + sizeof(path), buf, sizeof(buf));
	memcpy(buffer + sizeof(path) + sizeof(buf), size, sizeof(size));

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
	void *buffer = malloc(sizeof(path));
	memcpy(buffer, path, sizeof(path));

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
	void *buffer = malloc(sizeof(path) + sizeof(mode));
	memcpy(buffer, path, sizeof(path));

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
	void *buffer = malloc(sizeof(path));
	memcpy(buffer, path, sizeof(path));

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
	void *buffer = malloc(sizeof(path));
	memcpy(buffer, path, sizeof(path));

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

	void *buffer = malloc(sizeof(path) + sizeof(buf));
	memcpy(buffer, path, sizeof(path));
	memcpy(buffer + sizeof(path), buf, sizeof(buf));

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
	void *buffer = malloc(sizeof(path) + sizeof(mode));
	memcpy(buffer, path, sizeof(path));
	memcpy(buffer + sizeof(path), mode, sizeof(mode));

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
