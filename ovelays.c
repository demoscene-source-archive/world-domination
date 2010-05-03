#include "overlays.h"
#include "vector.h"
#include "matrix.h"
#include "texturemanager.h"

#include <stdio.h>

typedef struct{
	vector pos;
	float u, v;
}overlay_vertex;

#define OVERLAY_VERTEX_TYPE (D3DFVF_XYZ|D3DFVF_TEX1)

typedef struct{
	vector pos;
	unsigned int color;
	float u, v;
}grid_vertex;

#define GRID_VERTEX_TYPE (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)


overlay_vertex overlay_vertices[4] = {
	{{-0.5f,	-0.5f,	0.f}, 0.f, 0.f},
	{{639.5f,	-0.5f,	0.f}, 1.f, 0.f},
	{{639.5f,	479.5f,	0.f}, 1.f, 1.f},
	{{-0.5f,	479.5f,	0.f}, 0.f, 1.f}
};

IDirect3DVertexBuffer9 *overlay_vertexbuffer = NULL;
IDirect3DStateBlock9 *overlay_state = NULL;

vector grid_vertices[((640/8)+1)*((480/8)+1)];

IDirect3DVertexBuffer9 *grid_vertexbuffer = NULL;
IDirect3DIndexBuffer9 *grid_indexbuffer = NULL;

IDirect3DTexture9 *radialblur_rtt_texture[2] = {NULL, NULL};
IDirect3DSurface9 *radialblur_rtt_surface[2] = {NULL, NULL};
int radialblur_rtt_texture_id[2];

BOOL init_overlays(IDirect3DDevice9 *device){
	overlay_vertex *overlay_vertex_dest;
	grid_vertex *grid_vertex_dest;
	unsigned short *index_dest;
	int i;

	if(FAILED(IDirect3DDevice9_CreateVertexBuffer(device,sizeof(overlay_vertex)*4,0,OVERLAY_VERTEX_TYPE,D3DPOOL_MANAGED,&overlay_vertexbuffer,NULL)))
		return FALSE;

	if(FAILED(IDirect3DDevice9_CreateVertexBuffer(device,sizeof(grid_vertex)*((640/8)+1)*((480/8)+1),D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, GRID_VERTEX_TYPE, D3DPOOL_DEFAULT,&grid_vertexbuffer,NULL)))
		return FALSE;

	if(FAILED(IDirect3DDevice9_CreateIndexBuffer(device,sizeof(unsigned short)*3*(640/8)*(480/8)*2,0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&grid_indexbuffer,NULL)))
		return FALSE;

	IDirect3DVertexBuffer9_Lock(overlay_vertexbuffer, 0, 0, (BYTE**)&overlay_vertex_dest, 0 );
	memcpy(overlay_vertex_dest,overlay_vertices,sizeof(overlay_vertex)*4);
	IDirect3DVertexBuffer9_Unlock(overlay_vertexbuffer);

	IDirect3DDevice9_BeginStateBlock(device);
	{
		int i;
		matrix m;
		matrix_identity(m);

		IDirect3DDevice9_SetTransform( device, D3DTS_WORLD, (D3DMATRIX*)&m );
		IDirect3DDevice9_SetTransform( device, D3DTS_VIEW, (D3DMATRIX*)&m );
		IDirect3DDevice9_SetTransform( device, D3DTS_TEXTURE0, (D3DMATRIX*)&m );

		D3DXMatrixOrthoOffCenterLH((D3DMATRIX*)&m,
			0,640,
			480,0,
			0,1);

		IDirect3DDevice9_SetTransform( device, D3DTS_PROJECTION, (D3DMATRIX*)&m );

		IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);

		for(i=1;i<8;i++){
			IDirect3DDevice9_SetTextureStageState( device, i, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU );
			IDirect3DDevice9_SetTextureStageState( device, i, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			IDirect3DDevice9_SetTextureStageState( device, i, D3DTSS_COLORARG2, D3DTA_CURRENT );
			set_texture(device,i,-1);
		}

		IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU );
		IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT );

		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	}

	if(IDirect3DDevice9_EndStateBlock(device,&overlay_state)!=D3D_OK) return FALSE;

	for(i=0;i<2;i++){
		char temp[256];

		if(IDirect3DDevice9_CreateTexture(device,640,480,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT, &radialblur_rtt_texture[i], NULL)!=D3D_OK)
			return FALSE;
		if(IDirect3DTexture9_GetSurfaceLevel(radialblur_rtt_texture[i],0,&radialblur_rtt_surface[i])!=D3D_OK)
			return FALSE;

		sprintf(temp,"radialblur_rtt_dill%i.jpg",i);
		radialblur_rtt_texture_id[i] = texture_insert(device, temp, radialblur_rtt_texture[i]);
	}

	if(IDirect3DVertexBuffer9_Lock(grid_vertexbuffer, 0, 0, (BYTE**)&grid_vertex_dest, 0)==D3D_OK){
		int x,y;
		for(y=0;y<(480+8);y+=8){
			for(x=0;x<(640+8);x+=8){
				float u = x*(1.f/(float)640);
				float v = y*(1.f/(float)480);
				grid_vertex_dest->pos = vector_make((float)x-0.5f,(float)y-0.5f,0.f);
				grid_vertex_dest->u = u;
				grid_vertex_dest->v = v;
				grid_vertex_dest->color = 0xFFFFFFFF;
				grid_vertex_dest++;
			}
		}
		IDirect3DVertexBuffer9_Unlock(grid_vertexbuffer);
	}else return FALSE;

	if(IDirect3DIndexBuffer9_Lock(grid_indexbuffer,0,0,&index_dest,0)==D3D_OK){
		int x, y;
		for(y=0;y<(480/8);y++){
			for(x=0;x<(640/8);x++){
				*index_dest++ = x+y*((640/8)+1);
				*index_dest++ = x+1+y*((640/8)+1);
				*index_dest++ = x+(y+1)*((640/8)+1);

				*index_dest++ = x+1+y*((640/8)+1);
				*index_dest++ = x+1+(y+1)*((640/8)+1);
				*index_dest++ = x+(y+1)*((640/8)+1);
			}
		}
		IDirect3DIndexBuffer9_Unlock(grid_indexbuffer);
	}else return FALSE;

	return TRUE;
}

