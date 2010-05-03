#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#include "object.h"
#include "file.h"
#include "texturemanager.h"
#include "material.h"

void generate_normals(object *obj){
	unsigned int mesh,i;
	unsigned short *face;

	if(!obj) return;

	for(i=0;i<obj->vertex_count;i++)
		obj->vertices[i].normal = vector_make(0,0,0);

	for(mesh=0;mesh<obj->submesh_count;mesh++){
		face = obj->submeshes[mesh].triangles;
		for(i=0;i<obj->submeshes[mesh].triangle_count;i++){
			vector facenormal = vector_scale(
									vector_crossproduct(
										vector_sub( obj->vertices[face[i*3+2]].vertex, obj->vertices[face[i*3+0]].vertex ),
										vector_sub( obj->vertices[face[i*3+1]].vertex, obj->vertices[face[i*3+0]].vertex )
									),-1
								);

			obj->vertices[face[i*3+0]].normal =  vector_add( obj->vertices[face[i*3+0]].normal, facenormal );
			obj->vertices[face[i*3+1]].normal =  vector_add( obj->vertices[face[i*3+1]].normal, facenormal );
			obj->vertices[face[i*3+2]].normal =  vector_add( obj->vertices[face[i*3+2]].normal, facenormal );
	
		}
	}

//	for(i=0;i<obj->vertex_count;i++)
//		obj->vertices[i].normal = vector_normalize(obj->vertices[i].normal);
}

static char *get_dirname(char *filename){
	char temp[256];
	int i = strlen(filename);
	while(i--){
		if((filename[i]=='\\')||(filename[i]=='/')){
			break;
		}
	}
	strncpy(temp,filename,i+1);
	temp[i+1] = '\0';
	return strdup(temp);
}

object *load_object(IDirect3DDevice9 *device, char *filename){
	matrix position, rotation, scale;
	vector position_vector, scale_vector;
	quat rotation_quat;

	unsigned int submesh_counter;
	char test[4];
	object *temp;
	file *fp;
	vertex *dest;

	if(!filename) return NULL;

	fp = file_open(filename);
	if(!fp) return NULL;

	file_read(test,1,4,fp);
	if(memcmp(test,"KRO0",4)!=0){
		file_close(fp);
		return NULL;
	}

	temp = (object*)malloc(sizeof(object));
	if(!temp) return NULL;

	file_read(&position_vector,sizeof(float),3,fp);
	file_read(&rotation_quat,sizeof(float),4,fp);
	file_read(&scale_vector,sizeof(float),3,fp);

	file_read(&temp->vertex_count,1,4,fp);
	if(temp->vertex_count==0){
		/* dummy-object, skip loading */
		file_close(fp);
	
		temp->update = FALSE;
		temp->vertexbuffer = NULL;
		temp->vertices = 0;
		temp->submesh_count = 0;
		temp->submeshes = NULL;
		/* save prs */
		temp->prs.pos = position_vector;
		temp->prs.rot = rotation_quat;
		temp->prs.scale = scale_vector;

		/* build matrix */
		matrix_identity(temp->mat);
		matrix_translate(position, position_vector);
		matrix_from_quat(rotation, rotation_quat);
		matrix_scale(scale, scale_vector);
		matrix_multiply(temp->mat, temp->mat, position);
		matrix_multiply(temp->mat, temp->mat, rotation);
		matrix_multiply(temp->mat, temp->mat, scale);
		return temp;
	}

	temp->vertices = (vertex*)malloc(sizeof(vertex)*temp->vertex_count);
	if(!temp->vertices) return FALSE;
	file_read(temp->vertices,sizeof(vertex),temp->vertex_count,fp);

	file_read(&temp->morphtarget_count,1,4,fp);
	temp->morphtarget_vertices = malloc(sizeof(vector)*temp->vertex_count*temp->morphtarget_count);
	if(!temp->morphtarget_vertices) return FALSE;
	file_read(temp->morphtarget_vertices,sizeof(vector),temp->vertex_count*temp->morphtarget_count,fp);

