/*
 * sac_config.c
 *
 *  Created on: 13 sep. 2019
 *      Author: utnso
 */
#include "sac_config.h"

sac_config* sac_load_config(char* path){
	t_config* config = config_create(path);
	sac_config* sac_config = malloc(sizeof(sac_config));
	sac_config->listen_port = config_get_int_value(config,"LISTEN_PORT");
	config_destroy(config);
	return sac_config;
}
