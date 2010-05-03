#ifndef __MATERIAL_H__
#define __MATERIAL_H__

typedef struct{
	float r,g,b;
}color;

typedef struct{
	BOOL doublesided;
	BOOL wireframe;
	BOOL additive;

	float alpha;

	color specular;
	float specular_level;
	float shininess;

	int texturemap;
	int environmentmap;
	int selfilluminationmap;

	D3DMATERIAL9 material;
	IDirect3DStateBlock9 *state;
}material;

material *material_load(IDirect3DDevice9 *device, char *filename );
material *load_material_internal(IDirect3DDevice9 *device, char * filename);
void free_material(material *mat);
void free_materials();

void set_material(IDirect3DDevice9 *device, material* mat);

#endif /* __MATERIAL_H__ */