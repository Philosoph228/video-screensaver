#ifndef NYA_VIDEO_H
#define NYA_VIDEO_H

#define COBJMACROS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <dshow.h>
#include <evr.h>
#include <control.h>
#include "evrdefs.h"

#define SAFE_RELEASE(obj) \
if ((obj) != NULL) { \
  (obj)->lpVtbl->Release(obj); \
  (obj) = NULL; \
}

extern const WCHAR szDbmOk[];
extern const WCHAR szDbmInfo[];
extern const WCHAR szDbmWarn[];
extern const WCHAR szDbmFail[];

void PrintDebug(const WCHAR[], const WCHAR[], WCHAR[]);

LPVOID  CEVR_new();
void    CEVR_free(LPVOID);

HRESULT CEVR_HasVideo(LPVOID);
HRESULT CEVR_AddToGraph(LPVOID, IGraphBuilder*, HWND);
HRESULT CEVR_FinalizeGraph(LPVOID, IGraphBuilder*);
HRESULT CEVR_UpdateVideoWindow(LPVOID, HWND, const LPRECT);
HRESULT CEVR_Repaint(LPVOID, HWND, HDC);
HRESULT CEVR_DisplayModeChanged(LPVOID);

HRESULT InitializeEVR(IBaseFilter*, HWND, IMFVideoDisplayControl**);
HRESULT RemoveUnconnectedRenderer(IGraphBuilder*, IBaseFilter*, BOOL*);
HRESULT IsPinConnected(IPin*, BOOL*);
HRESULT IsPinDirection(IPin*, PIN_DIRECTION, BOOL*);
HRESULT FindConnectedPin(IBaseFilter* pFilter, PIN_DIRECTION, IPin**);
HRESULT AddFilterByCLSID(IGraphBuilder*, REFGUID, IBaseFilter**, LPCWSTR);

#endif  /* NYA_VIDEO_H */