	file_read(&temp->submesh_count,1,4,fp);
	temp->submeshes = (submesh*)malloc(sizeof(submesh)*temp->submesh_count);
	if(!temp->submeshes) return FALSE;

	for(submesh_counter=0;submesh_counter<temp->submesh_count;submesh_counter++){
		unsigned int* dest;
		char mat_name[256];
		char *material_name = file_loadstring(fp);

		sprintf(mat_name,"%s%s",get_dirname(filename),material_name); 
		temp->submeshes[submesh_counter].mat = material_load(device,mat_name);

		file_read(&temp->submeshes[submesh_counter].triangle_count,1,4,fp);
		temp->submeshes[submesh_counter].triangles = (unsigned short*)malloc(sizeof(unsigned short)*3*temp->submeshes[submesh_counter].triangle_count);
		if(!temp->submeshes[submesh_counter].triangles) return FALSE;
		file_read(temp->submeshes[submesh_counter].triangles,sizeof(unsigned short)*3,temp->submeshes[submesh_counter].triangle_count,fp);

		if(FAILED(IDirect3DDevice9_CreateIndexBuffer(device,sizeof(unsigned short)*3*temp->submeshes[submesh_counter].triangle_count,0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&temp->submeshes[submesh_counter].indexbuffer,NULL)))
			return FALSE;

		if(IDirect3DIndexBuffer9_Lock(temp->submeshes[submesh_counter].indexbuffer,0,0,&dest,0)==D3D_OK){
			memcpy(dest,temp->submeshes[submesh_counter].triangles,sizeof(unsigned short)*3*temp->submeshes[submesh_counter].triangle_count);
			IDirect3DIndexBuffer9_Unlock(temp->submeshes[submesh_counter].indexbuffer);
		}else return FALSE;
	}
	file_close(fp);

	if(temp->morphtarget_count > 0) {
		printf("something shall morph!");
		if(FAILED(IDirect3DDevice9_CreateVertexBuffer(device,sizeof(vertex)*temp->vertex_count,D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,OBJECT_VERTEX_TYPE,D3DPOOL_DEFAULT,&temp->vertexbuffer,NULL)))
			return FALSE;
	} else {
		if(FAILED(IDirect3DDevice9_CreateVertexBuffer(device,sizeof(vertex)*temp->vertex_count,0,OBJECT_VERTEX_TYPE,D3DPOOL_MANAGED,&temp->vertexbuffer,NULL)))
			return FALSE;
	}
	if(IDirect3DVertexBuffer9_Lock(temp->vertexbuffer, 0, 0, (BYTE**)&dest, 0)==D3D_OK){
		memcpy(dest,temp->vertices,sizeof(vertex)*temp->vertex_count);
		IDirect3DVertexBuffer9_Unlock(temp->vertexbuffer);
		temp->update = FALSE;
	}else return FALSE;

	/* save prs */
	temp->prs.pos = position_vector;
	temp->prs.rot = rotation_quat;
	temp->prs.scale = scale_vector;

	/* build matrix */
	matrix_identity(temp->mat);
	matrix_translate(position, position_vector);
	matrix_from_quat(rotation, rotation_quat);
	matrix_scale(scale, scale_vector);
	matrix_multiply(temp->mat, temp->mat, position);
	matrix_multiply(temp->mat, temp->mat, rotation);
	matrix_multiply(temp->mat, temp->mat, scale);

	return temp;
}

void free_object(object *obj){
	unsigned int mesh;
	if(obj->vertexbuffer) IDirect3DVertexBuffer9_Release(obj->vertexbuffer);
	if(obj->vertices){
		free(obj->vertices);
		obj->vertices=NULL;
	}
	if(obj->submeshes){
		for(mesh=0;mesh<obj->submesh_count;mesh++){
			if(obj->submeshes[mesh].indexbuffer) IDirect3DIndexBuffer9_Release(obj->submeshes[mesh].indexbuffer);
			obj->submeshes[mesh].indexbuffer = NULL;

			free(obj->submeshes[mesh].triangles);
			obj->submeshes[mesh].triangles=NULL;
		}
		free(obj->submeshes);
		obj->submeshes=NULL;
	}
	free(obj);
}

