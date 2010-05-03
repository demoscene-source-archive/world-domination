#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "vector.h"
#include "quat.h"

typedef float matrix[16];

void matrix_identity( matrix m );
void matrix_rotate( matrix m, const vector v );
void matrix_translate( matrix m, const vector v );
vector matrix_get_translation( const matrix m );
void matrix_scale( matrix m, const vector v );
void matrix_lookat( matrix m, vector position, vector target, float roll );
void matrix_project( matrix m, float fov, float aspect, float znear, float zfar );
void matrix_from_quat( matrix m, const quat q );

void matrix_texture_rotate( matrix m, const float r );
void matrix_texture_translate( matrix m, const float u, const float v );
void matrix_texture_scale( matrix m, const float u, const float v );


vector matrix_get_col( const matrix m, const int col );
void matrix_set_col( matrix m, const int col, const vector v );

void matrix_multiply(matrix target, const matrix m1, const matrix m2);
vector matrix_transformvector( const matrix m, const vector v );
vector matrix_rotatevector( const matrix m, const vector v );
void matrix_transpose( matrix dest, const matrix src );
vector matrix_inverserotatevector( const matrix m, const vector v );

#endif /* __MATRIX_H__ */