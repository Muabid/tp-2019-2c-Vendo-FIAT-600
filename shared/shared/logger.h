/*
 * log.h
 *
 *  Created on: 5 oct. 2019
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_
#include <commons/log.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

pthread_mutex_t mutexLog;
/**
 * @NAME: log_create
 * @DESC: Crea una instancia de logger, tomando por parametro
 * el nombre del programa, el nombre del archivo donde se van a generar los logs,
 * el nivel de detalle minimo a loguear y si además se muestra por pantalla lo que se loguea.
 */
t_log* log_lock_create(char* file, char *program_name, bool is_active_console,
		t_log_level level);

/**
 * @NAME: log_destroy
 * @DESC: Destruye una instancia de logger
 */
void log_lock_destroy(t_log* logger);

/**
 * @NAME: log_trace
 * @DESC: Loguea un mensaje con el siguiente formato
 *
 * [TRACE] hh:mm:ss:mmmm PROCESS_NAME/(PID:TID): MESSAGE
 *
 */
void log_lock_trace(t_log* logger, const char* message, ...);

/**
 * @NAME: log_debug
 * @DESC: Loguea un mensaje con el siguiente formato
 *
 * [DEBUG] hh:mm:ss:mmmm PROCESS_NAME/(PID:TID): MESSAGE
 *
 */
void log_lock_debug(t_log* logger, const char* message, ...);

/**
 * @NAME: log_info
 * @DESC: Loguea un mensaje con el siguiente formato
 *
 * [INFO] hh:mm:ss:mmmm PROCESS_NAME/(PID:TID): MESSAGE
 *
 */
void log_lock_info(t_log* logger, const char* message, ...);

/**
 * @NAME: log_warning
 * @DESC: Loguea un mensaje con el siguiente formato
 *
 * [WARNING] hh:mm:ss:mmmm PROCESS_NAME/(PID:TID): MESSAGE
 *
 */
void log_lock_warning(t_log* logger, const char* message, ...);

/**
 * @NAME: log_error
 * @DESC: Loguea un mensaje con el siguiente formato
 *
 * [ERROR] hh:mm:ss:mmmm PROCESS_NAME/(PID:TID): MESSAGE
 *
 */
void log_lock_error(t_log* logger, const char* message, ...);

/**
 * @NAME: log_level_as_string
 * @DESC: Convierte un t_log_level a su representacion en string
 */
char *log_lock_level_as_string(t_log_level level);

/**
 * @NAME: log_level_from_string
 * @DESC: Convierte un string a su representacion en t_log_level
 */
t_log_level log_lock_level_from_string(char *level);

#endif /* LOGGER_H_ */
