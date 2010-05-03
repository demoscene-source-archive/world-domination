#ifndef _PEST_H_
#define _PEST_H_

#ifdef __cplusplus
extern "C"{
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* config */
#define BUFFER_LEN 44100

/* select only one of theese */
#define PEST_DSOUND
//#define PEST_WAVEOUT

/* functions */
int pest_open( HWND win );
int pest_load( char *filename, float time_offset );
void pest_play();
float pest_get_pos();
void pest_close();


#ifdef __cplusplus
}
#endif

#endif /* _PEST_H_ */