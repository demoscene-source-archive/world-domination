#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

#include "file.h"

typedef struct{
	IDirect3DTexture9 *texture;
	char *filename;
}texture_node;

texture_node *textures = NULL;
int textures_allocated = 0;
int textures_used = 0;

void check_alloc(){
	if(!textures){
		textures_allocated = 16;
		textures = malloc(sizeof(texture_node)*textures_allocated);
	}
	if(textures_used>=textures_allocated){
		textures_allocated *= 2;
		textures = realloc(textures,sizeof(texture_node)*textures_allocated);
	}
}

int texture_load(IDirect3DDevice9 *device, char *filename, BOOL fullscreen){
	int i;
	file *fp;

	if( strcmp(filename,"")==0 ) return -1;

	for(i=0;i<textures_used;i++){
		if(strcmp(filename,textures[i].filename)==0){
			return i;
		}
	}

	check_alloc();

	fp = file_open(filename);
	if(!fp) return -1;

	if(!fullscreen){
		if(FAILED(D3DXCreateTextureFromFileInMemory(device,fp->data,fp->size,&textures[textures_used].texture))){
			file_close(fp);
			return -1;
		}
	}else{
		if(FAILED(D3DXCreateTextureFromFileInMemoryEx(device,fp->data,fp->size,
			640,480,
			1,0,D3DFMT_UNKNOWN,D3DPOOL_DEFAULT,D3DX_DEFAULT,D3DX_DEFAULT,0,NULL,NULL,
			&textures[textures_used].texture)))
		{
			file_close(fp);
			return -1;
		}
	}

	file_close(fp);
	textures[textures_used].filename = strdup(filename);
	textures_used++;
	return (textures_used-1);
}

int texture_insert(IDirect3DDevice9 *device, char *filename, IDirect3DTexture9* texture){
	int i;

	if( strcmp(filename,"")==0 ) return -1;

	for(i=0;i<textures_used;i++){
		if(strcmp(filename,textures[i].filename)==0){
			return i;
		}
	}

	check_alloc();

	textures[textures_used].filename = strdup(filename);
	textures[textures_used].texture = texture;
	textures_used++;
	return (textures_used-1);
}

void set_texture(IDirect3DDevice9 *device, int texture_unit, int texture){
	if(texture<0){
		if(texture==-1){
			IDirect3DDevice9_SetTexture(device, texture_unit, NULL);
			IDirect3DDevice9_SetTextureStageState( device, texture_unit, D3DTSS_COLOROP, D3DTOP_DISABLE );
			IDirect3DDevice9_SetTextureStageState( device, texture_unit, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		}
		return;
	}
	if(texture<=textures_used){
		IDirect3DDevice9_SetTexture(device, texture_unit, (IDirect3DBaseTexture9*)textures[texture].texture);
		IDirect3DDevice9_SetTextureStageState(device, texture_unit, D3DTSS_COLOROP, D3DTOP_MODULATE);
		IDirect3DDevice9_SetTextureStageState(device, texture_unit, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
//		IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	}
}

void free_textures(){
	int i;
	for(i=0;i<textures_used;i++){
		IDirect3DTexture9_Release(textures[i].texture);
		free(textures[i].filename);
	}
	free(textures);
	textures = NULL;
}