void deinit_overlays(){
	int i;

	if(overlay_state!=NULL)			IDirect3DStateBlock9_Release(overlay_state);
	if(overlay_vertexbuffer!=NULL)	IDirect3DVertexBuffer9_Release(overlay_vertexbuffer);
	if(grid_vertexbuffer!=NULL)		IDirect3DVertexBuffer9_Release(grid_vertexbuffer);
	if(grid_indexbuffer!=NULL)		IDirect3DIndexBuffer9_Release(grid_indexbuffer);

	for(i=0;i<2;i++) if(radialblur_rtt_surface[i]!=NULL)
		radialblur_rtt_surface[i]->lpVtbl->Release(radialblur_rtt_surface[i]);
}

void draw_grid(IDirect3DDevice9 *device, grid g, int texture, BOOL additive){
	static float time = 0;
	grid_vertex *vertex_dest;
	if(!grid_vertexbuffer) return;
	if(!grid_indexbuffer) return;
	IDirect3DStateBlock9_Apply(overlay_state);

	set_texture(device,0,texture);

	if(additive){
		IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_ONE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_ONE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
	}else{
		IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
	}

	IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );


	if(IDirect3DVertexBuffer9_Lock(grid_vertexbuffer, 0, 0, (BYTE**)&vertex_dest, 0)==D3D_OK){
		int x,y;
		for(y=0;y<(480+8);y+=8){
			for(x=0;x<(640+8);x+=8){
				vertex_dest->u = g->u;
				vertex_dest->v = g->v;
				vertex_dest->color = g->color;
				vertex_dest++;
				g++;
			}
		}
		IDirect3DVertexBuffer9_Unlock(grid_vertexbuffer);
	};

	IDirect3DDevice9_SetFVF(device, GRID_VERTEX_TYPE);
	IDirect3DDevice9_SetStreamSource(device, 0, grid_vertexbuffer, 0, sizeof(grid_vertex));
	IDirect3DDevice9_SetIndices(device,grid_indexbuffer);
	IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0,0,((640/8)+1)*((480/8)+1), 0,(640/8)*(480/8)*2);
	time += 0.01f;
}

