#include "suse.h"

int listen_port = 20000;


int main() {
	int socketDelCliente;
	struct sockaddr direccionCliente;
	unsigned int tamanioDireccion = sizeof(direccionCliente);
	int servidor = init_server(listen_port);


	t_message* bufferLoco = NULL;

	if((socketDelCliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion)) >= 0) {
			printf("Se ha aceptado una conexion: %i\n", socketDelCliente);
			bufferLoco = recv_message(socketDelCliente);
			char* contenido = (char *) bufferLoco->content;
			printf("El mensaje dice: %s\n", contenido);

	}
	else {
			printf("Se falló al aceptar la conexion. Error: %i", socketDelCliente);
	}

	free_t_message(bufferLoco);
	close(servidor);
}
