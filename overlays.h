#ifndef __OVERLAYS_H__
#define __OVERLAYS_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

typedef struct{
	float u, v;
	unsigned int color;
}grid_node;

typedef grid_node grid[((640/8)+1)*((480/8)+1)];

BOOL init_overlays(IDirect3DDevice9 *device);
void deinit_overlays();

void draw_radialblur(IDirect3DDevice9 *device, float x, float y, float scale_, float rot_, int texture, BOOL additive);
void draw_overlay(IDirect3DDevice9 *device, int texture, float xoffs, float yoffs, float alpha, BOOL additive);
void draw_grid(IDirect3DDevice9 *device, grid g, int texture, BOOL additive);

#endif /* __OVERLAYS_H__ */