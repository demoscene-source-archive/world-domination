#ifndef __QUAT_H__
#define __QUAT_H__

typedef struct{
	float x, y, z, w;
}quat;

__inline quat quat_make( float x, float y, float z, float w ){
	quat temp;
	temp.x = x;
	temp.y = y;
	temp.z = z;
	temp.w = w;
	return temp;
}

__inline float quat_magnitude( quat q ){
	return (float)sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
}

__inline quat quat_scale(quat q, const float scale){
	quat temp;
	temp.x = q.x*scale;
	temp.y = q.y*scale;
	temp.z = q.z*scale;
	temp.w = q.w*scale;
	return temp;
}

__inline quat quat_normalize(quat q){
	quat temp;
	float scale = 1.f/quat_magnitude(q);
	temp.x = q.x*scale;
	temp.y = q.y*scale;
	temp.z = q.z*scale;
	temp.w = q.w*scale;
	return temp;
}

__inline quat quat_interpolate( quat from, quat to, float t){
	quat to1;
	quat temp;
	float omega, cosom, sinom, scale0, scale1;
	float prediv = 0.f;
	cosom =	from.x*to.x+
			from.y*to.y+
			from.z*to.z+
			from.w*to.w;


	if(cosom<0.f){
		cosom = -cosom;
		to1.x = -to.x;
		to1.y = -to.y;
		to1.z = -to.z;
		to1.w = -to.w;
	}else{
		to1.x = to.x;
		to1.y = to.y;
		to1.z = to.z;
		to1.w = to.w;
	}
	omega = (float)acos(cosom);
	sinom = (float)sin(omega);
	if(sinom!=0) prediv = 1.f/sinom;
	scale0 = (float)sin((1.f-t)*omega)*prediv;
	scale1 = (float)sin(t*omega)*prediv;

	temp.x = scale0*from.x + scale1*to1.x;
	temp.y = scale0*from.y + scale1*to1.y;
	temp.z = scale0*from.z + scale1*to1.z;
	temp.w = scale0*from.w + scale1*to1.w;
	return temp;
}

#endif /* __QUAT_H__ */
