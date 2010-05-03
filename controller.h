#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include "vector.h"
#include "quat.h"

typedef struct{
	vector pos;
	quat rot;
}pr;

typedef struct{
	vector pos;
	quat rot;
	vector scale;
}prs;

typedef union{
	float f;
	vector v;
	quat q;
	pr pr;
	prs prs;
}keydata;

typedef struct{
	float time;
	keydata value;
}key;

#define CONTROLLER_UNKNOWN	0
#define CONTROLLER_FLOAT	1
#define CONTROLLER_VECTOR	2
#define CONTROLLER_QUAT		3
#define CONTROLLER_PR		4
#define CONTROLLER_PRS		5

typedef struct{
	unsigned int type;
	int key_count;
	key *keys;

	void *target;
}controller;

controller *make_controller(unsigned int type, unsigned int key_count, void *target);
void set_controller_key(controller *cont, int keynum, key inkey);
void update_controller(controller *cont,float time);

#endif /* __CONTOLLER_H__ */

