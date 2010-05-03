#include <math.h>
#ifndef M_PI
#define M_PI 3.1415926535897932384626433f
#endif

#include "overlays.h"

/* raytace */
#include "vector.h"
#include "matrix.h"

#define WIDTH 640
#define HEIGHT 480

void grid_flat(grid g, float xoffset, float yoffset){
	int x,y;
	for(y=0;y<(480+8);y+=8){
		for(x=0;x<(640+8);x+=8){
			g->u = x*(1.f/640.f)+xoffset;
			g->v = y*(1.f/480.f)+yoffset;
			g->color = 0xFFFFFFFF;
			g++;
		}
	}
}

void grid_zero(grid g){
	int x,y;
	for(y=0;y<(480+8);y+=8){
		for(x=0;x<(640+8);x+=8){
			g->u = 0;
			g->v = 0;
			g->color = 0xFFFFFFFF;
			g++;
		}
	}
}

void grid_wave(grid g, float xpos, float ypos, float size, float power){
	int x,y;
	for(y=0;y<(480+8);y+=8){
		for(x=0;x<(640+8);x+=8){
			unsigned int col = 0xFF;
			float mul = 1.f;
			float cx = (x-(640/2))*(1.f/640.f)-xpos;
			float cy = (y-(480/2))*(1.f/640.f)-ypos;
			float dist = (float)sqrt(cx*cx+cy*cy)*size;
			if(dist<M_PI){
				mul = (float)(1+cos(dist))*power;
				g->u += cx*mul;
				g->v += cy*mul;
			}
			g++;
		}
	}
}

void grid_add_noice( grid g, float strength ){
	int i;
	for(i=((WIDTH/8)+1)*((HEIGHT/8)+1);i;i--){
		g->u += (float)(rand()%RAND_MAX-(RAND_MAX/2))*(1.f/RAND_MAX)*strength;
		g->v += (float)(rand()%RAND_MAX-(RAND_MAX/2))*(1.f/RAND_MAX)*strength;
		g++;
	}
}

vector direction[((WIDTH/8)+1)*((HEIGHT/8)+1)];

#define TUNNEL_RADIUS 5.f
float tunnel_radius = TUNNEL_RADIUS;
float tunnel_radius_squared = TUNNEL_RADIUS*TUNNEL_RADIUS;

_inline float tunnel_intersect( vector origin, vector direction ){
	float a = (direction.x*direction.x) + (direction.y*direction.y);
	float b = 2*(origin.x*direction.x + origin.y*direction.y);
	float c = (origin.x*origin.x) + (origin.y*origin.y) - tunnel_radius_squared;

	float delta = (b*b) - 4*a*c;

	float t = (-b -(float)sqrt(delta))/(2*a);
	return t;
}

vector plane_normal = {0,1,0};
float plane_distance = 1;
#define sgn(a) ((a>0) ? 1:-1)
// (sgn(direction.y)*30-origin.y)/direction.y;
_inline float plane_intersect( vector origin, vector direction ){
	return (sgn(direction.y)*plane_distance-origin.y)/direction.y;
}

#define SPHERE_RADIUS 1.5f
float sphere_radius = SPHERE_RADIUS;
float sphere_radius_squared = SPHERE_RADIUS*SPHERE_RADIUS;

_inline float sphere_intersect( vector origin, vector direction ){
	float b = direction.x*origin.x + direction.y*origin.y + direction.z*origin.z;
	float c = origin.x*origin.x + origin.y*origin.y + origin.z*origin.z - sphere_radius_squared;

	float d = b * b - c;

	if (d < 0.0f) return -1.0f;

	return -b - (float)sqrt(d);
}

void init_tunnel(){
	int x,y;
	vector* dir = direction;

	for(y=0; y<(HEIGHT/8+1); y++)
		for(x=0; x<(WIDTH/8+1); x++){
			*dir = vector_make(
				((float)x)-((WIDTH/8+1)/2),
				((float)-y)+((HEIGHT/8+1)/2),
				-35.0f );
			vector_normalize( *dir );
			dir++;
		}
}

void render_tunnel( grid g, matrix m ){
	int i;
	vector origin = vector_make(m[12],m[13],m[14]);
	vector *dir = direction;

	for(i=((WIDTH/8)+1)*((HEIGHT/8)+1);i;i--){
		float shade;
		unsigned int color;
		vector direction = matrix_rotatevector( m, *dir++ );

		float t = tunnel_intersect( origin, direction );
		vector intersection = vector_add(origin,vector_scale(direction,t));

		g->u += (float)fabs(atan2(intersection.x, intersection.y)*(1.f/M_PI));
		g->v += (intersection.z*(8.f/256.f));

		shade = 1.06f-(float)fabs((t*1.3f)+0.5f);

//		shade = 1.015f-(float)fabs((t*1.3f)+0.25f);
//		shade = 1.05f-(float)fabs(t);

		if(shade>1) shade=1;
		if(shade<0) shade=0;

		shade = shade*shade;

		{
			unsigned char alpha = (unsigned char)(shade*255.f);
			unsigned int rb, _g;
			rb = g->color &0xFF00FF;
			_g = g->color&0x00FF00;

			rb *= alpha;
			_g *= alpha;

			color = ((rb>>8)&0xFF00FF)|((_g>>8)&0x00FF00);
		}

//		node->shade *= shade;
		g->color = color;
		g++;
	}
}


