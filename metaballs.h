#ifndef __METABALLS_H__
#define __METABALLS_H__

#include "marchingcubes.h"

void fill_metafield(metaball *balls, const int ball_count);
void fill_metafield_blur(metaball *balls, const int ball_count, const float blur);

#endif /* __METABALLS_H__ */