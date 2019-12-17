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
int min(int x, int y);
int max(int x, int y);
void log_function_init(t_log* log,const char* function);
void log_function_finish(t_log* log, const char* function);
#endif /* UTILS_H_ */
