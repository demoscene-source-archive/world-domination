#ifndef __PARTICLE_H__
#define __PARTICLE_H__

#include "vector.h"

typedef struct{
	vector pos;
	float size;
	float alpha;
}particle;

BOOL init_particles(IDirect3DDevice9 *device);
void deinit_particles();

void draw_particles(IDirect3DDevice9 *device, particle *particles, unsigned int particle_count, int texture);

#endif /* __PARTICLE_H__ */