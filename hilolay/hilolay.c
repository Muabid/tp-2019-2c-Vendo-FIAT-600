#include "hilolay.h"


int _suse_init(char* ip, int puerto) {
	int conectadoAlSocket = connect_to_server(ipServidor, puertoServidor, NULL);
	printf("Se conectó al server\n");
	return conectadoAlSocket;
}
