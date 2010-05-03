#ifndef __TEXTUREMANAGER_H__
#define __TEXTUREMANAGER_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

int texture_load(IDirect3DDevice9 *device, char *filename, BOOL fullscreen);
int texture_insert(IDirect3DDevice9 *device, char *filename, IDirect3DTexture9* texture);

void set_texture(IDirect3DDevice9 *device,int texture_unit,int texture);
void free_textures();

#endif /* __TEXTUREMANAGER_H__ */