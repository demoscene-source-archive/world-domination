#include "vector.h"
#include "quat.h"
#include "matrix.h"

#include <memory.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979f
#endif

#pragma intrinsic( cos, sin, tan )

/*
const matrix identity = {	1,0,0,0,
							0,1,0,0,
							0,0,1,0,
							0,0,0,1};
*/
void matrix_identity( matrix m ){
	m[0 ] = 1; m[4 ] = 0; m[8 ] = 0; m[12] = 0;
	m[1 ] = 0; m[5 ] = 1; m[9 ] = 0; m[13] = 0;
	m[2 ] = 0; m[6 ] = 0; m[10] = 1; m[14] = 0;
	m[3 ] = 0; m[7 ] = 0; m[11] = 0; m[15] = 1;
//	memcpy(m,identity,sizeof(matrix));
}

void matrix_rotate( matrix m, const vector v ){
	float cx, cy, cz, sx, sy, sz;
	cx = (float)cos(v.x);
	cy = (float)cos(v.y);
	cz = (float)cos(v.z);
	sx = (float)sin(v.x);
	sy = (float)sin(v.y);
	sz = (float)sin(v.z);

	m[0 ] = cy*cz;	m[4 ] = sx*sy*cz-cx*sz;	m[8 ] = cx*sy*cz+sx*sz;	m[12] = 0;
	m[1 ] = cy*sz;	m[5 ] = sx*sy*sz+cx*cz;	m[9 ] = cx*sy*sz-sx*cz;	m[13] = 0;
	m[2 ] = -sy;	m[6 ] = sx*cy;			m[10] = cx*cy;			m[14] = 0;
	m[3 ] = 0;		m[7 ] = 0;				m[11] = 0;				m[15] = 1;
}

void matrix_from_quat( matrix m, const quat q ){
	float xs = q.x + q.x;
	float ys = q.y + q.y;
	float zs = q.z + q.z;

	float wx = q.w * xs;
	float wy = q.w * ys;
	float wz = q.w * zs;

	float xx = q.x * xs;
	float xy = q.x * ys;
	float xz = q.x * zs;

	float yy = q.y * ys;
	float yz = q.y * zs;
	float zz = q.z * zs;

	m[0 ] = 1.0f - yy - zz;	m[4 ] = xy - wz;		m[8 ] = xz + wy;		m[12] = 0;
	m[1 ] = xy + wz;		m[5 ] = 1.0f - xx - zz;	m[9 ] = yz - wx;		m[13] = 0;
	m[2 ] = xz - wy;		m[6 ] = yz + wx;		m[10] = 1.0f - xx - yy;	m[14] = 0;
	m[3 ] = 0;				m[7 ] = 0;				m[11] = 0;				m[15] = 1;

}

void matrix_translate( matrix m, const vector v ){
	m[0 ] = 1;	m[4 ] = 0;	m[8 ] = 0;	m[12] = v.x;
	m[1 ] = 0;	m[5 ] = 1;	m[9 ] = 0;	m[13] = v.y;
	m[2 ] = 0;	m[6 ] = 0;	m[10] = 1;	m[14] = v.z;
	m[3 ] = 0;	m[7 ] = 0;	m[11] = 0;	m[15] = 1;
}

void matrix_lookat( matrix m, vector position, vector target, float roll ){
	vector up, forward, right;
	matrix temp;
    up = vector_make( (float)sin(roll), (float)-cos(roll), 0 );
	forward = vector_normalize(vector_sub(target,position));
	right = vector_normalize(vector_crossproduct(up,forward));
	up = vector_normalize(vector_crossproduct(right,forward));

	m[0 ] = right.x;	m[1 ] = up.x;	m[2 ] = forward.x;	m[3 ] = 0;
	m[4 ] = right.y;	m[5 ] = up.y;	m[6 ] = forward.y;	m[7 ] = 0;
	m[8 ] = right.z;	m[9 ] = up.z;	m[10] = forward.z;	m[11] = 0;
	m[12] = 0;			m[13] = 0;		m[14] = 0;			m[15] = 1;

	matrix_translate( temp, vector_scale(position, -1.f) );
	matrix_multiply( m, m,temp );

}

void matrix_scale( matrix m, const vector v ){
	m[0 ] = v.x;	m[1 ] = 0;		m[2 ] = 0;		m[3 ] = 0;
	m[4 ] = 0;		m[5 ] = v.y;	m[6 ] = 0;		m[7 ] = 0;
	m[8 ] = 0;		m[9 ] = 0;		m[10] = v.z;	m[11] = 0;
	m[12] = 0;		m[13] = 0;		m[14] = 0;		m[15] = 1;
}

