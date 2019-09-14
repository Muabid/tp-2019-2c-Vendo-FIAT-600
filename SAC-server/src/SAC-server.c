/*
 ============================================================================
 Name        : SAC-server.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "sac_config.h"
#include "nodes.h"

int main(void) {
	int32_t x[1000];
	for(int i = 0; i<1000; i++){
		x[i] = 1;
	}
	GFile gFile = create_GFile(1,"HOLA",0,4,"20190913","20190913",x);
	printf("%i",gFile.size);

	GFile g = read_GFile((void*)&gFile);
	return gFile.status;
}
