#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <dshow.h>
#include "video.h"

typedef enum _tagEPlaybackState {
  STATE_NO_GRAPH,
  STATE_RUNNING,
  STATE_PAUSED,
  STATE_STOPPED,
} EPlaybackState;

typedef void (CALLBACK *GraphEventFN)(HWND, long, LONG_PTR, LONG_PTR);

LPVOID  DShowPlayer_new(HWND);
void    DShowPlayer_free(LPVOID);

EPlaybackState DShowPlayer_State(LPVOID);

HRESULT DShowPlayer_OpenFile(LPVOID, PCWSTR);

HRESULT DShowPlayer_Play(LPVOID);
HRESULT DShowPlayer_Pause(LPVOID);
HRESULT DShowPlayer_Stop(LPVOID);

BOOL    DShowPlayer_HasVideo(LPVOID);
HRESULT DShowPlayer_UpdateVideoWindow(LPVOID, const LPRECT);
HRESULT DShowPlayer_Repaint(LPVOID, HDC hdc);
HRESULT DShowPlayer_DisplayModeChanged(LPVOID);

HRESULT DShowPlayer_HandleGraphEvent(LPVOID, GraphEventFN);

HRESULT DShowPlayer_InitializeGraph(LPVOID);
void    DShowPlayer_TearDownGraph(LPVOID);
HRESULT DShowPlayer_CreateVideoRenderer(LPVOID);
HRESULT DShowPlayer_RenderStreams(LPVOID, IBaseFilter*);

#endif  /* PLAYBACK_H */
