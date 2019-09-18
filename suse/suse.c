#include "suse.h"

int listen_port = 10500;//int listen_port;
//int metrics_timer;
//int max_multiprog;
//char* sem_ids;
//int sem_init[];
//int sem_max[];
//float alpha_sjf;

//void cargarConfiguracion() {
//	t_config* archivoConfiguracion = config_create("/suse.config");
//
//	listen_port = config_get_int_value(archivoConfiguracion, "LISTEN_PORT");
//	metrics_timer = config_get_int_value(archivoConfiguracion, "METRICS_TIMER");
//	max_multiprog = config_get_int_value(archivoConfiguracion, "MAX_MULTIPROG");
//	sem_ids = config_get_string_value(archivoConfiguracion, "SEM_IDS");
//	sem_init = config_get_array_value(archivoConfiguracion, "SEM_INIT");
//	sem_max = config_get_array_value(archivoConfiguracion, "SEM_MAX");
//	alpha_sjf = config_get_int_value(archivoConfiguracion, "ALPHA_SJF");
//
//	config_destroy(archivoConfiguracion);
//}

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[2000];

    //Send some messages to the client
    message = "Greetings! I am your connection handler\n";
    write(sock , message , strlen(message));

    message = "Now type something and i shall repeat what you type \n";
    write(sock , message , strlen(message));

    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
        //end of string marker
		client_message[read_size] = '\0';

		//Send the message back to client
        write(sock , client_message , strlen(client_message));

		//clear the message buffer
		memset(client_message, 0, 2000);
    }

    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    return 0;
}

int main() {
	//cargarConfiguracion();
	int socketCliente;
	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion;
	pthread_t threadId;

	int servidor = iniciarServidor(listen_port);

	while((socketCliente = accept(servidor, (void*) &direccionCliente, &tamanioDireccion)))
	    {
	        printf("Connection accepted");

	        if( pthread_create( &threadId , NULL ,  connection_handler , (void*) &socketCliente) < 0)
	        {
	            perror("could not create thread");
	            return 1;
	        }

	        //Now join the thread , so that we dont terminate before the thread
	        //pthread_join( thread_id , NULL);
	        puts("Handler assigned");
	    }

	    if (socketCliente < 0)
	    {
	        perror("accept failed");
	        return 1;
	    }



	close(socket);


}
