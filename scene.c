#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3dx9.h>

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979f
#endif

#include "scene.h"
#include "matrix.h"
#include "controller.h"
#include "file.h"

__inline unsigned int make_col(float r, float g, float b){
	unsigned int temp;
	if(r<0) r = 0;
	if(r>1) r = 1;
	if(g<0) g = 0;
	if(g>1) g = 1;
	if(b<0) b = 0;
	if(b>1) b = 1;
	temp = (unsigned char)(r*255);
	temp |= (unsigned char)(g*255)<<8;
	temp |= (unsigned char)(b*255)<<16;
	return temp;
}

#define KT_POS			0
#define KT_ROT			1
#define KT_SCALE		2
#define KT_PR			3
#define KT_PRS			4
#define KT_FOV			5
#define KT_FOG_START	6
#define KT_FOG_END		7

char *get_dirname(char *filename){
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

scene *load_scene(IDirect3DDevice9 *device, char *filename){
	unsigned int i;
	unsigned int entry = 0;
	scene *temp;
	char test[4];

	file *fp = file_open(filename);
	if(!fp) return NULL;

	file_read(test,1,4,fp);
	if(memcmp(test,"KRS0",4)!=0){
		file_close(fp);
		return NULL;
	}

	temp = malloc(sizeof(scene));

	file_read(&temp->entry_count,sizeof(unsigned int),1,fp);
	temp->entries = malloc(sizeof(scene_entry)*temp->entry_count);
	memset(temp->entries,0,sizeof(scene_entry)*temp->entry_count);

	file_read(&temp->background,sizeof(float),3,fp);
	file_read(&temp->ambient,sizeof(float),3,fp);

	file_read(&temp->object_count,sizeof(unsigned int),1,fp);
	temp->objects = malloc(sizeof(object*)*temp->object_count);
	if(!temp->objects) return NULL;

	for(i=0;i<temp->object_count;i++){
		char temp_string[256];
		char *objectfilename;
		char *dirname;

		file_read(&temp->entries[entry].id,sizeof(unsigned int),1,fp);
		temp->entries[entry].entry_type = ENTRY_OBJECT;

		objectfilename = file_loadstring(fp);

		dirname = get_dirname(filename);
		sprintf(temp_string,"%s%s",dirname,objectfilename);
		free(objectfilename);
		free(dirname);
		temp->objects[i] = load_object(device,temp_string);

		if(!temp->objects[i]) return NULL;
		temp->entries[entry].entry = temp->objects[i];
		entry++;
	}

	file_read(&temp->camera_count,sizeof(unsigned int),1,fp);
	temp->cameras = malloc(sizeof(camera)*temp->camera_count);
	if(!temp->cameras) return NULL;

	for(i=0;i<temp->camera_count;i++){
		file_read(&temp->entries[entry].id,sizeof(unsigned int),1,fp);
		temp->entries[entry].entry_type = ENTRY_CAMERA;

		temp->entries[entry].entry = &temp->cameras[i];
		entry++;

		file_read(&temp->cameras[i].prs.pos,sizeof(float),3,fp);
		file_read(&temp->cameras[i].prs.rot,sizeof(float),4,fp);
		file_read(&temp->cameras[i].prs.scale,sizeof(float),3,fp);

		file_read(&temp->cameras[i].fov,sizeof(float),1,fp);
		file_read(&temp->cameras[i].near_clip,sizeof(float),1,fp);
		file_read(&temp->cameras[i].far_clip,sizeof(float),1,fp);

		file_read(&temp->cameras[i].fog,sizeof(BOOL),1,fp);

		file_read(&temp->cameras[i].fog_color.r,sizeof(float),1,fp);
		file_read(&temp->cameras[i].fog_color.g,sizeof(float),1,fp);
		file_read(&temp->cameras[i].fog_color.b,sizeof(float),1,fp);

		file_read(&temp->cameras[i].fog_start,sizeof(float),1,fp);
		file_read(&temp->cameras[i].fog_end,sizeof(float),1,fp);
	}

	file_read(&temp->light_count,sizeof(unsigned int),1,fp);
	temp->lights = malloc(sizeof(light)*temp->light_count);
	if(!temp->lights) return NULL;

	for(i=0;i<temp->light_count;i++){
		file_read(&temp->entries[entry].id,sizeof(unsigned int),1,fp);
		temp->entries[entry].entry_type = ENTRY_LIGHT;

		temp->entries[entry].entry = &temp->lights[i];
		entry++;

		file_read(&temp->lights[i].prs.pos,sizeof(float),3,fp);
		file_read(&temp->lights[i].prs.rot,sizeof(float),4,fp);
		file_read(&temp->lights[i].prs.scale,sizeof(float),3,fp);

	}

	/* animdata */
	file_read(&temp->start,sizeof(float),1,fp);
	file_read(&temp->end,sizeof(float),1,fp);

	file_read(&temp->controller_count,sizeof(unsigned int),1,fp);

	if(temp->controller_count>0){
		temp->controllers = malloc(sizeof(controller*)*temp->controller_count);
		if(!temp->controllers) return NULL;

		for(i=0;i<temp->controller_count;i++){
			object *target_obj;
			camera *target_cam;
			light *target_light;
			controller *current_controller;
			scene_entry *target_entry = NULL;
			unsigned int j;
			unsigned int owner_id;
			unsigned int field;
			unsigned int key_count;

			unsigned int type = CONTROLLER_UNKNOWN;
			void *target = NULL;

			file_read(&owner_id,sizeof(unsigned int),1,fp);
			file_read(&field,sizeof(unsigned int),1,fp);
			file_read(&key_count,sizeof(unsigned int),1,fp);

			for(j=0;j<temp->entry_count;j++){
				if(temp->entries[j].id==owner_id) target_entry = &temp->entries[j];
			}

			if(!target_entry){
				fprintf(stderr, "no target! (%i)\n (only %i targets)",owner_id,temp->entry_count);
				return FALSE;
			}

			switch(target_entry->entry_type){
			case ENTRY_OBJECT:
				target_obj = target_entry->entry;

				switch(field){
				case KT_POS:
					type = CONTROLLER_VECTOR;
					target = &target_obj->prs.pos;
				break;
				case KT_ROT:
					type = CONTROLLER_QUAT;
					target = &target_obj->prs.rot;
				break;
				case KT_PRS:
					type = CONTROLLER_PRS;
					target = &target_obj->prs;
				break;
				}
			break;
			case ENTRY_CAMERA:
				target_cam = target_entry->entry;

				switch(field){
				case KT_POS:
					type = CONTROLLER_VECTOR;
					target = &target_cam->prs.pos;
				break;
				case KT_ROT:
					type = CONTROLLER_QUAT;
					target = &target_cam->prs.rot;
				break;
				case KT_PRS:
					type = CONTROLLER_PRS;
					target = &target_cam->prs;
				break;
				case KT_FOG_START:
					type = CONTROLLER_FLOAT;
					target = &target_cam->fog_start;
				break;
				case KT_FOG_END:
					type = CONTROLLER_FLOAT;
					target = &target_cam->fog_end;
				break;
				case KT_FOV:
					type = CONTROLLER_FLOAT;
					target = &target_cam->fov;
				break;
				}
			break;
			case ENTRY_LIGHT:
				target_light = target_entry->entry;

				switch(field){
				case KT_POS:
					type = CONTROLLER_VECTOR;
					target = &target_light->prs.pos;
				break;
				case KT_PRS:
					type = CONTROLLER_PRS;
					target = &target_light->prs;
				break;
				}
			break;
			}

			temp->controllers[i] = current_controller = make_controller(type,key_count,target);
			if(!current_controller) return NULL;

			for(j=0;j<key_count;j++){
				key newkey;
				memset(&newkey,0,sizeof(key));
				file_read(&newkey.time,sizeof(float),1,fp);

				switch(type){
				case CONTROLLER_VECTOR:
					file_read(&newkey.value.v,sizeof(vector),1,fp);
					set_controller_key(current_controller,j,newkey);
				break;
				case CONTROLLER_QUAT:
					file_read(&newkey.value.q,sizeof(quat),1,fp);
					set_controller_key(current_controller,j,newkey);
				break;
				case CONTROLLER_PRS:
					file_read(&newkey.value.prs,sizeof(prs),1,fp);
					set_controller_key(current_controller,j,newkey);
				break;
				case CONTROLLER_FLOAT:
					file_read(&newkey.value.f,sizeof(float),1,fp);
					set_controller_key(current_controller,j,newkey);
				break;
				}
			}
		}
	}

	file_close(fp);
	return temp;
}

void free_scene(scene *input){

	unsigned int i;
	for(i=0;i<input->object_count;i++){
		free_object(input->objects[i]);
	}

	free(input->objects);
	input->object_count = 0;

	free(input);
}

void animate_scene(scene *input, float time){
	unsigned int i;

	time = input->start + (float)fmod(time,input->end-input->start);
	for(i=0;i<input->controller_count;i++){
		update_controller(input->controllers[i],time);
	}

	/* rebuild objectmatrices */
	for(i=0;i<input->object_count;i++){
		matrix position, rotation, scale;
		object *obj = input->objects[i];

		matrix_translate(position, obj->prs.pos);
		matrix_from_quat(rotation, obj->prs.rot);
		matrix_scale(scale, obj->prs.scale);

		matrix_identity(obj->mat);
		matrix_multiply(obj->mat, obj->mat, position);
		matrix_multiply(obj->mat, obj->mat, rotation);
		matrix_multiply(obj->mat, obj->mat, scale);
	}

	/* rebuild cameramatrices */
	/* move to set_camera() routine later, no need to rebuild unused cameras */
	for(i=0;i<input->camera_count;i++){
		vector by, bz;
		matrix position, rotation, scale, temp;
		camera *cam = &input->cameras[i];

		matrix_translate(position, cam->prs.pos);
		matrix_from_quat(rotation, cam->prs.rot);
		matrix_scale(scale, cam->prs.scale);

		by = matrix_get_col(rotation,2);
		bz = matrix_get_col(rotation,1);
		matrix_set_col(rotation,1,by);
		matrix_set_col(rotation,2,vector_scale(bz,-1));

		matrix_identity(temp);
		matrix_multiply(temp, temp, position);
		matrix_multiply(temp, temp, rotation);
		matrix_multiply(temp, temp, scale);
		D3DXMatrixInverse((D3DMATRIX*)&cam->mat,NULL,(D3DMATRIX*)&temp);
	}
}

void set_light(IDirect3DDevice9 *device, int light_index, light *l){
		D3DLIGHT9 light;

		if(!l){
			IDirect3DDevice9_LightEnable(device,light_index,FALSE);
			return;
		}

		memset(&light,0,sizeof(D3DLIGHT9));
		light.Type = D3DLIGHT_POINT;
		light.Position.x = l->prs.pos.x;
		light.Position.y = l->prs.pos.y;
		light.Position.z = l->prs.pos.z;
		light.Diffuse.r = 1*400;
		light.Diffuse.g = 1*400;
		light.Diffuse.b = 1*400;
		light.Specular.r = 1;
		light.Specular.g = 1;
		light.Specular.b = 1;
		light.Ambient.r = 1;
		light.Ambient.g = 1;
		light.Ambient.b = 1;

		light.Range = (float)sqrt(FLT_MAX);

		light.Attenuation0 = 0.0f;
		light.Attenuation1 = 1.7f;
		light.Attenuation2 = 0.0f;

		IDirect3DDevice9_SetLight(device,light_index,&light);
		IDirect3DDevice9_LightEnable(device,light_index,TRUE);
}

void set_camera(IDirect3DDevice9 *device, camera *cam){
	vector by, bz;
	matrix position, rotation, scale, temp, view, projection;

	matrix_translate(position, cam->prs.pos);
	matrix_from_quat(rotation, cam->prs.rot);
	matrix_scale(scale, cam->prs.scale);

	by = matrix_get_col(rotation,2);
	bz = matrix_get_col(rotation,1);
	matrix_set_col(rotation,1,by);
	matrix_set_col(rotation,2,vector_scale(bz,-1));

	matrix_identity(temp);
	matrix_multiply(temp, temp, position);
	matrix_multiply(temp, temp, rotation);
	matrix_multiply(temp, temp, scale);
	D3DXMatrixInverse((D3DMATRIX*)&view,NULL,(D3DMATRIX*)&temp);

	matrix_project( projection, cam->fov, 4.f/3.f, cam->near_clip, cam->far_clip );

	IDirect3DDevice9_SetTransform( device, D3DTS_PROJECTION, (D3DMATRIX*)&projection );
	IDirect3DDevice9_SetTransform( device, D3DTS_VIEW, (D3DMATRIX*)&view );

	if(cam->fog){
		IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, make_col(cam->fog_color.r,cam->fog_color.g,cam->fog_color.b));
		IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
		IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD*) (&cam->fog_start)));
		IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD*) (&cam->fog_end)));

		IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
	}else{
		IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE );
	}
}

void draw_scene(IDirect3DDevice9 *device, scene *input, int cam, BOOL clear){
	unsigned int i;

	if(!input) return;
	if(!input->objects) return;

	if(cam>=0 && (unsigned)cam<input->camera_count) set_camera(device,&input->cameras[cam]);

	/* set ambient */
	IDirect3DDevice9_SetRenderState(device, D3DRS_AMBIENT, make_col(input->ambient.r, input->ambient.g, input->ambient.b ));

	if(clear){
		IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, make_col(input->background.r, input->background.g, input->background.b), 1.0f, 0);
	}

	if(input->light_count==0){
		IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
	}

	if(input->light_count<=8){
		for(i=0;i<input->light_count;i++) set_light(device,i,&input->lights[i]);
		for(;i<8;i++) set_light(device,i,NULL);
	}else{
		/* find de nærmeste lysene */

	}

	for(i=0;i<input->object_count;i++){
		draw_object_ex(device,input->objects[i],FALSE);
	}
	for(i=0;i<input->object_count;i++){
		draw_object_ex(device,input->objects[i],TRUE);
	}

	if(input->light_count==0){
		IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);
	}

}