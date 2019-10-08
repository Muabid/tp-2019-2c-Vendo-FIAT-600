/*
 * operations.h
 *
 *  Created on: 16 sep. 2019
 *      Author: utnso
 */

#ifndef OPERATIONS_H_
#define OPERATIONS_H_
#include "nodes.h"
#include <string.h>
#include <shared/protocol.h>
#include <commons/string.h>
#include <commons/collections/list.h>

int getattr(int sock,const char* path);
int mknod(char* path);
int get_subdirectories(int node);
int get_number_links(GFile node,int index);

#endif /* OPERATIONS_H_ */
