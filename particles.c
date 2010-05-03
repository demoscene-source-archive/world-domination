#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#include "particles.h"
#include "matrix.h"
#include "texturemanager.h"

/*
typedef struct{
	vector pos;
	unsigned int color;
	float u, v;
}grid_vertex;
#define GRID_VERTEX_TYPE (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
*/


#define PARTICLES_VERTEX_TYPE (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
typedef struct{
	vector pos;
	unsigned int color;
	float u,v;
}particle_vertex;

#define MAX_PARTICLES 4096

__inline void get_up_right_vectors(IDirect3DDevice9 *device, vector *up,vector *right){
	float view[16];
	IDirect3DDevice9_GetTransform( device, D3DTS_VIEW, (D3DMATRIX*)&view );
	*right = vector_make(view[0],view[4],view[8]);
	*up = vector_make(view[1],view[5],view[9]);
}

IDirect3DStateBlock9 *particle_state = NULL;

IDirect3DIndexBuffer9 *particle_indexbuffer = NULL;
IDirect3DVertexBuffer9 *particle_vertexbuffer = NULL;

BOOL init_particles(IDirect3DDevice9 *device){
	unsigned short *index_dest;
	
	if(FAILED(IDirect3DDevice9_CreateVertexBuffer(device,sizeof(particle_vertex)*4*MAX_PARTICLES,D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,PARTICLES_VERTEX_TYPE,D3DPOOL_DEFAULT,&particle_vertexbuffer,NULL)))
		return FALSE;

	if(FAILED(IDirect3DDevice9_CreateIndexBuffer(device,sizeof(unsigned short)*3*2*MAX_PARTICLES,0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&particle_indexbuffer,NULL)))
		return FALSE;

	if(IDirect3DIndexBuffer9_Lock(particle_indexbuffer,0,0,&index_dest,0)==D3D_OK){
		int counter;
		for(counter=0;counter<MAX_PARTICLES;counter++){
			/* tri 1 */
			*index_dest++ = counter*4+0;
			*index_dest++ = counter*4+1;
			*index_dest++ = counter*4+3;

			/* tri 2 */
			*index_dest++ = counter*4+1;
			*index_dest++ = counter*4+2;
			*index_dest++ = counter*4+3;
		}
		IDirect3DIndexBuffer9_Unlock(particle_indexbuffer);
	}else return FALSE;

	IDirect3DDevice9_BeginStateBlock(device);
	{
		int i;

		IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);

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
	if(IDirect3DDevice9_EndStateBlock(device,&particle_state)!=D3D_OK) return FALSE;

	return TRUE;
}

void deinit_particles(){
	if(particle_vertexbuffer) IDirect3DVertexBuffer9_Release(particle_vertexbuffer);
	if(particle_indexbuffer) IDirect3DIndexBuffer9_Release(particle_indexbuffer);
	if(particle_state) IDirect3DStateBlock9_Release(particle_state);
}


void draw_particles(IDirect3DDevice9 *device, particle *particles, unsigned int particle_count, int texture){
	int offset = 0;

	vector right, up;
	matrix world, view;
	matrix_identity(world);
	IDirect3DDevice9_SetTransform( device, D3DTS_WORLD, (D3DMATRIX*)&world );
	IDirect3DDevice9_GetTransform( device, D3DTS_VIEW, (D3DMATRIX*)&view );

	right = matrix_get_col( view, 0 );
	up = matrix_get_col( view, 1 );

	get_up_right_vectors(device,&up,&right);

	IDirect3DStateBlock9_Apply(particle_state);
	set_texture(device,0,texture);

	IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_ONE);
	IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);

	while(particle_count>0){
		particle_vertex *vertex_dest;
		int to_draw = MAX_PARTICLES;
		if(particle_count<MAX_PARTICLES) to_draw = particle_count;

		if(IDirect3DVertexBuffer9_Lock(particle_vertexbuffer, 0, 0, (BYTE**)&vertex_dest, 0)==D3D_OK){
			int counter;
			for(counter=0;counter<to_draw;counter++){
				int urinlekasje = counter*4;
				vector particle_pos = particles[offset+counter].pos;
				float half_size = particles[offset+counter].size*0.5f;
				float alpha = particles[offset+counter].alpha;
				unsigned int color = 0xFFFFFF;
				if(alpha>1) alpha = 1;
				if(alpha<0) alpha = 0;
				color |= (int)(alpha*255)<<24;

				vertex_dest[urinlekasje+0].pos = vector_add(particle_pos,vector_scale(vector_sub(vector_scale(right,-1),up),half_size));
				vertex_dest[urinlekasje+0].u = 0;
				vertex_dest[urinlekasje+0].v = 1;
				vertex_dest[urinlekasje+0].color = color;

				vertex_dest[urinlekasje+1].pos = vector_add(particle_pos,vector_scale(vector_sub(right,up),half_size));
				vertex_dest[urinlekasje+1].u = 1;
				vertex_dest[urinlekasje+1].v = 1;
				vertex_dest[urinlekasje+1].color = color;

				vertex_dest[urinlekasje+2].pos = vector_add(particle_pos,vector_scale(vector_add(right,up),half_size));
				vertex_dest[urinlekasje+2].u = 1;
				vertex_dest[urinlekasje+2].v = 0;
				vertex_dest[urinlekasje+2].color = color;

				vertex_dest[urinlekasje+3].pos = vector_add(particle_pos,vector_scale(vector_add(vector_scale(right,-1),up),half_size));
				vertex_dest[urinlekasje+3].u = 0;
				vertex_dest[urinlekasje+3].v = 0;
				vertex_dest[urinlekasje+3].color = color;

			}
			IDirect3DVertexBuffer9_Unlock(particle_vertexbuffer);
		}

		IDirect3DDevice9_SetFVF(device, PARTICLES_VERTEX_TYPE);
		IDirect3DDevice9_SetStreamSource(device, 0, particle_vertexbuffer, 0, sizeof(particle_vertex));
		IDirect3DDevice9_SetIndices(device,particle_indexbuffer);
		IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,0,0,4*MAX_PARTICLES,0,2*to_draw);

		particle_count -= to_draw;
		offset += to_draw;
	}
}

/*
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


*/