#include "object.h"

#define ENTRY_UNKNOWN	0
#define ENTRY_OBJECT	1
#define ENTRY_CAMERA	2
#define ENTRY_LIGHT		3

typedef struct{
	matrix mat;
	prs prs;

	float fov;
	float near_clip;
	float far_clip;

	BOOL fog;
	color fog_color;
	float fog_start;
	float fog_end;
}camera;

typedef struct{
	prs prs;
	color col;
	float strenght;
}light;

typedef struct{
	unsigned int id;
	unsigned int entry_type;
	void *entry;
}scene_entry;

typedef struct{
	color ambient;
	color background;

	unsigned int object_count;
	object **objects;

	unsigned int camera_count;
	camera *cameras;

	unsigned int light_count;
	light *lights;

	unsigned int entry_count;
	scene_entry *entries;

	float start;
	float end;

	unsigned int controller_count;
	controller **controllers;
}scene;


void set_camera(IDirect3DDevice9 *device, camera *cam);

scene *load_scene(IDirect3DDevice9 *device, char *filename);
void free_scene(scene *input);

void animate_scene(scene *input, float time);
void draw_scene(IDirect3DDevice9 *device, scene *input, int cam, BOOL clear);