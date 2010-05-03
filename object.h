#ifndef __OBJECT_H__
#define __OBJECT_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#include "material.h"
#include "controller.h"
#include "vector.h"
#include "quat.h"
#include "matrix.h"

#define OBJECT_VERTEX_TYPE (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

typedef struct{
	vector vertex;
	vector normal;
	float u,v;
}vertex;

typedef struct{
	material *mat;
	unsigned int triangle_count;
	unsigned short *triangles;
	IDirect3DIndexBuffer9 *indexbuffer;
}submesh;

typedef struct{
	BOOL update;

	matrix mat;
	prs prs;

	unsigned int morphtarget_count;
	vector *morphtarget_vertices;

	unsigned int vertex_count;
	vertex *vertices;

	IDirect3DVertexBuffer9 *vertexbuffer;

	unsigned int submesh_count;
	submesh *submeshes;

}object;

void morph_object(object *obj, float t);

object *load_object(IDirect3DDevice9 *device, char *filename );
void free_object(object *obj);
void draw_object(IDirect3DDevice9 *device, object *obj);
void draw_object_ex(IDirect3DDevice9 *device, object *obj, BOOL trans);

#endif /* __OBJECT_H__ */