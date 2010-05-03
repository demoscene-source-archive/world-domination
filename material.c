#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#include "file.h"
#include "texturemanager.h"
#include "material.h"



typedef struct{
	material *mat;
	char *filename;
}material_node;

material_node *materials = NULL;
int materials_allocated = 0;
int materials_used = 0;

static void check_alloc(){
	if(!materials){
		materials_allocated = 64;
		materials = malloc(sizeof(material_node)*materials_allocated);
	}
	if(materials_used>=materials_allocated){
		materials_allocated *= 2;
		materials = realloc(materials,sizeof(material_node)*materials_allocated);
	}
}

material *load_material_internal(IDirect3DDevice9 *device, char *filename);

material *material_load(IDirect3DDevice9 *device, char *filename ){
	int i;

	if( strcmp(filename,"")==0 ) return NULL;

	for(i=0;i<materials_used;i++){
		if(strcmp(filename,materials[i].filename)==0){
			return materials[i].mat;
		}
	}

	check_alloc();

	materials[materials_used].mat = load_material_internal(device, filename);

	materials[materials_used].filename = strdup(filename);
	materials_used++;
	return materials[materials_used-1].mat;
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

material *load_material_internal(IDirect3DDevice9 *device, char *filename){
	char filepath[256];
	int texturestage = 0;
	char *texture_filename;
	char *dirname;
	material *temp;
	char test[4];
	file *fp = file_open(filename);

	printf("*** LOADING MATERIAL: %s\n",filename);
	if(!fp){
		return NULL;
	}

	file_read(test,1,4,fp);
	if(memcmp(test,"KRM0",4)!=0){
		file_close(fp);
		return NULL;
	}

	temp = (material*)malloc(sizeof(material));
	if(!temp){
		file_close(fp);
		return NULL;
	}

	temp->alpha = 1.f;

	memset(&temp->material,0,sizeof(D3DMATERIAL9));

	file_read(&temp->material.Ambient,1,sizeof(color),fp);
	file_read(&temp->material.Diffuse,1,sizeof(color),fp);
	file_read(&temp->specular,1,sizeof(color),fp);
	file_read(&temp->specular_level,1,sizeof(float),fp);
	file_read(&temp->shininess,1,sizeof(float),fp);

	file_read(&temp->doublesided,sizeof(BOOL),1,fp);
	file_read(&temp->wireframe,sizeof(BOOL),1,fp);
	file_read(&temp->additive,sizeof(BOOL),1,fp);


	dirname = get_dirname(filename);

	/* texturemap */
	texture_filename = file_loadstring(fp);
	sprintf(filepath,"%s%s",dirname,texture_filename);
	temp->texturemap = texture_load(device, filepath, FALSE);
	free(texture_filename);

	/* environmentmap */
	texture_filename = file_loadstring(fp);
	sprintf(filepath,"%s%s",dirname,texture_filename);
	temp->environmentmap = texture_load(device, filepath, FALSE);
	free(texture_filename);

	/* self-illuminationmap */
	texture_filename = file_loadstring(fp);
	sprintf(filepath,"%s%s",dirname,texture_filename);
	temp->selfilluminationmap = texture_load(device, filepath, FALSE);
	free(texture_filename);
	file_close(fp);


	free(dirname);

	/* lage stateblock! */
	IDirect3DDevice9_BeginStateBlock(device);

	if(temp->additive) temp->doublesided = TRUE;

	if(temp->doublesided) IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
	else IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_CCW);

	IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);
	IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
	IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);

	if(temp->additive){
		IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);

		IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_ONE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_ONE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
	}else{
		IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
		IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
	}

	if(temp->selfilluminationmap>=0){
		set_texture(device,texturestage,temp->selfilluminationmap);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU );
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLOROP, D3DTOP_ADD);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLORARG2, D3DTA_CURRENT);

		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

		texturestage++;
	}

	if(temp->texturemap>=0){
		set_texture(device,texturestage,temp->texturemap);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU );
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLOROP, D3DTOP_MODULATE);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLORARG2, D3DTA_CURRENT);

		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

		texturestage++;
	}

	if(temp->environmentmap>=0){
		set_texture(device,texturestage,temp->environmentmap);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR );
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLOROP, D3DTOP_ADD);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_COLORARG2, D3DTA_CURRENT);

		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

		texturestage++;
	}

	for(;texturestage<8;texturestage++){
		set_texture(device,texturestage,-1);
		IDirect3DDevice9_SetTextureStageState(device, texturestage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU );
	}

	if(IDirect3DDevice9_EndStateBlock(device,&temp->state)!=D3D_OK) return NULL;

	return temp;
}

void free_material(material *mat){
	if(!mat) return;
	if(mat->state) IDirect3DStateBlock9_Release(mat->state);
	mat->state = NULL;
	free(mat);

}

void free_materials(){
	int i;
	for(i=0;i<materials_used;i++){
		free_material(materials[i].mat);
		free(materials[i].filename);
	}
	free(materials);
	materials = NULL;
}

void set_material(IDirect3DDevice9 *device, material* mat){
	IDirect3DStateBlock9_Apply(mat->state);

	mat->material.Specular.r = mat->specular.r*mat->specular_level;
	mat->material.Specular.g = mat->specular.g*mat->specular_level;
	mat->material.Specular.b = mat->specular.b*mat->specular_level;
	mat->material.Power = mat->shininess;
	IDirect3DDevice9_SetMaterial(device,&mat->material);
}

