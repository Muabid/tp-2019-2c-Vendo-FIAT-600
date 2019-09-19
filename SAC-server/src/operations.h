/*
 * operations.h
 *
 *  Created on: 16 sep. 2019
 *      Author: utnso
 */

#ifndef OPERATIONS_H_
#define OPERATIONS_H_
#include "nodes.h"
#include <commons/collections/list.h>

int getattr(const char* path);
int get_subdirectories(int node);
int get_number_links(GFile node,int index);

#endif /* OPERATIONS_H_ */
