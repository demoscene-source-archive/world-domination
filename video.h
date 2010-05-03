#ifndef __VIDEO_H__
#define __VIDEO_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d9.h>

#include "file.h"

typedef struct{
	file *fp;
	int handle;
	int width;
	int height;
	int frames;
	int current_frame;
	int start_offset;
	float fps;

	unsigned int *pixeldata;

	IDirect3DTexture9 *texture;

}video;

void video_init();
//video *video_load(char *filename);
video *video_load(IDirect3DDevice9 *device, char *filename);
BOOL video_get_frame(video *vid, unsigned int *pixeldata, float time);

void video_update(video *vid, float time);
void free_video(video *vid);


#endif /* __VIDEO_H__ */