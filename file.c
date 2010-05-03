#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "urarlib.h"


file* file_open( char *filename ){

	file* temp = (file*)malloc( sizeof(file) );

	if( !urarlib_get(&temp->data, &temp->size, filename, ARCHIVE, PASSWORD) ){
		char buffer[256];
		FILE *fp;

#ifdef _DEBUG
		printf("file not found in archive %s\n", filename);
#endif

		sprintf(buffer,"data/%s", filename);
#ifdef _DEBUG
//		printf("trying: %s\n", buffer);
#endif


		fp = fopen(buffer, "rb");
		if(fp==NULL) return NULL;

		fseek(fp, 0, SEEK_END);
		temp->size = ftell(fp);
		rewind(fp);
		temp->data = malloc(sizeof(unsigned char)*temp->size);
		fread( temp->data, sizeof(unsigned char), temp->size, fp );
		fclose(fp);
	}

	temp->pos = 0;
	return temp;
}

int file_close(file* file){
	if(file->data) free(file->data);
	file->data = 0;
	file->size = 0;
	file->pos = 0;
	free(file);
	file = NULL;

	return 0;
}

int file_tell( file *file ){
	return file->pos;
}

size_t file_read( void *buffer, size_t size, size_t number, file *file ){
	unsigned int counter;
	for( counter=0; counter<(unsigned int)number; counter++ ){
		if((file->pos+size)>file->size) return counter;
		memcpy( (char*)buffer+(counter*size), file->data+file->pos, size );
		file->pos += size;
	}
	return number;
}

int file_seek( file *file, __int64 offset, int mode ){
	switch( mode ){
		case SEEK_SET:
			file->pos = (unsigned int)offset;
		break;
		case SEEK_CUR:
			file->pos += (unsigned int)offset;
		break;
		case SEEK_END:
			file->pos = file->size-(unsigned int)offset;
		break;
	}
	return 0;
}

char *file_loadstring(file *file){
	char *temp = strdup((char*)(file->data+file->pos));
	file->pos += strlen(temp)+1;
	return temp;
}