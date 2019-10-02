#include "suse.h"

int listen_port = 20000;

//void handler(void* socketConectado) {
//	int socket = *(int*)socketConectado;
//	t_message* bufferLoco = recv_message(socket);
//	printf("El mensaje dice: %s\n", (char*) bufferLoco->content);
//	free_t_message(bufferLoco);
//}



void handler(void* socketConectado) {
	int socket = *(int*)socketConectado;
	t_message* bufferLoco;
	while((bufferLoco = recv_message(socket))->head !=0){ //HAY QUE AGREGAR UNA COLA DE ESPERA
		printf("Se recibió un mensaje");

		switch(bufferLoco->head) {
		case SUSE_CREATE:
			//suseCreate();
			break;
		}
		case SUSE_SCHEDULE_NEXT:
			//suseScheduleNext();
			break;

		case SUSE_WAIT:
			//suseWait();
			break;

		case SUSE_SIGNAL:
			//suseSignal();
			break;

		case SUSE_JOIN:
			//suseJoin();
			break;

		otherwise:
			printf("La instruccion no es correcta");
			break;
	}


	free_t_message(bufferLoco);
}

int main() {
	int socketDelCliente;
	struct sockaddr direccionCliente;
	unsigned int tamanioDireccion = sizeof(direccionCliente);
	int servidor = init_server(listen_port);

	//HAY QUE GUARDAR LOS threadId en alguna lista


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
