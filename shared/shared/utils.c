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
