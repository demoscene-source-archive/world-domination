#include "metaballs.h"

extern float iso_values[GRIDSIZE][GRIDSIZE][GRIDSIZE];
extern vector iso_positions[GRIDSIZE][GRIDSIZE][GRIDSIZE];
extern vector iso_normals[GRIDSIZE][GRIDSIZE][GRIDSIZE];
extern cell iso_cells[GRIDSIZE-1][GRIDSIZE-1][GRIDSIZE-1];

void fill_metafield(metaball *balls, const int ball_count){
	int x, y, z, i;

	memset(iso_values,0,sizeof(float)*GRIDSIZE*GRIDSIZE*GRIDSIZE);
	memset(iso_normals,0,sizeof(vector)*GRIDSIZE*GRIDSIZE*GRIDSIZE);

	for(i=0;i<ball_count;i++){

		float temp;
		int xstart, xend;
		int ystart, yend;
		int zstart, zend;
		float start;

		balls[i].rr = balls[i].r*balls[i].r;
		balls[i].irr = 1.f/balls[i].rr;

		start = (0.707f*balls[i].r)*GRIDSIZE;

		temp = (balls[i].pos.x*GRIDSIZE)+(GRIDSIZE/2);
		xstart = (int)ceil(temp-start);
		xend = (int)floor(temp+start);
		if(xstart<0) xstart = 0;
		if(xend>GRIDSIZE) xend = GRIDSIZE;

		temp = (balls[i].pos.y*GRIDSIZE)+(GRIDSIZE/2);
		ystart = (int)ceil(temp-start);
		yend = (int)floor(temp+start);
		if(ystart<0) ystart = 0;
		if(yend>GRIDSIZE) yend = GRIDSIZE;

		temp = (balls[i].pos.z*GRIDSIZE)+(GRIDSIZE/2);
		zstart = (int)ceil(temp-start);
		zend = (int)floor(temp+start);
		if(zstart<0) zstart = 0;
		if(yend>GRIDSIZE) yend = GRIDSIZE;


		for(z=zstart;z<zend;z++){
			for(y=ystart;y<yend;y++){
				for(x=xstart;x<xend;x++){
					const vector dir = vector_sub(balls[i].pos,iso_positions[x][y][z]);
					const float rr = ((dir.x*dir.x)+(dir.y*dir.y)+(dir.z*dir.z))*balls[i].irr;
					if(rr<0.5f){
						const float f = (1.f-(rr-rr*rr)*4)*2;
						iso_normals[x][y][z] = vector_add(iso_normals[x][y][z],vector_scale(dir,f));
						iso_values[x][y][z] += f;
					}
				}
			}
		}
	}
}

void fill_metafield_blur(metaball *balls, const int ball_count, const float blur){
	int x, y, z, i;
	vector *n = (vector*)iso_normals;
	float *f = (float*)iso_values;

	for(i=GRIDSIZE*GRIDSIZE*GRIDSIZE;i;i--){
		*n++ = vector_scale(*n,blur);
	}

	for(i=GRIDSIZE*GRIDSIZE*GRIDSIZE;i;i--){
		*f++ = *f*blur;
	}

	for(i=0;i<ball_count;i++){
		float temp;
		int xstart, xend;
		int ystart, yend;
		int zstart, zend;
		float start;

		balls[i].rr = balls[i].r*balls[i].r;
		balls[i].irr = 1.f/balls[i].rr;

		start = (0.707f*balls[i].r)*GRIDSIZE;

		temp = (balls[i].pos.x*GRIDSIZE)+(GRIDSIZE/2);
		xstart = (int)ceil(temp-start);
		xend = (int)floor(temp+start);
		if(xstart<0) xstart = 0;
		if(xstart>GRIDSIZE) return;
		if(xend>GRIDSIZE) xend = GRIDSIZE;
		if(xend<0) return;

		temp = (balls[i].pos.y*GRIDSIZE)+(GRIDSIZE/2);
		ystart = (int)ceil(temp-start);
		yend = (int)floor(temp+start);
		if(ystart<0) ystart = 0;
		if(ystart>GRIDSIZE) return;
		if(yend>GRIDSIZE) yend = GRIDSIZE;
		if(yend<0) return;

		temp = (balls[i].pos.z*GRIDSIZE)+(GRIDSIZE/2);
		zstart = (int)ceil(temp-start);
		zend = (int)floor(temp+start);
		if(zstart<0) zstart = 0;
		if(zstart>GRIDSIZE) return;
		if(zend>GRIDSIZE) zend = GRIDSIZE;
		if(zend<0) return;


		for(z=zstart;z<zend;z++){
			for(y=ystart;y<yend;y++){
				for(x=xstart;x<xend;x++){
					const vector dir = vector_sub(balls[i].pos,iso_positions[x][y][z]);
					const float rr = ((dir.x*dir.x)+(dir.y*dir.y)+(dir.z*dir.z))*balls[i].irr;
					if(rr<0.5f){
						const float f = (1.f-(rr-rr*rr)*4)*2;
						iso_normals[x][y][z] = vector_add(iso_normals[x][y][z],vector_scale(dir,f));
						iso_values[x][y][z] += f;
					}
				}
			}
		}
	}
}
