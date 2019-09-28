#include "suse.h"

int listen_port = 20000;

void handler(void* socketConectado) {
	int socket = *(int*)socketConectado;
	t_message* bufferLoco = recv_message(socket);
	printf("El mensaje dice: %s\n", (char*) bufferLoco->content);
	free_t_message(bufferLoco);
}

int main() {
	int socketDelCliente;
	struct sockaddr direccionCliente;
	unsigned int tamanioDireccion = sizeof(direccionCliente);
	int servidor = init_server(listen_port);




	while((socketDelCliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion))) {
		pthread_t threadId;
		printf("Se ha aceptado una conexion: %i\n", socketDelCliente);
		if(pthread_create(&threadId, NULL, handler, (void*) &socketDelCliente) < 0) {
			perror("No se pudo crear el hilo");
			return 1;
		} else {
			printf("Handler asignado\n");
			pthread_join(threadId, NULL);
		}


	}
	if(socketDelCliente < 0) {
		perror("Falló al aceptar conexión");
	}

	close(servidor);
}
