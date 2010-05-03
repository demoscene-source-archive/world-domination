/*

TIMELINE-FJAS

"we molest the bigscreen" - fallende bokstaver.


135

*/

//#define BIGSCREEN


#define BPM 135

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "bass.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433f
#endif

#include "vector.h"
#include "matrix.h"

#include "d3dwindow.h"
#include "pest.h"
#include "scene.h"
#include "object.h"
#include "texturemanager.h"
#include "video.h"
#include "overlays.h"
#include "metaballs.h"
#include "particles.h"
#include "grideffects.h"

#define WIDTH 640
#define HEIGHT 480
#define TITLE "odd : mordi domination"
#ifndef _DEBUG
#define FULLSCREEN FALSE
#else
#define FULLSCREEN FALSE
#endif

// SETT TIL TRUE FOR BIGSCREEN OG RELEASE (med TRUE er hacken AV)

#ifdef BIGSCREEN
#define FULLSCREEN_HACK TRUE
#else
#define FULLSCREEN_HACK FALSE
#endif

IDirect3DStateBlock9 *default_state;

void init_defaultstate(IDirect3DDevice9 *device){
	int i;
	matrix projection_matrix, view_matrix;

	IDirect3DDevice9_SetVertexShader(device, NULL);
	IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_CCW);

	IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);

	IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
	IDirect3DDevice9_SetRenderState(device, D3DRS_SPECULARENABLE, TRUE);

	IDirect3DDevice9_SetRenderState(device, D3DRS_NORMALIZENORMALS, TRUE);

	for(i=0;i<8;i++){
		IDirect3DDevice9_SetTextureStageState(device, i, D3DTSS_COLOROP, D3DTOP_DISABLE);
		IDirect3DDevice9_SetTextureStageState(device, i, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice9_SetTextureStageState(device, i, D3DTSS_COLORARG2, D3DTA_CURRENT);

		IDirect3DDevice9_SetTextureStageState(device, i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		IDirect3DDevice9_SetTextureStageState(device, i, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

		IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
//		IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	}

	matrix_project( projection_matrix, 45.f, 4.f/3.f, 1.f, 1000.f );
	IDirect3DDevice9_SetTransform( device, D3DTS_PROJECTION, (D3DMATRIX*)&projection_matrix );

	matrix_identity( view_matrix );
	IDirect3DDevice9_SetTransform( device, D3DTS_VIEW, (D3DMATRIX*)&view_matrix );
}

file *fp;
HSTREAM music_file;

void error(char *reason){
	free_textures();
	if (music_file) BASS_StreamFree( music_file );
	if (fp) file_close( fp );
	BASS_Free();
	d3dwin_close();
	MessageBox(NULL, reason, NULL, MB_OK);
	exit(1);
}

void set_gamma(IDirect3DDevice9 *device, float gamma){
	D3DGAMMARAMP ramp;
	int counter;

	for( counter=0; counter<256; counter++ ){
		int val = (int)(pow(counter,gamma)*255);
		if(val<0) val=0;
		if(val>65535) val=65535;
		ramp.red[counter] = val;
		ramp.green[counter] = val;
		ramp.blue[counter] = val;
	}
	IDirect3DDevice9_SetGammaRamp(device,0,D3DSGR_NO_CALIBRATION,&ramp);
}

void make_random_particles(particle *particles, int particle_count, vector offset, float field_scale){
	int i;
	for(i=0;i<particle_count;i++){
		particles[i].pos = vector_add(vector_make(
				(float)(rand()%RAND_MAX-(RAND_MAX/2))*(1.f/RAND_MAX)*field_scale,
				(float)(rand()%RAND_MAX-(RAND_MAX/2))*(1.f/RAND_MAX)*field_scale,
				(float)(rand()%RAND_MAX-(RAND_MAX/2))*(1.f/RAND_MAX)*field_scale
			),offset);
		particles[i].size = 15;//(float)sin(i)*10.f;
		particles[i].alpha = 1;
	}
}

/*
float catmull_rom( float p1, float p2, float p3, float p4, float t){
	 float result = (float)(0.5 * ((-p1 + 3*p2 -3*p3 + p4)*t*t*t
			+ (2*p1-5*p2 + 4*p3 - p4)*t*t
			+ (-p1+p3)*t
			+ 2*p2));
			
	return result;
}
*/

void animate_particles(particle *particles, int particle_count, float time_delta){
	int i;
	vector target = vector_make(0,-40,500);
	time_delta *= 500;
	for(i=0;i<particle_count;i++){
		vector to = vector_sub(target,particles[i].pos);
		float len = vector_magnitude(to);
		if(len>1.f){
			vector dir = vector_normalize(to);
			particles[i].pos = vector_add(particles[i].pos, vector_scale(dir,time_delta*(1.f/(len*0.1f) ) ));
			if(particles[i].pos.z>500){
				particles[i].pos = target;
				len = 0;
			}
			particles[i].alpha = (len-1)*0.005f;
		}else{
			particles[i].size = 0;
			particles[i].alpha = 0;
		}
	}
}

__inline float fade(float start_time, float len, float time, float start, float end){
	float result;
	if(len<=0.f) return 0;

	result = (time-start_time)/len;

	if(result<0.f) return start;
	if(result>1.f) return end;

	return start+result*(end-start);
}

void flash(IDirect3DDevice9 *device, int texture, float time, float start_time, float len){
	float alpha;
	if(len<=0) return;

	alpha = (len-(time-start_time))/len;
	if(alpha>1) return;
	if(alpha>0)	draw_overlay(device, texture, 0,0,alpha,FALSE);
}

float timetable[] = {
	
	// *** intro - odd is back again ***
	// text1 
	0.149f,
	0.481f,
	0.698f,
	0.815f,

	// start distortion text1 (index 4)
	3.48f,
	(3.48f+0.075f),
	3.7f,
	(3.7f+0.075f),
	3.85f,
	(3.85f+0.075f),
	4.0f,
	(4.0f+0.075f),
	4.15f,
	(4.15f+0.075f),
	4.29f,
	(4.29f+0.075f),
	4.39f,
	(4.39f+0.075f),
	4.62f,	//start utfading text1 (index 18)
	5.83f,	//done utfading text1

	// text 2 (index 20)
	7.26f, 
	7.26f + (0.478f - 0.14f),
	7.26f + (0.698f - 0.14f),
	7.26f + (0.8f - 0.14f),
	// start distortion text2 (index 24)
	10.5f,
	10.6f,
	10.74f,
	10.79f,
	10.9f,
	11.03f,
	11.15f,
	11.26f,
	11.42f,
	11.48f,
	11.52f,
	11.55f,
	11.63f,
	11.87f,	//start utfading text2 (index 37)
	12.6f,	//stopp utfading text2 (index 38) 

	14.367f,	//her begynner moroa hehe lol!!! (^2k)

	14.79f,	//once
	15.04f,	//again

	17.92f, // "o" (index 42)
	18.36f, // "d"
	18.8f,  // "d"
	19.70f, // "demos"

	24.57f, // "2003"	(index 46)
	26.36f, // "odd"
	26.78f, // "world"
	27.24f, // "domination"

	28.14f, // "we're back"

	29.03f, // neste part (index 51)

	42.81f, // radiblur
	44.59f, // variform-glenz
	46.37f, // radiblur
	48.15f, // glenz
	49.92f, // radiblur
	51.70f, // glenz
	53.48f, // radiblur
	55.26f, // glenz
	57.04f, // radiblur
	58.81f, // glenz
	60.59f, // radiblur
	62.37f, // glenz
	64.15f, // radiblur
	65.92f, // glenz
	69.47f, // dummy for å unngå fuckup
	69.48f, // glenz igjen

	71.26f, // metaball-tunnel start (index 68)

	85.48f, // mad props
	86.14f, // out to
	87.20f, // deux ex machina
	88.81f, // to valium design

	90.25f, // but
	90.49f, // not
	91.03f, // to
	91.25f, // ephidrena

	101.80f, // begyn bilde-fade (index 77)
	105.39f, // metaball-tunnel stopp

	106.37f, // other
	106.70f, // crews can
	107.16f, // piss
	107.47f, // the
	107.64f, // fuck off
	108.03f, // cause
	108.25f, // we
	108.59f, // eat
	108.81f, // your
	109.03f, // code
	109.46f, // RAAAW (index 89)

	112.60f, // hardcore

	113.93f, // neste part

	124.15f, //
	125.92f, //
	126.36f, //
	126.81f, //

	128.15f,

	131.70f, // o
	132.15f, // d
	132.60f, // d
	134.28f, // in yr face

	143.04f, // (index 102)

	120+36.15f, // flasj

	120+50.8f, // 
	120+63.4f, // 
	120+70 // the end 
};
int time_index = 0;

#define BALLS 5
metaball balls[BALLS];

#define BALLS2 640
metaball balls2[BALLS2];

#define PARTICLES 500
particle particles[PARTICLES];

#define PARTICLES2 500
particle particles2[PARTICLES2];

#define PARTICLES3 2000
particle particles3[PARTICLES3];

int main(){
	BOOL done = FALSE;
	MSG msg;
	IDirect3DDevice9 *device = NULL;
	IDirect3DSurface9 *main_rendertarget = NULL;
	D3DFORMAT format;
	
	float old_time = 0;

	/* render-to-texture-stuff */
	IDirect3DTexture9 *rtt_texture;
	IDirect3DSurface9 *rtt_surface;
	IDirect3DTexture9 *rtt_32_texture;
	IDirect3DSurface9 *rtt_32_surface;

	/* textures used from mainloop */
	int white, black;
	int odd_is_back_again[4];
	int at_the_gathering_2003[4];
	int o_d_d_in_your_face[4];
	int world_domination[4];
	int back_once_again[3];
	int were_back;
	int cred[4];
	int mad_props[4];
	int not_eph[4];
	int piss_the_fuck_off[11];
	int hardcore;

	int refmap, refmap2;
	int eatyrcode;
	int code_0, code_1;
	int overlaytest;
	int circle_particle;

	/* special-textures */
	int rtt_texture_id;
	int rtt_32_texture_id;
	int video_texture_id;
	int dilldall,dilldall2;
	int metaball_text;

	/* videos */
	video *vid;

	/* 3d-scenes */
	scene *fysikkfjall;
	scene *startblob;
	scene *inni_abstrakt;
	scene *korridor;
	scene *skjerm_rom;
	scene *bare_paa_lissom;

	format = D3DFMT_X8R8G8B8;
	device = d3dwin_open(TITLE, WIDTH, HEIGHT, format, FULLSCREEN);
	if(!device){
		format = D3DFMT_A8R8G8B8;
		device = d3dwin_open(TITLE, WIDTH, HEIGHT, format, FULLSCREEN);
		if(!device){
			format = D3DFMT_X1R5G5B5;
			device = d3dwin_open(TITLE, WIDTH, HEIGHT, format, FULLSCREEN);
			if(!device){
				format = D3DFMT_R5G6B5;
				device = d3dwin_open(TITLE, WIDTH, HEIGHT, format, FULLSCREEN);
				if(!device) error("failed to initialize Direct3D9");
			}
		}
	}

#ifdef BIGSCREEN
	set_gamma(device,1.05f);
#endif

	if (!BASS_Init(1, 44100, BASS_DEVICE_LATENCY, win, NULL)) error("failed to initialize BASS");
	fp = file_open("worlddomination.ogg");
	if (!fp) error("music-file not found");
	music_file = BASS_StreamCreateFile(1, fp->data, 0, fp->size, 0);

	/*** music ***/
//	if(!pest_open(win)) error("failed to initialize DirectSond");
//	if(!pest_load("worlddomination.ogg",0)) error("failed to load music-file");


	/*** subsystems ***/
	init_tunnel();
	video_init();
	if(!init_particles(device)) error("failed to initialize");
	if(!init_overlays(device)) error("fuck a duck");
	if(!init_marching_cubes(device)) error("screw a kangaroo");

	make_random_particles(particles, PARTICLES, vector_make(0,-180,0), 500);
	make_random_particles(particles2, PARTICLES2, vector_make(0,-180,0), 500);
	make_random_particles(particles3, PARTICLES3, vector_make(0,0,0), 800);

	/*** rendertextures ***/
	if (IDirect3DDevice9_CreateTexture(device, 512, 256, 0,D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &rtt_texture, NULL)!=D3D_OK) error("failed to create rendertarget-texture");
	if (IDirect3DTexture9_GetSurfaceLevel(rtt_texture,0,&rtt_surface)!=D3D_OK) error("could not get kvasi-backbuffer-surface");
	if ((rtt_texture_id=texture_insert(device, "rendertexture.jpg", rtt_texture))==-1) error("fakk off!");

	if(IDirect3DDevice9_CreateTexture(device,16,16,0,D3DUSAGE_RENDERTARGET|D3DUSAGE_AUTOGENMIPMAP,D3DFMT_X8R8G8B8,D3DPOOL_DEFAULT, &rtt_32_texture, NULL)!=D3D_OK) error("failed to create rendertarget-texture");
	if(IDirect3DTexture9_GetSurfaceLevel(rtt_32_texture,0,&rtt_32_surface)!=D3D_OK) error("could not get kvasi-backbuffer-surface");
	if((rtt_32_texture_id=texture_insert(device, "rendertexture2.jpg", rtt_32_texture))==-1) error("fakk off!");


	/*** textures ***/
	/* solid colors for fades etc */
	if((white=texture_load(device,"white.png",FALSE))==-1) error("shjit!");
	if((black=texture_load(device,"black.png",FALSE))==-1) error("shjit!");

	/* textoverlays */
	if((odd_is_back_again[0]=texture_load(device,"odd_is_back_again_0.png",FALSE))==-1) error("failed to load image");
	if((odd_is_back_again[1]=texture_load(device,"odd_is_back_again_1.png",FALSE))==-1) error("failed to load image");
	if((odd_is_back_again[2]=texture_load(device,"odd_is_back_again_2.png",FALSE))==-1) error("failed to load image");
	if((odd_is_back_again[3]=texture_load(device,"odd_is_back_again_3.png",FALSE))==-1) error("failed to load image");
	if((at_the_gathering_2003[0]=texture_load(device,"at_the_gathering_2003_0.png",FALSE))==-1) error("failed to load image");
	if((at_the_gathering_2003[1]=texture_load(device,"at_the_gathering_2003_1.png",FALSE))==-1) error("failed to load image");
	if((at_the_gathering_2003[2]=texture_load(device,"at_the_gathering_2003_2.png",FALSE))==-1) error("failed to load image");
	if((at_the_gathering_2003[3]=texture_load(device,"at_the_gathering_2003_3.png",FALSE))==-1) error("failed to load image");
	if((back_once_again[0]=texture_load(device,"back_once_again_0.png",TRUE))==-1) error("failed to load image");
	if((back_once_again[1]=texture_load(device,"back_once_again_1.png",TRUE))==-1) error("failed to load image");
	if((back_once_again[2]=texture_load(device,"back_once_again_2.png",TRUE))==-1) error("failed to load image");
	if((o_d_d_in_your_face[0]=texture_load(device,"o_d_d_in_your_face_0.png",TRUE))==-1) error("failed to load image");
	if((o_d_d_in_your_face[1]=texture_load(device,"o_d_d_in_your_face_1.png",TRUE))==-1) error("failed to load image");
	if((o_d_d_in_your_face[2]=texture_load(device,"o_d_d_in_your_face_2.png",TRUE))==-1) error("failed to load image");
	if((o_d_d_in_your_face[3]=texture_load(device,"o_d_d_in_your_face_3.png",TRUE))==-1) error("failed to load image");
	if((world_domination[0]=texture_load(device,"world_domination_0.png",TRUE))==-1) error("failed to load image");
	if((world_domination[1]=texture_load(device,"world_domination_1.png",TRUE))==-1) error("failed to load image");
	if((world_domination[2]=texture_load(device,"world_domination_2.png",TRUE))==-1) error("failed to load image");
	if((world_domination[3]=texture_load(device,"world_domination_3.png",TRUE))==-1) error("failed to load image");
	if((were_back=texture_load(device,"were_back.png",TRUE))==-1) error("failed to load image");

	if((cred[0]=texture_load(device,"cred_0.png",TRUE))==-1) error("failed to load image");
	if((cred[1]=texture_load(device,"cred_1.png",TRUE))==-1) error("failed to load image");
	if((cred[2]=texture_load(device,"cred_2.png",TRUE))==-1) error("failed to load image");
	if((cred[3]=texture_load(device,"cred_3.png",TRUE))==-1) error("failed to load image");
	if((mad_props[0]=texture_load(device,"mad_props_0.png",TRUE))==-1) error("failed to load image");
	if((mad_props[1]=texture_load(device,"mad_props_1.png",TRUE))==-1) error("failed to load image");
	if((mad_props[2]=texture_load(device,"mad_props_2.png",TRUE))==-1) error("failed to load image");
	if((mad_props[3]=texture_load(device,"mad_props_3.png",TRUE))==-1) error("failed to load image");
	if((not_eph[0]=texture_load(device,"not_eph_0.png",TRUE))==-1) error("failed to load image");
	if((not_eph[1]=texture_load(device,"not_eph_1.png",TRUE))==-1) error("failed to load image");
	if((not_eph[2]=texture_load(device,"not_eph_2.png",TRUE))==-1) error("failed to load image");
	if((not_eph[3]=texture_load(device,"not_eph_3.png",TRUE))==-1) error("failed to load image");

	if((piss_the_fuck_off[0]=texture_load(device,"piss_the_fuck_off_0.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[1]=texture_load(device,"piss_the_fuck_off_1.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[2]=texture_load(device,"piss_the_fuck_off_2.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[3]=texture_load(device,"piss_the_fuck_off_3.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[4]=texture_load(device,"piss_the_fuck_off_4.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[5]=texture_load(device,"piss_the_fuck_off_5.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[6]=texture_load(device,"piss_the_fuck_off_6.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[7]=texture_load(device,"piss_the_fuck_off_7.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[8]=texture_load(device,"piss_the_fuck_off_8.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[9]=texture_load(device,"piss_the_fuck_off_9.png",TRUE))==-1) error("failed to load image");
	if((piss_the_fuck_off[10]=texture_load(device,"piss_the_fuck_off_10.png",TRUE))==-1) error("failed to load image");
	if((hardcore=texture_load(device,"hardcore.png",TRUE))==-1) error("failed to load image");

	/* other textures */
	if((circle_particle=texture_load(device,"circle_particle.jpg",FALSE))==-1) error("shjit!");
	if((refmap=texture_load(device,"fysikkfjall/refmap.jpg",FALSE))==-1) error("shjit!");
	if((refmap2=texture_load(device,"refmap2.jpg",FALSE))==-1) error("shjit!");
	if((eatyrcode=texture_load(device,"eatyrcode.jpg",FULLSCREEN_HACK))==-1) error("shjit!");
	if((code_0=texture_load(device,"code-0.jpg",FALSE))==-1) error("shjit!");
	if((code_1=texture_load(device,"code-1.jpg",FALSE))==-1) error("shjit!");
	if((overlaytest=texture_load(device,"overlaytest.jpg",FALSE))==-1) error("shjit!");

	if((dilldall=texture_load(device,"dilldall.png",FALSE))==-1) error("shjit!");
	if((dilldall2=texture_load(device,"dilldall2.png",FALSE))==-1) error("shjit!");

	if((metaball_text=texture_load(device,"metaballs.png",TRUE))==-1) error("shjit!");

	/*** video ***/
	if(!(vid=video_load(device,"test.kpg"))) error("fæck!");
	video_texture_id = texture_insert(device, "skjerm_rom/skjerm_tom.tga", vid->texture);


	/*** misc stuff ***/
	/* main rendertarget */
	IDirect3DDevice9_GetRenderTarget(device,0,&main_rendertarget);

	/* default state */
	IDirect3DDevice9_CreateStateBlock(device, D3DSBT_ALL, &default_state);
	init_defaultstate(device);
	IDirect3DStateBlock9_Capture(default_state);


	/*** 3d scenes ***/
	if(!(fysikkfjall=load_scene(device,"fysikkfjall/fysikkfjall.krs"))) error("failed to load 3d scene");
	if(!(startblob=load_scene(device,"startblob/startblob.krs"))) error("failed to load 3d scene");
	if(!(inni_abstrakt=load_scene(device,"inni_abstrakt/inni_abstrakt.krs"))) error("failed to load 3d scene");
	if(!(korridor=load_scene(device,"korridor/korridor.krs"))) error("failed to load 3d scene");
	if(!(skjerm_rom=load_scene(device,"skjerm_rom/skjerm_rom.krs"))) error("failed to load 3d scene");
	if(!(bare_paa_lissom=load_scene(device,"bare_paa_lissom/bare_paa_lissom.krs"))) error("failed to load 3d scene");

//	pest_play();
	BASS_Start();
	BASS_StreamPlay(music_file, 1, 0);
	do{
		grid g;
		int i;
		matrix m, temp, temp_matrix;
		matrix marching_cubes_matrix;

		long long bytes_played = BASS_ChannelGetPosition(music_file);
		float time =  bytes_played * (1.0 / (44100 * 2 * 2));
//		float time = pest_get_pos()+0.05f;
		float delta_time = time-old_time;

		int beat = (int)(time*((float)BPM/60.f));

		if(time_index<(sizeof(timetable)/4)){
			while(timetable[time_index]<time) time_index++;
		}

#ifdef _DEBUG
		printf("time: %2.2f, delta_time: %2.2f, time_index: %i, beat: %i                    \r",time,delta_time,time_index,beat);
#endif

		IDirect3DDevice9_SetRenderTarget(device,0,main_rendertarget);
		IDirect3DStateBlock9_Apply(default_state);
		IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0, 1.0f, 0);
//		IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		IDirect3DDevice9_BeginScene(device);
#if 1
		if(time_index<40){
			if(time_index>0&&time_index<5){
				draw_overlay(device,odd_is_back_again[time_index-1],0,0,1,FALSE);
			}else if(time_index>4&&time_index<19){
				float itime = time*10;
				grid_flat(g,0,0);
				for(i=0;i<7;i++){
					int scale = (time_index&1);
					grid_wave(g,(float)sin(itime-i+sin(i-itime))*0.3f*scale,(float)cos(itime*0.69f+i*0.733f)*0.3f*scale,7,(float)sin(itime+i*0.1f)*0.5f*scale);
				}
				draw_grid(device, g, odd_is_back_again[3], FALSE);
			}else if(time_index>=19){
				float itime = (time-timetable[18])*0.5f;
				grid_flat(g,0,0);
				for(i=0;i<7;i++){
					grid_wave(g,(float)sin(time-i+sin(i-time))*0.3f,(float)cos(time*0.69f+i*0.733f)*0.3f,7, itime*itime);
				}
				draw_grid(device, g, odd_is_back_again[3], FALSE);
			}
			if(time_index>20&&time_index<25){
				draw_overlay(device,at_the_gathering_2003[(time_index-21)%4],0,0,1,TRUE);
			}else if(time_index>=25&&time_index<38){
				float itime = time*10;
				grid_flat(g,0,0);
				for(i=0;i<10;i++){
					int scale = (time_index&1);
					grid_wave(g,(float)sin(itime-i+sin(i-itime))*0.3f*scale,(float)cos(itime*0.69f+i*0.733f)*0.3f*scale,7,(float)sin(itime+i*0.1f)*0.5f*scale);
				}
				draw_grid(device,g,at_the_gathering_2003[3],TRUE);
			}else if(time_index>=38){
				float itime = (time-timetable[37])*0.5f;
				grid_flat(g,0,0);
				for(i=0;i<10;i++){
					grid_wave(g,(float)sin(time-i+sin(i-time))*0.3f,(float)cos(time*0.69f+i*0.733f)*0.3f,7, itime*itime);
				}
				draw_grid(device,g,at_the_gathering_2003[3],TRUE);
			}
		}else if(time_index<52){
			animate_scene(inni_abstrakt,(time-timetable[39]));
			draw_scene(device,inni_abstrakt,0,TRUE);

			flash(device,white,time,timetable[39],1);

			if(time_index<43){
				draw_overlay(device, back_once_again[(time_index-40)%3],0,0,1,FALSE);
			}else if(time_index<47){
				draw_overlay(device, o_d_d_in_your_face[(time_index-43)%4],0,0,1,FALSE);
			}else if(time_index<51){
				draw_overlay(device, world_domination[(time_index-47)%4],0,0,1,FALSE);
			}else{
				draw_overlay(device, were_back,0,0,(time-timetable[51]),FALSE);
			}

			draw_overlay(device,dilldall,sin(sin(time)*0.07f+(((beat+1)/2)*0.8f)),sin(time*0.03337f+(((beat+1)/2)*0.14f)),0.5f,TRUE);
			draw_overlay(device,dilldall2,sin(sin(time*0.1f)*0.07f+time*0.1f+(((beat+1)/2)*0.8f)),sin(time*0.01337f+(((beat+1)/2)*0.14f)),0.5f,TRUE);

		}else if(time_index<69){

			animate_scene(startblob,(time-timetable[39]-0.27f+((beat+1)&2))*0.74948f);

			startblob->cameras[0].fog = TRUE;
			startblob->cameras[0].fog_start = 100.f;
			startblob->cameras[0].fog_end = 700.f;

			if(time_index>=53 && time_index<66 && time_index&1 ){
				float f = fade(timetable[time_index-1],1.7f,time,0.5f,0);
				f *= f;
				f *= 2;

				if(f>0.f)
				IDirect3DDevice9_SetRenderTarget(device,0,rtt_surface);

				draw_scene(device,startblob,0,TRUE);
				draw_particles(device, particles3, PARTICLES3, code_0);

				if(f>0.f){
					IDirect3DDevice9_SetRenderTarget(device,0,main_rendertarget);
					draw_radialblur(device,0,0,f,0,rtt_texture_id, FALSE);
				}
				if(time_index<61) draw_overlay(device,cred[((time_index/2)-2)%4],0,0,f*3,FALSE);
			}else{
				draw_scene(device,startblob,0,TRUE);
				draw_particles(device, particles3, PARTICLES3, circle_particle);			
			}
			if(time_index>=54&& !(time_index&1)){

				float f = fade(timetable[time_index-1],1.8f,time,1.f,0)*0.8f;
				IDirect3DDevice9_SetRenderTarget(device,0,rtt_32_surface);
				draw_scene(device,startblob,0,TRUE);
				draw_particles(device, particles3, PARTICLES3, code_0);

				IDirect3DDevice9_SetRenderTarget(device,0,main_rendertarget);
				IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				draw_overlay(device,rtt_32_texture_id,0,0,f,TRUE);
				draw_overlay(device,rtt_32_texture_id,0,0,f,TRUE);
				IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			}

			flash(device,were_back,time,timetable[51],2);
			flash(device,white,time,timetable[51],1);
		}else if(time_index<79){

			for(i=0;i<BALLS;i++){
				balls[i].pos.x = (float)sin(time+i-sin((float)i*0.1212111f))*0.35f;
				balls[i].pos.y = (float)cos(time-(float)i*0.29342111f)*0.35f;
				balls[i].pos.z = (float)sin(time*0.31121f+sin(i-time))*0.35f;
				balls[i].r = 0.15f + (float)sin(time+i)*0.01f;
				balls[i].pos = vector_normalize(balls[i].pos);
				balls[i].pos = vector_scale(balls[i].pos, (float)(cos(i*0.11131f-time*0.55311f)+sin(time+(float)sin(time-i+time*0.3f)))*0.2f );
			}

			animate_scene(korridor,(time-timetable[68])*0.65f );
			draw_scene(device,korridor,(beat/4)&1,TRUE);

			memcpy(temp,korridor->objects[korridor->object_count-1]->mat,sizeof(matrix));
			matrix_scale(marching_cubes_matrix,vector_make(120,120,120));
			matrix_multiply(marching_cubes_matrix,temp,marching_cubes_matrix);
	
			matrix_rotate(temp,vector_make(time,-time,time*0.5f+sin(time)));
			matrix_multiply(marching_cubes_matrix,marching_cubes_matrix,temp);

			IDirect3DDevice9_SetTransform( device, D3DTS_WORLD, (D3DMATRIX*)&marching_cubes_matrix );

			fill_metafield_blur(balls, BALLS,0.98f);
			march_my_cubes_opt(balls, BALLS, 0.9f);

			IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);

			set_texture(device,0,refmap2);
			IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR );
			IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
			IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);

			draw_marched_cubes(device);

			flash(device,white,time,timetable[68],1);

			if(time_index>69&&time_index<74){
				draw_overlay(device,mad_props[(time_index+2)%4],0,0,1,FALSE);
			}else if(time_index>73&&time_index<77){
				draw_overlay(device,not_eph[(time_index+2)%4],0,0,1,FALSE);
			}else if(time_index==77){
				float f = fade(timetable[76],2.5f,time,1,0);
				draw_overlay(device,not_eph[3],0,0,f,FALSE);
			}
		}
		if(time_index>77&&time_index<92){
			float f = fade(timetable[77],timetable[78]-timetable[77],time,0,1);
			draw_overlay(device,eatyrcode,0,0,f,FALSE);

			if(time>109.5f){
				IDirect3DStateBlock9_Apply(default_state);
				animate_particles(particles, PARTICLES, delta_time*30);
				animate_particles(particles2, PARTICLES2, delta_time*28);

				draw_particles(device, particles, PARTICLES, code_0);
				draw_particles(device, particles2, PARTICLES2, code_1);
			}
			if(time_index>79&&time_index<90){
				draw_overlay(device,piss_the_fuck_off[(time_index-80)%11],0,0,1,FALSE);
			}else if(time_index==90){
				float f = fade(timetable[89],2,time,1,0);
				draw_overlay(device,piss_the_fuck_off[10],0,0,f,FALSE);
			}
			if(time_index==91){
				float f = fade(timetable[90],2,time,1,0);
				draw_overlay(device,hardcore,0,0,f,FALSE);
			}
		}else if(time_index>91 && time_index<97){
			animate_scene(skjerm_rom,time);
			video_update(vid, time);
			draw_scene(device,skjerm_rom,((beat/4)&1),TRUE);

			draw_overlay(device,dilldall,sin(sin(time)*0.07f+(((beat+1)/2)*0.8f)),sin(time*0.03337f+(((beat+1)/2)*0.14f)),0.5f,TRUE);
			draw_overlay(device,dilldall2,sin(sin(time*0.1f)*0.07f+time*0.1f+(((beat+1)/2)*0.8f)),sin(time*0.01337f+(((beat+1)/2)*0.14f)),0.5f,TRUE);
			if(time_index>92)
				draw_overlay(device, world_domination[(time_index-93)%4],0,0,1,FALSE);

		}else if(time_index>96&&time_index<104){
			grid_zero(g);

			matrix_translate(m, vector_make(cos(time)*1.5f, sin(time)*1.5f,time*10));
			matrix_rotate(temp_matrix, vector_make(sin(time*0.8111f)*0.2f,sin(time*1.2f)*0.2f,time));
			matrix_multiply(m,m,temp_matrix);

			render_tunnel(g,m);

			matrix_rotate(temp_matrix, vector_make(0,M_PI,0));
			matrix_multiply(m,m,temp_matrix);
			render_tunnel(g,m);
			matrix_rotate(temp_matrix, vector_make(M_PI,0,0));
			matrix_multiply(m,m,temp_matrix);
			render_tunnel(g,m);

			grid_add_noice(g,sin(time)*0.1f);

			draw_grid(device, g, circle_particle, FALSE);

			if(time_index>97&&time_index<102){
				float f = 1;
				if(time_index==101) f = fade(timetable[100],3,time,1,0);
				draw_overlay(device, o_d_d_in_your_face[(time_index-42)%4],0,0,f,FALSE);
			}
		}
		if(time_index>101 && time_index<104){
			float f = fade(timetable[101],8,time,0,1);
			float f2 = fade(timetable[101],4,time,0,1);

			draw_overlay(device,black,0,0,f2,FALSE);

			IDirect3DDevice9_SetRenderTarget(device,0,rtt_surface);
			IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

			IDirect3DStateBlock9_Apply(default_state);

			animate_scene(bare_paa_lissom,time-timetable[101]);
			draw_scene(device,bare_paa_lissom,0,TRUE);

			time *=0.5f;

			for(i=0;i<BALLS2;i++){
				balls2[i].pos.x = (float)sin(time+i-sin((float)i*0.1212111f))*0.35f;
				balls2[i].pos.y = (float)cos(time-(float)i*0.29342111f)*0.35f;
				balls2[i].pos.z = (float)sin(time*0.31121f+sin(i-time))*0.35f;
				balls2[i].r = 0.15f + (float)sin(time+i)*0.01f;
				balls2[i].pos = vector_normalize(balls2[i].pos);
				balls2[i].pos = vector_scale(balls2[i].pos, (float)(cos(i*0.11131f-time*0.55311f)+sin(time+(float)sin(time-i+time*0.3f)))*0.2f );
			}

			memcpy(temp,bare_paa_lissom->objects[bare_paa_lissom->object_count-1]->mat,sizeof(matrix));
			matrix_scale(marching_cubes_matrix,vector_make(120,120,120));
			matrix_multiply(marching_cubes_matrix,temp,marching_cubes_matrix);

			IDirect3DDevice9_SetTransform( device, D3DTS_WORLD, (D3DMATRIX*)&marching_cubes_matrix );
			fill_metafield(balls2, BALLS2);
			march_my_cubes_opt(balls2, BALLS2, 0.9f);

			IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);

			set_texture(device,0,refmap);
			IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR );
			IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
			IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);

			draw_marched_cubes(device);

			IDirect3DDevice9_SetRenderTarget(device,0,main_rendertarget);
			draw_overlay(device,rtt_texture_id,0,0,f,TRUE);

			if(time_index==103){
				float f;
				time *= 2;
				f = fade(timetable[time_index-1],3,time,1,0);
				draw_overlay(device,metaball_text,0,0,f,FALSE);

				flash(device,white,time,timetable[time_index-1],1);
			}
		}
		if(time_index==104){
			float f = fade(timetable[103],timetable[104]-timetable[103],time,0,1);

			animate_scene(korridor,180-(time-timetable[time_index-1])*0.8f );
			draw_scene(device,korridor,(beat/4)&1,TRUE);
			draw_overlay(device,black,0,0,f,FALSE);
		}
		if(time_index==105){
			float f = fade(timetable[104],timetable[105]-timetable[104],time,0,1);
			float t = (time-timetable[time_index-1])*0.6f+2.2f;
			animate_scene(fysikkfjall,t);
			draw_scene(device,fysikkfjall,0,TRUE);
			draw_overlay(device,black,0,0,f,FALSE);
		}
#endif
/*
*/
/*
		draw_overlay(device, were_back, (1+sin(time))*0.5f, TRUE);
*/
//		animate_scene(risterom,time);
//		draw_scene(device,risterom,0,TRUE);

#if 0 //helvete_har_frosset
		draw_overlay(device, eatyrcode, 1,FALSE);

		IDirect3DStateBlock9_Apply(default_state);

		animate_particles(particles, PARTICLES, delta_time*30);
		animate_particles(particles2, PARTICLES2, delta_time*28);

		draw_particles(device, particles, PARTICLES, code_0);
		draw_particles(device, particles2, PARTICLES2, code_1);
#endif

#ifdef pikk

//				IDirect3DDevice9_SetRenderTarget(device,0,rtt_surface);
				IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

				draw_overlay(device, eatyrcode, TRUE);

				animate_scene(startblob,(time-timetable[39]-0.27f)*0.7453f);
				draw_scene(device,startblob,0,FALSE);
				draw_particles(device, particles, PARTICLES, code_0);
				draw_particles(device, particles2, PARTICLES2, code_1);
//				IDirect3DDevice9_SetRenderTarget(device,0,main_rendertarget);
/*
				draw_radialblur(device,
					(float)sin(time)*0.2f,
					(float)sin(-time*0.331f)*0.13f,
					(float)(2+(float)sin(time*0.5f))*0.2f,
					0,//sin(time)*0.25f,
					rtt_texture_id, TRUE);

				animate_scene(fysikkfjall,time);
				draw_scene(device,fysikkfjall,0,FALSE);
*/
//				draw_overlay(device, rtt_texture_id, TRUE);
#endif

//		draw_overlay(device, eatyrcode, FALSE);

//		video_update(vid, time);
#if 0
//		IDirect3DDevice9_SetRenderTarget(device,0,rtt_surface);

		animate_scene(testscene,time);

//		morph_object(testscene->objects[0], time );
		draw_scene(device,testscene,0,TRUE);
//		draw_particles(device, particles, PARTICLES, particle);

//		IDirect3DDevice9_SetRenderTarget(device,0,main_rendertarget);
#endif
#if 0
		time *=0.5f;

		for(i=0;i<BALLS2;i++){
			balls2[i].pos.x = (float)sin(time+i-sin((float)i*0.1212111f))*0.35f;
			balls2[i].pos.y = (float)cos(time-(float)i*0.29342111f)*0.35f;
			balls2[i].pos.z = (float)sin(time*0.31121f+sin(i-time))*0.35f;
			balls2[i].r = 0.15f + (float)sin(time+i)*0.01f;
			balls2[i].pos = vector_normalize(balls2[i].pos);
			balls2[i].pos = vector_scale(balls2[i].pos, (float)(cos(i*0.11131f-time*0.55311f)+sin(time+(float)sin(time-i+time*0.3f)))*0.2f );
//			balls2[i].pos.x *= 2.8f;
		}

		matrix_translate(temp,vector_make(0,0,87));
		matrix_scale(marching_cubes_matrix,vector_make(50,50,50));
		matrix_multiply(marching_cubes_matrix,temp,marching_cubes_matrix);

		IDirect3DDevice9_SetTransform( device, D3DTS_WORLD, (D3DMATRIX*)&marching_cubes_matrix );
		fill_metafield(balls2, BALLS2);
//		fill_metafield_blur(balls, BALLS,0.98f);
		march_my_cubes_opt(balls2, BALLS2, 0.9f);
//		march_my_cubes(0.9f);

		IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_CW);

		set_texture(device,0,refmap);
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR );
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);

		draw_marched_cubes(device);
#endif

#if 0
//		time *= 0.5f;
		grid_zero(g);
//		for(i=0;i<10;i++)
//			grid_wave(g,sin(time-i+sin(i-time))*0.3f,cos(time*0.69f+i*0.733f)*0.3f,7,sin(time+i*0.1f)*0.5f);

		matrix_translate(m, vector_make(cos(time)*1.5f, sin(time)*1.5f,time*10));
		matrix_rotate(temp_matrix, vector_make(sin(time*0.8111f)*0.2f,sin(time*1.2f)*0.2f,time));
		matrix_multiply(m,m,temp_matrix);

//		empty_grid( grid );
		render_tunnel(g,m);

		matrix_rotate(temp_matrix, vector_make(0,M_PI,0));
		matrix_multiply(m,m,temp_matrix);
		render_tunnel(g,m);
		matrix_rotate(temp_matrix, vector_make(M_PI,0,0));
		matrix_multiply(m,m,temp_matrix);
		render_tunnel(g,m);
//		tyfuus_expand_grid( screen, grid, texture );

		grid_add_noice(g,sin(time)*0.1f);

		draw_grid(device, g, circle_particle, FALSE);
#endif
//		IDirect3DDevice9_SetRenderTarget(device,0,main_rendertarget);

//		video_update(vid,time);
/*
		draw_radialblur(device,
			(float)sin(time)*0.2f,
			(float)sin(-time*0.331f)*0.13f,
			(float)(1+(float)sin(time*0.5f))*0.2f,
			0,//sin(time)*0.25f,
			rtt_texture_id, FALSE);
*/
//		IDirect3DStateBlock9_Apply(default_state);

//		draw_overlay(device,rtt_texture_id,FALSE);
//		draw_overlay(device,fullscreen,FALSE);

//		draw_radialblur(device,0,sin(time*0.2f)*2.f, rtt_texture_id);

//		IDirect3DDevice9_StretchRect(device,main_rendertarget,NULL,rtt_surface,NULL,D3DTEXF_NONE);
//		IDirect3DBaseTexture9_GenerateMipSubLevels(rtt_texture);

		IDirect3DDevice9_EndScene(device);
		if(IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL)==D3DERR_DEVICELOST)
			error("fakkin lost device. keep your hands off alt-tab, looser.");

		old_time = time;

		while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)){ 
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT ||
			    msg.message == WM_KEYDOWN && LOWORD(msg.wParam) == VK_ESCAPE)
				done = TRUE;
		}
	}while(!done);

	deinit_marching_cubes();
	deinit_overlays();
	deinit_particles();

	free_scene(fysikkfjall);
	free_scene(startblob);
	free_scene(inni_abstrakt);
	free_scene(korridor);
	free_scene(skjerm_rom);
	free_scene(bare_paa_lissom);

	free_materials();
	free_textures();
	free_video(vid);

	rtt_surface->lpVtbl->Release(rtt_surface);
	rtt_32_surface->lpVtbl->Release(rtt_32_surface);

	main_rendertarget->lpVtbl->Release(main_rendertarget);
	IDirect3DStateBlock9_Release(default_state);


	d3dwin_close();

	if (music_file) BASS_StreamFree( music_file );
	if (fp) file_close( fp );
	BASS_Free();

//	pest_close();

	return 0;
}