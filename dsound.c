#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include "pest.h"

typedef HRESULT (WINAPI *DIRECTSOUNDCREATE) (GUID FAR *lpGUID, LPDIRECTSOUND FAR * lplpDS, IUnknown FAR *pUnkOuter);

LPDIRECTSOUND dsound=NULL;
LPDIRECTSOUNDBUFFER primary=NULL;
LPDIRECTSOUNDBUFFER secondary=NULL;

CRITICAL_SECTION critical;

volatile BOOL done = FALSE;
volatile BOOL playing = FALSE;
volatile BOOL ready_to_quit = TRUE;

volatile int last_known = 0;
volatile unsigned __int64 bytes_written = 0;

int pest_open( HWND win ){
	HMODULE lib;
	DIRECTSOUNDCREATE dsound_create;
	WAVEFORMATEX format;
	DSBUFFERDESC desc_primary, desc_secondary;

	lib = (HMODULE)LoadLibrary("dsound.dll");
	if(lib==NULL) return FALSE;

	dsound_create = (DIRECTSOUNDCREATE)GetProcAddress(lib, "DirectSoundCreate");
	if(dsound_create==NULL) return FALSE;
	FreeLibrary(lib);

	if( dsound_create( NULL, &dsound, NULL )!= DS_OK ) return FALSE;

	if( IDirectSound_SetCooperativeLevel( dsound, win, DSSCL_EXCLUSIVE | DSSCL_PRIORITY ) != DS_OK ) return FALSE;

	memset( &desc_primary, 0, sizeof(DSBUFFERDESC) );
	desc_primary.dwSize = sizeof(DSBUFFERDESC);
	desc_primary.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_STICKYFOCUS;
	
	if(IDirectSound_CreateSoundBuffer( dsound, &desc_primary, &primary, NULL )!=DS_OK) return FALSE;

	memset( &format, 0, sizeof(WAVEFORMATEX) );
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = 44100;
	format.nAvgBytesPerSec = 44100 * 4;
	format.nBlockAlign = 4;
	format.wBitsPerSample = 16;

	if( IDirectSoundBuffer_SetFormat( primary, &format )!= DS_OK) return FALSE;

	memset( &desc_secondary, 0, sizeof(DSBUFFERDESC) );
	desc_secondary.dwSize = sizeof(DSBUFFERDESC);
	desc_secondary.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	desc_secondary.lpwfxFormat = &format;
	desc_secondary.dwBufferBytes = 2*2*BUFFER_LEN;

	if(IDirectSound_CreateSoundBuffer( dsound, &desc_secondary, &secondary, NULL )!=DS_OK) return FALSE;

	InitializeCriticalSection(&critical);

	return TRUE;
}

void pest_internal_close(){
	done = TRUE;
	while(!ready_to_quit){
		Sleep(100);
	}

	if(primary!=NULL) IDirectSoundBuffer_Release( primary );
	if(secondary!=NULL) IDirectSoundBuffer_Release( secondary );
	if(dsound!=NULL) IDirectSound_Release( dsound );
}

extern float pest_time_offset;

float pest_get_pos(){
	unsigned __int64 time = 0;
	int play_pos;
	if(!playing) return 0;

	EnterCriticalSection(&critical);
	IDirectSoundBuffer_GetCurrentPosition( secondary, &play_pos, NULL );

	time += bytes_written;
	if( last_known <= play_pos ){
		time += play_pos - last_known;
	}else{
		time += (BUFFER_LEN*4)-(last_known-play_pos);
	}
	LeaveCriticalSection(&critical);
/*
	{
		char temp[256];
		sprintf( temp, "returning time: %f",  (float)((time/4)-(44100))*(1.f/44100.f)+pest_time_offset );
		OutputDebugString( temp );
	}
*/
	return (float)((time/4)-(44100))*(1.f/44100.f)+pest_time_offset;
}

int fill_buffer( void* buffer, int bytes );

DWORD WINAPI pest_thread_proc( LPVOID jalla ){
	short *buffer1;
	short *buffer2;
	int play_pos;
	DWORD len1;
	DWORD len2;

	ready_to_quit = FALSE;

	if(done) return 0;
	done = FALSE;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	IDirectSoundBuffer_Lock( secondary, 0, BUFFER_LEN*4, (void**)&buffer1, &len1, (void**)&buffer2, &len2, DSBLOCK_ENTIREBUFFER);
	bytes_written = fill_buffer( buffer1, len1 );
	if(buffer2) bytes_written+= fill_buffer( buffer2, len2 );

	EnterCriticalSection(&critical);
	last_known = (int)(bytes_written%(BUFFER_LEN*4));
	LeaveCriticalSection(&critical);

	IDirectSoundBuffer_Unlock(secondary,(void*)buffer1,len1,(void*)buffer2,len2);
	IDirectSoundBuffer_SetCurrentPosition(secondary, 0);
	IDirectSoundBuffer_Play(secondary, 0, 0, DSBPLAY_LOOPING);

	playing = TRUE;
	Sleep(100);

	while( !done )
	{
		short buffer[BUFFER_LEN*2];
		int to_write;
		DWORD written;

		IDirectSoundBuffer_GetCurrentPosition( secondary, &play_pos, NULL );

		if( last_known < play_pos ){
			to_write = play_pos - last_known;
			OutputDebugString( "Last known smaller than play pos" );
		}else{
			to_write = (BUFFER_LEN*4)-(last_known-play_pos);
		}

		while( IDirectSoundBuffer_Lock( secondary, last_known, to_write, (void**)&buffer1, &len1, (void**)&buffer2, &len2, 0)!=DS_OK){
			OutputDebugString("Trying to restore");
			IDirectSoundBuffer_Restore( secondary );
			IDirectSoundBuffer_Play( secondary, 0, 0, DSBPLAY_LOOPING);
		}

		written = fill_buffer( (short*)buffer, len1+len2 );
		memcpy(buffer1,buffer,len1);

		if(written>len1) memcpy(buffer2,(char*)buffer+len1,written-len1);

		IDirectSoundBuffer_Unlock( secondary, (void*)buffer1, len1, (void*)buffer2, len2 );

		EnterCriticalSection(&critical);
		bytes_written += written;
		last_known = (int)(bytes_written%(BUFFER_LEN*4));
		LeaveCriticalSection(&critical);
		Sleep(100);
	}

	playing=FALSE;
	ready_to_quit=TRUE;

	DeleteCriticalSection(&critical);

	return 0;
}