void draw_overlay(IDirect3DDevice9 *device, int texture, float xoffs, float yoffs, float alpha, BOOL additive){
	matrix m;
	if(alpha==0.f) return;

	if(alpha>1) alpha=1;
	if(alpha<0) alpha=0;


	if(!overlay_vertexbuffer) return;
	IDirect3DStateBlock9_Apply(overlay_state);

	set_texture(device,0,texture);

	if(additive){
		if(alpha<1.f){
			unsigned int color =/* 0xFFFFFF |*/ ((int)(alpha*255.f)<<24);
			IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, color);
			IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );

			IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_ONE);
		}else{
			IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_ONE);
			IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_ONE);
		}

		IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
	}else{
		unsigned int color = 0xFFFFFF | ((int)(alpha*255.f)<<24);
		IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, color);
		IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR );
		IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );

		IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
	}

	matrix_texture_translate(m, xoffs, yoffs);

	IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
	IDirect3DDevice9_SetTransform( device, D3DTS_TEXTURE0, (D3DMATRIX*)&m);

	IDirect3DDevice9_SetFVF(device, OVERLAY_VERTEX_TYPE);
	IDirect3DDevice9_SetStreamSource(device, 0, overlay_vertexbuffer, 0, sizeof(overlay_vertex));
	IDirect3DDevice9_DrawPrimitive(device,D3DPT_TRIANGLEFAN,0,2);
}

#define OUTER_BLUR_LEN 4
#define INNER_BLUR_LEN 2
#define TEXTURE_UNITS 2

