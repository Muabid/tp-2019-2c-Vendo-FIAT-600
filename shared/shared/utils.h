/*
 * utils.h
 *
 *  Created on: 26 sep. 2019
 *      Author: utnso
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <commons/log.h>

char* get_date();
int fsize(char* path);

void log_error_code(t_log* log, int error_code);
#endif /* UTILS_H_ */
