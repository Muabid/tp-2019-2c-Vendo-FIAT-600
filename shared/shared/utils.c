/*
 * utils.c
 *
 *  Created on: 26 sep. 2019
 *      Author: utnso
 */
#include "utils.h"

char* get_date(){
	char* text= malloc(9);
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(text,9,"%Y%m%d", t);
	text[8] = '\0';
	return text;

}

int fsize(char* path){
	struct stat st;
	stat(path, &st);
	return st.st_size;
}

void log_error_code(t_log* log, int error_code){
	if(error_code <0){
		error_code = -error_code;
	}
	char*error = strerror(error_code);
	log_error(log,error);
	free(error);
}