void morph_object(object *obj, float t){
	unsigned int i;
	vector *p1, *p2;
	vertex *d;
	int start, end;

	t = (float)fmod(t,1);

	if(t==0) start = 0;
	else start = (int)(((float)obj->morphtarget_count)*t);
	end = start+1;
	
	t -= (float)start/(float)obj->morphtarget_count;
	t *= obj->morphtarget_count;

	if(!obj) return;
	if(obj->morphtarget_count<2) return;

	if(t>1) t=1;
	if(t<0) t=0;

	start %= obj->morphtarget_count;
	end %= obj->morphtarget_count;

	p1 = &obj->morphtarget_vertices[obj->vertex_count*start];
	p2 = &obj->morphtarget_vertices[obj->vertex_count*end];
	d = obj->vertices;

	for(i=0;i<obj->vertex_count;i++){
		d->vertex = vector_add(*p1,vector_scale(vector_sub(*p2,*p1),t));

		p1++;
		p2++;
		d++;
	}

	obj->update = TRUE;

}

void draw_object(IDirect3DDevice9 *device, object *obj){
	unsigned int mesh;
	if(!device) return;
	if(!obj) return;

	if(obj->update){
		vertex *dest;
		generate_normals(obj);
		IDirect3DVertexBuffer9_Lock(obj->vertexbuffer, 0, 0, (BYTE**)&dest, 0 );
		memcpy(dest,obj->vertices,sizeof(vertex)*obj->vertex_count);
		IDirect3DVertexBuffer9_Unlock(obj->vertexbuffer);
		obj->update = FALSE;
	}

	IDirect3DDevice9_SetTransform( device, D3DTS_WORLD, (D3DMATRIX*)&obj->mat );

	IDirect3DDevice9_SetStreamSource(device, 0, obj->vertexbuffer, 0, sizeof(vertex));
	IDirect3DDevice9_SetFVF(device, OBJECT_VERTEX_TYPE);

	for(mesh=0;mesh<obj->submesh_count;mesh++){
		if(obj->submeshes[mesh].mat) set_material(device,obj->submeshes[mesh].mat);
		IDirect3DDevice9_SetIndices(device,obj->submeshes[mesh].indexbuffer);
		IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0,0,obj->vertex_count, 0,obj->submeshes[mesh].triangle_count);
	}
}

void draw_object_ex(IDirect3DDevice9 *device, object *obj, BOOL trans){
	unsigned int mesh;
	if(!device) return;
	if(!obj) return;

	if(obj->update){
		vertex *dest;
		generate_normals(obj);
		IDirect3DVertexBuffer9_Lock(obj->vertexbuffer, 0, 0, (BYTE**)&dest, 0 );
		memcpy(dest,obj->vertices,sizeof(vertex)*obj->vertex_count);
		IDirect3DVertexBuffer9_Unlock(obj->vertexbuffer);
		obj->update = FALSE;
	}

	IDirect3DDevice9_SetTransform( device, D3DTS_WORLD, (D3DMATRIX*)&obj->mat );
	IDirect3DDevice9_SetStreamSource(device, 0, obj->vertexbuffer, 0, sizeof(vertex));
	IDirect3DDevice9_SetFVF(device, OBJECT_VERTEX_TYPE);

	for(mesh=0;mesh<obj->submesh_count;mesh++){
		if(obj->submeshes[mesh].mat){
			if(obj->submeshes[mesh].mat->additive==trans){
				set_material(device,obj->submeshes[mesh].mat);
				IDirect3DDevice9_SetIndices(device,obj->submeshes[mesh].indexbuffer);
				IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0,0,obj->vertex_count, 0,obj->submeshes[mesh].triangle_count);
			}
		}else{
			if(!trans){
				IDirect3DDevice9_SetIndices(device,obj->submeshes[mesh].indexbuffer);
				IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0,0,obj->vertex_count, 0,obj->submeshes[mesh].triangle_count);
			}
		}
	}
}