void matrix_texture_rotate( matrix m, const float r ){
	float c, s;
	c = (float)cos(r);
	s = (float)sin(r);

	m[0 ] = c;	m[4 ] = -s;	m[8 ] = 0;	m[12] = 0;
	m[1 ] = s;	m[5 ] = c;	m[9 ] = 0;	m[13] = 0;
	m[2 ] = 0;	m[6 ] = 0;	m[10] = 1;	m[14] = 0;
	m[3 ] = 0;	m[7 ] = 0;	m[11] = 0;	m[15] = 1;
}

void matrix_texture_translate( matrix m, const float u, const float v ){
	m[0 ] = 1;	m[4 ] = 0;	m[8 ] = u;	m[12] = 0;
	m[1 ] = 0;	m[5 ] = 1;	m[9 ] = v;	m[13] = 0;
	m[2 ] = 0;	m[6 ] = 0;	m[10] = 1;	m[14] = 0;
	m[3 ] = 0;	m[7 ] = 0;	m[11] = 0;	m[15] = 1;
}

void matrix_texture_scale( matrix m, const float u, const float v ){
	m[0 ] = u;	m[1 ] = 0;	m[2 ] = 0;	m[3 ] = 0;
	m[4 ] = 0;	m[5 ] = v;	m[6 ] = 0;	m[7 ] = 0;
	m[8 ] = 0;	m[9 ] = 0;	m[10] = 1;	m[11] = 0;
	m[12] = 0;	m[13] = 0;	m[14] = 0;	m[15] = 1;
}


void matrix_project( matrix m, float fovy, float aspect, float zn, float zf ){
	float h, q;

	aspect = 1.f/aspect;
	fovy *= (M_PI/180);
	h = (float)(1.0/tan(fovy*0.5f));
	q = zf/(zf-zn);

	m[0 ] = h*aspect;	m[1 ] = 0;	m[2 ] = 0;		m[3 ] = 0;
	m[4 ] = 0;			m[5 ] = h;	m[6 ] = 0;		m[7 ] = 0;
	m[8 ] = 0;			m[9 ] = 0;	m[10] = q;		m[11] = 1;
	m[12] = 0;			m[13] = 0;	m[14] = -q*zn;	m[15] = 0;
}

vector matrix_get_translation( const matrix m ){
	return vector_make( m[12], m[13], m[14] );
}

vector matrix_get_col( const matrix m, const int col ){
	return vector_make(
		m[(col*4)+0],
		m[(col*4)+1],
		m[(col*4)+2]);
}

void matrix_set_col( matrix m, const int col, const vector v ){
	m[(col*4)+0]	= v.x;
	m[(col*4)+1]	= v.y;
	m[(col*4)+2]	= v.z;
}

vector matrix_transformvector( const matrix m, const vector v ){
	vector temp;
	temp.x = v.x*m[0] + v.y*m[4] + v.z*m[8] + m[12];
	temp.y = v.x*m[1] + v.y*m[5] + v.z*m[9] + m[13];
	temp.z = v.x*m[2] + v.y*m[6] + v.z*m[10] + m[14];
	return temp;
}

vector matrix_rotatevector( const matrix m, const vector v ){
	vector temp;
	temp.x = v.x*m[0] + v.y*m[4] + v.z*m[8];
	temp.y = v.x*m[1] + v.y*m[5] + v.z*m[9];
	temp.z = v.x*m[2] + v.y*m[6] + v.z*m[10];
	return temp;
}

vector matrix_inverserotatevector( const matrix m, const vector v ){
	vector temp;
	temp.x = v.x*m[0] + v.y*m[1] + v.z*m[2];
	temp.y = v.x*m[4] + v.y*m[5] + v.z*m[6];
	temp.z = v.x*m[8] + v.y*m[9] + v.z*m[10];
	return temp;
}

void matrix_transpose( matrix dest, const matrix src ){
	dest[0 ] = src[0];	dest[1 ] = src[4];	dest[2 ] = src[8];	dest[3 ] = src[12];
	dest[4 ] = src[1];	dest[5 ] = src[5];	dest[6 ] = src[9];	dest[7 ] = src[13];
	dest[8 ] = src[2];	dest[9 ] = src[6];	dest[10] = src[10];	dest[11] = src[14];
	dest[12] = src[3];	dest[13] = src[7];	dest[14] = src[11];	dest[15] = src[15];
}

void matrix_multiply(matrix target, const matrix m1, const matrix m2){
	int i, j, counter;
	matrix temp;
	for( i=0; i<4; i++ )
		for ( j=0; j<4; j++ )
			temp[i+(j<<2)] = (m1[i]*m2[(j<<2)])
							+(m1[i+(1<<2)]*m2[1+(j<<2)])
							+(m1[i+(2<<2)]*m2[2+(j<<2)])
							+(m1[i+(3<<2)]*m2[3+(j<<2)]);

	for( counter=0; counter<(4*4); counter++ ) target[counter] = temp[counter];
}
