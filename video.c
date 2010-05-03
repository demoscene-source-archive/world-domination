#include "video.h"
#include "decore/decore.h"
#include <math.h>

#define MAGIC "TURBOMPG"

static int handle = 1;

void video_init(){
	handle = 1;
}

video *video_load(IDirect3DDevice9 *device, char *filename){
	video *temp;
	char magic[8];
	DEC_PARAM dec_param;
	DEC_SET dec_set;
	DEC_MEM_REQS dec_mem_reqs;

	file *fp = file_open(filename);
	if(!fp) return NULL;

	file_read( (void*)&magic, 1, 8, fp );
	if(memcmp(magic, MAGIC, strlen(MAGIC))!=0) return NULL;

	temp = malloc(sizeof(video));
	if(!temp) return NULL;

	temp->fp = fp;
	temp->handle = handle;
	file_read( (void*)&temp->width, sizeof(int), 1, fp );
	file_read( (void*)&temp->height, sizeof(int), 1, fp );
	file_read( (void*)&temp->frames, sizeof(int), 1, fp );
	file_read( (void*)&temp->fps, sizeof(float), 1, fp );

	dec_param.x_dim = temp->width;
	dec_param.y_dim = temp->height;
	dec_param.output_format = DEC_RGB32;
	dec_param.time_incr=0;
	dec_set.postproc_level = 0;

	if(decore(temp->handle, DEC_OPT_MEMORY_REQS, &dec_param, &dec_mem_reqs)!=DEC_OK) return NULL;

	dec_param.buffers.mp4_edged_ref_buffers =	malloc(dec_mem_reqs.mp4_edged_ref_buffers_size);
	dec_param.buffers.mp4_edged_for_buffers =	malloc(dec_mem_reqs.mp4_edged_for_buffers_size);
	dec_param.buffers.mp4_display_buffers =		malloc(dec_mem_reqs.mp4_display_buffers_size);
	dec_param.buffers.mp4_state =				malloc(dec_mem_reqs.mp4_state_size);
	dec_param.buffers.mp4_tables =				malloc(dec_mem_reqs.mp4_tables_size);
	dec_param.buffers.mp4_stream =				malloc(dec_mem_reqs.mp4_stream_size);

	memset(dec_param.buffers.mp4_state, 0, dec_mem_reqs.mp4_state_size);
	memset(dec_param.buffers.mp4_tables, 0, dec_mem_reqs.mp4_tables_size);
	memset(dec_param.buffers.mp4_stream, 0, dec_mem_reqs.mp4_stream_size);

	if(decore(temp->handle,DEC_OPT_INIT,&dec_param,NULL)!=DEC_OK) return NULL;
	if(decore(temp->handle,DEC_OPT_SETOUT,&dec_param,NULL)!=DEC_OK) return NULL;
	if(decore(temp->handle,DEC_OPT_SETPP,&dec_set,NULL)!=DEC_OK) return NULL;

	temp->start_offset = file_tell(temp->fp);

	temp->current_frame = 0;

	temp->pixeldata = malloc(sizeof(unsigned int)*temp->width*temp->height);
	if(!temp->pixeldata) return NULL;

	if(IDirect3DDevice9_CreateTexture(device,temp->width,temp->height, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &temp->texture, NULL)!=D3D_OK){
		return NULL;
	}

	handle++;
	return temp;
}

BOOL video_get_frame(video *vid, unsigned int *pixeldata, float time){
	static unsigned char buffer[1024*1024];
	DEC_FRAME dec_frame;
	int size;
	int frame;

	time = (float)fmod(time,(float)vid->frames/vid->fps);
	frame = (int)(time*vid->fps);

	if( frame==vid->current_frame ){
#ifdef _DEBUG
//		printf("skipping...\r");
#endif
		return FALSE;
	}

	while(frame!=vid->current_frame){
		if(vid->current_frame>vid->frames){
			file_seek(vid->fp,vid->start_offset,SEEK_SET);
			vid->current_frame = 0;
		}else{
			vid->current_frame++;
		}

		file_read(&size, sizeof(int),1,vid->fp);
		file_read(buffer,size,1,vid->fp);

		dec_frame.bmp = pixeldata;
		dec_frame.bitstream = buffer;
		dec_frame.length = size;
		dec_frame.render_flag = 1;

#ifdef _DEBUG
//		printf("decoding... target frame: %i frame: %i (total: %i) time: %f\r", frame, vid->current_frame, vid->frames, time);
#endif
		if( decore(vid->handle, 0, &dec_frame, NULL) != DEC_OK ){
			printf("jalla-error\n");
			return FALSE;
		}
	}

	return TRUE;
}

void video_update(video *vid, float time){
	if(video_get_frame(vid, vid->pixeldata, time)){
		D3DLOCKED_RECT rect;
		if(IDirect3DTexture9_LockRect(vid->texture, 0, &rect, NULL, 0)==D3D_OK){
			int counter;
			int w, p;
			char *source;
			char *dest;

			source = (char*)vid->pixeldata;
			dest = rect.pBits;

			w = vid->width*sizeof(unsigned int);
			p = rect.Pitch;

			counter = vid->height;
			while(counter--){
				memcpy(dest,source,w);
//				memset(dest, 0x7F,w*2);

				source += w;
				dest += p;
			}

			IDirect3DTexture9_UnlockRect(vid->texture, 0);
			IDirect3DBaseTexture9_GenerateMipSubLevels(vid->texture);

		}else{
			printf("dilldall\n");
		}
	}
}

void free_video(video *vid){
	if(!vid) return;

	if(vid->fp) file_close(vid->fp);
	vid->fp = NULL;

	if(vid->pixeldata) free(vid->pixeldata);
	vid->pixeldata = NULL;

	free(vid);
};
