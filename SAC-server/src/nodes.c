/*
 * nodes.c
 *
 *  Created on: 13 sep. 2019
 *      Author: utnso
 */
#include "nodes.h"

GFile create_GFile(unsigned char status,char file_name[71],
		int32_t root,int32_t size,char creation_date[8],
		char modification_date[8],int32_t blocks_ptr[1000]){

	GFile gFile ={
			.status =status,
			.root = root,
			.size = size
	};

	strcpy(gFile.file_name,file_name);
	strcpy(gFile.creation_date,creation_date);
	strcpy(gFile.modification_date,modification_date);
	for(int i=0; i<1000; i++){
		gFile.blocks_ptr[i] = blocks_ptr[i];
	}

	return gFile;
}
///*
// * Creo que al leer el bloque de la tabla de nodos va a venir así.
// * TODO: probar
// */
//GFile read_GFile(void* cachoOfMemory){
//	unsigned char status;
//	char file_name[71];
//	int32_t root,size;
//	char creation_date[8];
//	char modification_date[8];
//	int32_t blocks_ptr[1000];
//
//	memccpy(&status,cachoOfMemory,1);
//	memccpy(file_name,cachoOfMemory+1,71);
//	memccpy(&root,cachoOfMemory+71,sizeof(int32_t));
//	memccpy(&size,cachoOfMemory+4,sizeof(int32_t));
//	memccpy(creation_date,cachoOfMemory+4,8);
//	memccpy(modification_date,cachoOfMemory+8,8);
//	memccpy(blocks_ptr,cachoOfMemory+8,1000 * sizeof(int32_t));
//
//	return create_GFile(status,file_name,root,size,creation_date,modification_date,blocks_ptr);
//}
