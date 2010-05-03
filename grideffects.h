#ifndef __GRIDEFFECTS_H__
#define __GRIDEFFECTS_H__

#include "overlays.h"

void grid_flat(grid g, float xoffset, float yoffset);
void grid_zero(grid g);
void grid_wave(grid g, float xpos, float ypos, float size, float power);

void grid_add_noice( grid g, float strength );

void init_tunnel();
void render_tunnel( grid g, matrix m );

#endif /* __GRIDEFFECTS_H__ */