void draw_radialblur(IDirect3DDevice9 *device, float x, float y, float scale_, float rot_, int texture, BOOL additive){
	matrix identity;
	IDirect3DSurface9 *prev_rendertarget;
	float alpha;
	float scale;
	float scale_delta;
	float rot;
	float rot_delta;
	int inner_counter, outer_counter;
	int target;
	matrix texture_matrix, m;
	int i;

	if(!overlay_vertexbuffer) return;
	IDirect3DStateBlock9_Apply(overlay_state);

	matrix_identity(identity);

	IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
	IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	IDirect3DDevice9_SetTextureStageState( device, 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
	set_texture(device,0,texture);

	for(i=1;i<TEXTURE_UNITS;i++){
		set_texture(device,i,texture);
		IDirect3DDevice9_SetTextureStageState( device, i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
		IDirect3DDevice9_SetTextureStageState( device, i, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		IDirect3DDevice9_SetTextureStageState( device, i, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
		IDirect3DDevice9_SetTextureStageState( device, i, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA );
	}

	if(additive){
		unsigned int color;
		alpha = 0.5f;
		color = 0xFFFFFF | (unsigned int)(alpha*255)<<24;

		IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, color);
		IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_ONE);
	}else{
		unsigned int color;
		alpha = 0.4f;
		color = 0xFFFFFF | (unsigned int)(alpha*255)<<24;

		IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, color);
		IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}

	IDirect3DDevice9_SetStreamSource(device, 0, overlay_vertexbuffer, 0, sizeof(overlay_vertex));
	IDirect3DDevice9_SetFVF(device, OVERLAY_VERTEX_TYPE);

	IDirect3DDevice9_GetRenderTarget(device,0,&prev_rendertarget);
	
	scale_delta = scale_/(OUTER_BLUR_LEN*INNER_BLUR_LEN*TEXTURE_UNITS);
	rot_delta = rot_/(OUTER_BLUR_LEN*INNER_BLUR_LEN*TEXTURE_UNITS);

	for(outer_counter=0;outer_counter<OUTER_BLUR_LEN;outer_counter++){
		scale = 1.f;
		rot = 0.f;

		target = outer_counter & 1;

		IDirect3DDevice9_SetRenderTarget(device,0,radialblur_rtt_surface[target]);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);

		IDirect3DDevice9_SetTransform( device, D3DTS_TEXTURE0+i, (D3DMATRIX*)&identity);

		for(i=1;i<TEXTURE_UNITS;i++){
			/* move coordinate-system origo to center */
			matrix_texture_translate(texture_matrix, .5f+x, .5f+y);
			/* scale from 4:3 to 1:1 */
			matrix_texture_scale(m, 1*scale, (4.f/3.f)*scale);
			matrix_multiply(texture_matrix, texture_matrix, m);
			/* rotate */
			matrix_texture_rotate(m, rot);
			matrix_multiply(texture_matrix, texture_matrix, m);
			/* scale back to 4:3 from 1:1 */
			matrix_texture_scale(m, 1, 3.f/4.f);
			matrix_multiply(texture_matrix, texture_matrix, m);
			/* move coordinate-system back to bottom left */
			matrix_texture_translate(m, -.5f-x, -.5f-y);
			matrix_multiply(texture_matrix, texture_matrix, m);
			/* screenspace move */
			matrix_texture_translate(m, 0, 0);
			matrix_multiply(texture_matrix, texture_matrix, m);
			scale -= scale_delta;
			rot += rot_delta;
			IDirect3DDevice9_SetTransform( device, D3DTS_TEXTURE0+i, (D3DMATRIX*)&texture_matrix);
		}

		IDirect3DDevice9_DrawPrimitive(device,D3DPT_TRIANGLEFAN,0,2);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);

		for(inner_counter=0;inner_counter<(INNER_BLUR_LEN-1);inner_counter++){
			for(i=0;i<TEXTURE_UNITS;i++){
				/* move coordinate-system origo to center */
				matrix_texture_translate(texture_matrix, .5f+x, .5f+y);
				/* scale from 4:3 to 1:1 */
				matrix_texture_scale(m, 1*scale, (4.f/3.f)*scale);
				matrix_multiply(texture_matrix, texture_matrix, m);
				/* rotate */
				matrix_texture_rotate(m, rot);
				matrix_multiply(texture_matrix, texture_matrix, m);
				/* scale back to 4:3 from 1:1 */
				matrix_texture_scale(m, 1, 3.f/4.f);
				matrix_multiply(texture_matrix, texture_matrix, m);
				/* move coordinate-system back to bottom left */
				matrix_texture_translate(m, -.5f-x, -.5f-y);
				matrix_multiply(texture_matrix, texture_matrix, m);
				/* screenspace move */
				matrix_texture_translate(m, 0, 0);
				matrix_multiply(texture_matrix, texture_matrix, m);
				scale -= scale_delta;
				rot += rot_delta;
				IDirect3DDevice9_SetTransform( device, D3DTS_TEXTURE0+i, (D3DMATRIX*)&texture_matrix);
			}
			IDirect3DDevice9_DrawPrimitive(device,D3DPT_TRIANGLEFAN,0,2);
		}
		scale_delta *= 1.7f;
		rot_delta *= 1.7f;

		for(i=0;i<TEXTURE_UNITS;i++)
			IDirect3DDevice9_SetTexture(device, i, (IDirect3DBaseTexture9*)radialblur_rtt_texture[target]);
	}
	for(i=0;i<TEXTURE_UNITS;i++){
		IDirect3DDevice9_SetTextureStageState(device, i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	}

	IDirect3DDevice9_SetRenderTarget(device,0,prev_rendertarget);
	IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
	IDirect3DDevice9_DrawPrimitive(device,D3DPT_TRIANGLEFAN,0,2);

	prev_rendertarget->lpVtbl->Release(prev_rendertarget);

}
