#ifndef __MARCHINGCUBES_H__
#define __MARCHINGCUBES_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#include "vector.h"

#define GRIDSIZE 32
#define MAXFACES 200000

typedef struct{
	float *val[8];
	vector *pos[8];
	vector *norm[8];
	BOOL computed;
	unsigned char cube_index;
}cell;

typedef struct{
	vector pos;
	float r;
	float rr;
	float irr;
}metaball;

BOOL init_marching_cubes(IDirect3DDevice9 *device);
void deinit_marching_cubes();
void march_my_cubes_opt(const metaball *balls, const int ball_count, const float treshold);
void march_my_cubes( const float treshold );
void draw_marched_cubes(IDirect3DDevice9 *device);

#endif /* __MARCHINGCUBES_H__ */