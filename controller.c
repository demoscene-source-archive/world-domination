#include <stdlib.h>
#include "controller.h"

__inline float lerp( float left, float right, float t ){
	return left + (right-left)*t;
}

controller *make_controller(unsigned int type, unsigned int key_count, void *target){
	controller *temp = malloc(sizeof(controller));

	temp->type = type;
	temp->key_count = key_count;
	temp->keys = malloc(sizeof(key)*temp->key_count);
	if(!temp->keys) return NULL;

	temp->target = target;

	return temp;
}

void set_controller_key(controller *cont, int keynum, key inkey){
	if(!cont) return;
	if((keynum<cont->key_count) && (keynum>=0)){
		cont->keys[keynum] = inkey;
	}
}

void update_controller(controller *cont,float time){
	key *left, *right;
	float t, div_me;
	int current, next;

	if(cont->target==NULL) return;

	for(current=0;current<cont->key_count;current++){
		next = (current+1)%cont->key_count;
		left = &cont->keys[current];
		right = &cont->keys[next];
		if((left->time<=time)&&(right->time>time)) break;
	}

	div_me = right->time-left->time;
	if(div_me!=0.f) t = (time - left->time) / div_me;

	if(t<0.f) t = 0.f;
	if(t>1.f) t = 1.f;

	switch(cont->type){

	case CONTROLLER_FLOAT:
		*((float*)cont->target) = lerp(left->value.f,right->value.f,t);
	break;

	case CONTROLLER_VECTOR:
		*((vector*)cont->target) = vector_lerp(left->value.v,right->value.v,t);
	break;

	case CONTROLLER_QUAT:
		*((quat*)cont->target) = quat_interpolate(left->value.q, right->value.q,t);
	break;

	case CONTROLLER_PRS:
		((prs*)cont->target)->pos = vector_lerp(left->value.v,right->value.v,t);
		((prs*)cont->target)->rot = quat_interpolate(left->value.prs.rot, right->value.prs.rot, t);
	break;

	}
}