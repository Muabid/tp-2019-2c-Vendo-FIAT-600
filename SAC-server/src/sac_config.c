/*
 * sac_config.c
 *
 *  Created on: 13 sep. 2019
 *      Author: utnso
 */
#include "sac_config.h"

void sac_load_config(const char* path){
	t_config* config = config_create(path);
	sac_config = malloc(sizeof(t_sac_config));
	sac_config->listen_port = config_get_int_value(config,"LISTEN_PORT");
	char* aux = strdup(config_get_string_value(config,"FILE_SYSTEM_PATH"));
	sac_config->file_system_path = malloc(strlen(aux) +1);
	strcpy(sac_config->file_system_path,aux);
	sac_config->file_system_path[strlen(aux)] = '\0';
	free(aux);
	config_destroy(config);
}

