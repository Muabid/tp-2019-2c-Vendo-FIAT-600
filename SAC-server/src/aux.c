/*
 * aux.c
 *
 *  Created on: 31 oct. 2019
 *      Author: utnso
 */
#include "aux.h"

void fill_path(char path[71],char * buf,bool contain_len){
	size_t str_len = strlen(buf);
	if(contain_len){
		memcpy(&str_len,buf,sizeof(size_t));
		buf+=sizeof(size_t);
	}
	memset(path, 0, 71);
	memcpy(path,buf,str_len);
	path[strlen(path)]= '\0';
}
