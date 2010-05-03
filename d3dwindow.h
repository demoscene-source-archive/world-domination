#ifndef _D3DWINDOW_H_
#define _D3DWINDOW_H_

#ifdef __cplusplus
extern "C"{
#endif

extern HWND win;
IDirect3DDevice9 *d3dwin_open( char *title, unsigned int width, unsigned int height, D3DFORMAT format, BOOL fullscreen );
void d3dwin_close();

#ifdef __cplusplus
}
#endif

#endif /* _D3DWINDOW_H_ */
