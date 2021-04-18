#define COBJMACROS
#include "video.h"
#include "playback.h"
#include <assert.h>

typedef struct _tagDSHOWPLAYER {
  EPlaybackState m_state;
  HWND           m_hWnd;
  IGraphBuilder* m_pGraph;
  IMediaControl* m_pControl;
  IMediaEventEx* m_pEvent;
  LPVOID         m_pVideo;
} DSHOWPLAYER, *LPDSHOWPLAYER;

extern const UINT WM_GRAPH_EVENT;

LPVOID DShowPlayer_new(HWND hWnd)
{
  LPDSHOWPLAYER pDPlayer = calloc(1, sizeof(DSHOWPLAYER));
  pDPlayer->m_state = STATE_NO_GRAPH;
  pDPlayer->m_hWnd = hWnd;

  return (LPVOID)pDPlayer;
}

void DShowPlayer_free(LPVOID pThis)
{
  if (!pThis)
    return;

  DShowPlayer_TearDownGraph(pThis);
  free(pThis);
}

static LPDSHOWPLAYER DShowPlayer_getInstance(LPVOID pThis,
    PCWSTR szModule)
{
  LPDSHOWPLAYER pDPlayer = (LPDSHOWPLAYER)pThis;
  assert(pDPlayer);
  if (!pDPlayer)
    PrintDebug(szDbmFail, szModule, L"Invalid 'this' pointer passed");
  return pDPlayer;
}

static const WCHAR szModule_DShowPlayer_SetState[] = L"DShowPlayet::SetState";

PCWSTR pszPlaybackStates[] = {
  L"STATE_NO_GRAPH",
  L"STATE_RUNNING",
  L"STATE_PAUSED",
  L"STATE_STOPPED",
};

HRESULT DShowPlayer_SetState(LPDSHOWPLAYER pDPlayer, EPlaybackState state)
{
  if (!pDPlayer)
    return E_FAIL;

  PCWSTR szState = NULL;

  if (state < 4)
    szState = pszPlaybackStates[state];
  else
    szState = L"<Unknown>";

  WCHAR szBuf[MAX_PATH];
  StringCchPrintf(szBuf, MAX_PATH, L"State changed to %s", szState);

  PrintDebug(szDbmInfo, szModule_DShowPlayer_SetState, szBuf);

  pDPlayer->m_state = state;
  return S_OK;
}

static const WCHAR szModule_DShowPlayer_State[] = L"DShowPlayer::State";

EPlaybackState DShowPlayer_State(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_State);
  if (!pDPlayer)
    return STATE_NO_GRAPH;

  return pDPlayer->m_state;
}

static const WCHAR szModulePlayer[] = L"DShowPlayer";

static const WCHAR szModule_DShowPlayer_OpenFile[] = L"DShowPlayer::OpenFile";
HRESULT DShowPlayer_OpenFile(LPVOID pThis, PCWSTR pszFileName)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_OpenFile);
  if (!pDPlayer)
    return E_FAIL;

  HRESULT hr = E_FAIL;

  hr = DShowPlayer_InitializeGraph(pDPlayer);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_OpenFile,
        L"OpenFile: Unable to initialize the graph");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_DShowPlayer_OpenFile,
      L"OpenFile: Graph initialized");

  /* Create a new filter graph. (This also closes the old one, if any.) */
  IBaseFilter* pSource = NULL;
  hr = IGraphBuilder_AddSourceFilter(pDPlayer->m_pGraph, pszFileName, NULL,
      &pSource);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_OpenFile,
        L"OpenFile: Unable to add source filter from file to graph");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_DShowPlayer_OpenFile,
      L"OpenFile: Add source filter from file to graph");

  hr = DShowPlayer_RenderStreams(pDPlayer, pSource);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_OpenFile,
        L"OpenFile: Unable to render streams");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_DShowPlayer_OpenFile,
      L"OpenFile: Streams rendered");

done:
  if (FAILED(hr))
  {
    DShowPlayer_TearDownGraph(pDPlayer);
  }

  SAFE_RELEASE(pSource);
  return hr;
}

static const WCHAR szModule_DShowPlayer_HandleGraphEvent[] =
    L"DShowPlayer::HandleGraphEvent";

HRESULT DShowPlayer_HandleGraphEvent(LPVOID pThis,
    GraphEventFN pfnOnGraphEvent)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_HandleGraphEvent);
  if (!pDPlayer)
    return E_FAIL;

  HRESULT hr = S_OK;

  if (!pDPlayer->m_pEvent)
    return E_UNEXPECTED;

  long evCode = 0;
  LONG_PTR param1 = 0, param2 = 0;

  /* Get the event from the queue */
  while (SUCCEEDED(IMediaEventEx_GetEvent(pDPlayer->m_pEvent, &evCode,
      &param1, &param2, 0)))
  {
    /* Invoke the callback */
    pfnOnGraphEvent(pDPlayer->m_hWnd, evCode, param1, param2);

    /* Free the event data */
    hr = IMediaEventEx_FreeEventParams(pDPlayer->m_pEvent, evCode, param1,
        param2);
    if (FAILED(hr))
    {
      PrintDebug(szDbmFail, szModule_DShowPlayer_HandleGraphEvent,
          L"Unable to free event params");
      break;
    }
    PrintDebug(szDbmOk, szModule_DShowPlayer_HandleGraphEvent,
        L"Event params freed");
  }

  return hr;
}

static const WCHAR szModule_DShowPlayer_Play[] = L"DShowPlayer::Play";

HRESULT DShowPlayer_Play(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_Play);
  if (!pDPlayer)
    return E_FAIL;

  PrintDebug(szDbmInfo, szModule_DShowPlayer_Play, L"Play requested");

  HRESULT hr = E_FAIL;

  if (pDPlayer->m_state != STATE_PAUSED && pDPlayer->m_state != STATE_STOPPED)
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_Play,
        L"Unable to play: Wrong state");
    return VFW_E_WRONG_STATE;
  }

  hr = IMediaControl_Run(pDPlayer->m_pControl);
  if (SUCCEEDED(hr))
  {
    PrintDebug(szDbmInfo, szModule_DShowPlayer_Play, L"Playing");
    DShowPlayer_SetState(pDPlayer, STATE_RUNNING);
  }

  return hr;
}

static const WCHAR szModule_DShowPlayer_Pause[] = L"DShowPlayer::Pause";

HRESULT DShowPlayer_Pause(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_Pause);
  if (!pDPlayer)
    return E_FAIL;

  PrintDebug(szDbmInfo, szModule_DShowPlayer_Pause, L"Pause requested");

  HRESULT hr = E_FAIL;

  if (pDPlayer->m_state != STATE_RUNNING)
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_Pause,
        L"Unable to pause: Wrong state");
    return VFW_E_WRONG_STATE;
  }

  hr = IMediaControl_Pause(pDPlayer->m_pControl);
  if (SUCCEEDED(hr))
  {
    PrintDebug(szDbmInfo, szModule_DShowPlayer_Pause, L"Paused");
    DShowPlayer_SetState(pDPlayer, STATE_PAUSED);
  }

  return hr;
}

static const WCHAR szModule_DShowPlayer_Stop[] = L"DShowPlayer::Stop";

HRESULT DShowPlayer_Stop(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_Stop);
  if (!pDPlayer)
    return E_FAIL;

  PrintDebug(szDbmInfo, szModule_DShowPlayer_Stop, L"Stop requested");

  HRESULT hr = E_FAIL;

  if (pDPlayer->m_state != STATE_RUNNING && pDPlayer->m_state != STATE_PAUSED)
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_Stop,
        L"Unable to stop: Wrong state");
    return VFW_E_WRONG_STATE;
  }

  hr = IMediaControl_Stop(pDPlayer->m_pControl);
  if (SUCCEEDED(hr))
  {
    PrintDebug(szDbmInfo, szModule_DShowPlayer_Stop, L"Stopped");
    DShowPlayer_SetState(pDPlayer, STATE_STOPPED);
  }

  return hr;
}

/* EVR/VMR functionality */
static const WCHAR szModule_DShowPlayer_HasVideo[] = L"DShowPlayer::HasVideo";

BOOL DShowPlayer_HasVideo(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_HasVideo);
  if (!pDPlayer)
    return FALSE;

  return (pDPlayer->m_pVideo && CEVR_HasVideo(pDPlayer->m_pVideo));
}

/* Sets the destination rectangle for the video. */
static const WCHAR szModule_DShowPlayer_UpdateVideoWindow[] =
    L"DShowPlayer::UpdateVideoWindow";

HRESULT DShowPlayer_UpdateVideoWindow(LPVOID pThis, const LPRECT prc)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_UpdateVideoWindow);
  if (!pDPlayer)
    return E_FAIL;

  if (pDPlayer->m_pVideo)
    return CEVR_UpdateVideoWindow(pDPlayer->m_pVideo, pDPlayer->m_hWnd, prc);

  return S_OK;
}

/*
 *  Repaints the video.
 *  Call this method when the application receives WM_PAINT.
 */
static const WCHAR szModule_DShowPlayer_Repaint[] = L"DShowPlayer::Repaint";

HRESULT DShowPlayer_Repaint(LPVOID pThis, HDC hdc)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_Repaint);
  if (!pDPlayer)
    return E_FAIL;

  if (pDPlayer->m_pVideo)
  {
    PrintDebug(szDbmOk, szModule_DShowPlayer_Repaint,
        L"Renderer present, request repaint");
    return CEVR_Repaint(pDPlayer->m_pVideo, pDPlayer->m_hWnd, hdc);
  }
  else
  {
    PrintDebug(szDbmWarn, szModule_DShowPlayer_Repaint,
        L"No-op. No renderer present.");
    return S_OK;
  }
}

/*
 *  Notifies the video renderer that the display mode changed.
 *  Call this method when the application receives WM_DISPLAYCHANGE.
 */
static const WCHAR szModule_DShowPlayer_DisplayModeChanged[] =
    L"DShowPlayer::DisplayModeChanged";

HRESULT DShowPlayer_DisplayModeChanged(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_DisplayModeChanged);
  if (!pDPlayer)
    return E_FAIL;

  if (pDPlayer->m_pVideo)
    return CEVR_DisplayModeChanged(pDPlayer->m_pVideo);
  else
    return S_OK;
}

/*
 *  Graph building
 *  Create a new filter graph.
 */
static const WCHAR szModule_DShowPlayer_InitializeGraph[] =
    L"DShowPlayer::InitializeGraph";

HRESULT DShowPlayer_InitializeGraph(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_InitializeGraph);
  if (!pDPlayer)
    return E_FAIL;

  HRESULT hr = E_FAIL;

  DShowPlayer_TearDownGraph(pDPlayer);

  /* Create the Filter Graph Manager */
  hr = CoCreateInstance(
      &CLSID_FilterGraph,
      NULL,
      CLSCTX_INPROC_SERVER,
      &IID_IGraphBuilder,
      (LPVOID*)&pDPlayer->m_pGraph);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_InitializeGraph,
        L"Unable to create IGraphBuilder instance");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_DShowPlayer_InitializeGraph,
      L"IGraphBuilder instance created");

  hr = IGraphBuilder_QueryInterface(
      pDPlayer->m_pGraph,
      &IID_IMediaControl,
      (LPVOID*)&pDPlayer->m_pControl);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_InitializeGraph,
        L"Unable to query IMediaControl");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_DShowPlayer_InitializeGraph,
      L"IMediaControl acquired");

  hr = IGraphBuilder_QueryInterface(
      pDPlayer->m_pGraph,
      &IID_IMediaEventEx,
      (LPVOID*)&pDPlayer->m_pEvent);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_InitializeGraph,
        L"Unable to query IMediaEventEx");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_DShowPlayer_InitializeGraph,
      L"IMediaEventEx acquired");

  hr = IMediaEventEx_SetNotifyWindow(
      pDPlayer->m_pEvent,
      (OAHWND)pDPlayer->m_hWnd,
      WM_GRAPH_EVENT,
      (LONG_PTR)NULL);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_DShowPlayer_InitializeGraph,
        L"Unable to set media notify window");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_DShowPlayer_InitializeGraph,
      L"InitializeGraph: Media notify window set");

  DShowPlayer_SetState(pDPlayer, STATE_STOPPED);

done:
  return hr;
}

static const WCHAR szModule_DShowPlayer_TearDownGraph[] =
    L"DShowPlayer::TearDownGraph";

void DShowPlayer_TearDownGraph(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_TearDownGraph);
  if (!pDPlayer)
    return;

  PrintDebug(szDbmFail, szModule_DShowPlayer_TearDownGraph,
      L"Tearing down");

  /* Stop sending event messages */
  if (pDPlayer->m_pEvent)
    IMediaEventEx_SetNotifyWindow(pDPlayer->m_pEvent, (OAHWND)NULL, 0,
        (LONG_PTR)NULL);

  SAFE_RELEASE(pDPlayer->m_pGraph);
  SAFE_RELEASE(pDPlayer->m_pControl);
  SAFE_RELEASE(pDPlayer->m_pEvent);

  free(pDPlayer->m_pVideo);
  pDPlayer->m_pVideo = NULL;

  DShowPlayer_SetState(pDPlayer, STATE_NO_GRAPH);
}

static const WCHAR szModule_DShowPlayer_CreateVideoRenderer[] =
    L"DShowPlayer::CreateVideoRenderer";

HRESULT DShowPlayer_CreateVideoRenderer(LPVOID pThis)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_CreateVideoRenderer);
  if (!pDPlayer)
    return E_FAIL;

  HRESULT hr = E_FAIL;

  pDPlayer->m_pVideo = CEVR_new();

  if (pDPlayer->m_pVideo == NULL)
    return E_OUTOFMEMORY;

  hr = CEVR_AddToGraph(pDPlayer->m_pVideo, pDPlayer->m_pGraph,
      pDPlayer->m_hWnd);
  if (SUCCEEDED(hr))
    return hr;

  CEVR_free(pDPlayer->m_pVideo);
  pDPlayer->m_pVideo = NULL;

  return hr;
}

/* Render the streams from a source filter. */
static const WCHAR szModule_DShowPlayer_RenderStreams[] =
    L"DShowPlayer::RenderStreams";

HRESULT DShowPlayer_RenderStreams(LPVOID pThis, IBaseFilter* pSource)
{
  LPDSHOWPLAYER pDPlayer = DShowPlayer_getInstance(pThis,
      szModule_DShowPlayer_RenderStreams);
  if (!pDPlayer)
    return E_FAIL;

  HRESULT hr = E_FAIL;

  BOOL bRenderedAnyPin = FALSE;

  IFilterGraph2* pGraph2 = NULL;
  hr = IMediaEventEx_QueryInterface(pDPlayer->m_pGraph, &IID_IFilterGraph2,
      (LPVOID*)&pGraph2);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModulePlayer, L"RenderStreams: Unable to query "
        L"IFilterGraph");
    goto done;
  }
  PrintDebug(szDbmOk, szModulePlayer, L"RenderStreams: IFilterGraph "
      L"acquired");

  /* Add the video renderer to the graph */
  hr = DShowPlayer_CreateVideoRenderer(pDPlayer);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModulePlayer, L"RenderStreams: Unable to create "
        L"video renderer");
    goto done;
  }
  PrintDebug(szDbmOk, szModulePlayer, L"RenderStreams: Video renderer "
      L"created");

  /* Add the DSound Renderer to the graph */
  IBaseFilter* pAudioRenderer = NULL;
  hr = AddFilterByCLSID(pDPlayer->m_pGraph, &CLSID_DSoundRender,
      &pAudioRenderer, L"Audio Rendrered");
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModulePlayer, L"RenderStreams: Unable to add "
        L"DSoundRenderer");
    goto done;
  }
  PrintDebug(szDbmOk, szModulePlayer, L"RenderStreams: DSoundRenderer added");

  /* Enumerate the pins on the source filter */
  IEnumPins* pEnum = NULL;
  hr = IBaseFilter_EnumPins(pSource, &pEnum);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModulePlayer, L"RenderStreams: Unable to enumerate pins");
    goto done;
  }
  PrintDebug(szDbmOk, szModulePlayer, L"RenderStreams: Pins enumerated");

  /* Loop through all the pins */
  IPin* pPin;
  while (S_OK == IEnumPins_Next(pEnum, 1, &pPin, NULL))
  {
    /*
     *  Try to render this pin.
     *  It's OK if we fail some pins, if at least one pin renders.
     */
    HRESULT hr2 = IFilterGraph2_RenderEx(pGraph2, pPin,
        AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);

    SAFE_RELEASE(pPin);

    if (SUCCEEDED(hr2))
    {
      PrintDebug(szDbmInfo, szModulePlayer, L"RenderStreams: Pin rendered");
      bRenderedAnyPin = TRUE;
    }
    else
      PrintDebug(szDbmFail, szModulePlayer, L"RenderStreams: No one pin "
          L"rendered");
  }

  hr = CEVR_FinalizeGraph(pDPlayer->m_pVideo, pDPlayer->m_pGraph);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModulePlayer, L"RenderStreams: Can't Finalize "
        L"EVR graph");
    goto done;
  }
  PrintDebug(szDbmOk, szModulePlayer, L"RenderStreams: EVR graph finalized");

  /* Remove the audio renderer, if not used. */
  BOOL bRemoved;
  hr = RemoveUnconnectedRenderer(pDPlayer->m_pGraph, pAudioRenderer, &bRemoved);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmOk, szModulePlayer, L"RenderStreams: Unable to remove "
        L"unconnected renderer");
    goto done;
  }
  PrintDebug(szDbmOk, szModulePlayer, L"RenderStreams: Removed unconnected "
      L"renderer");

done:
  SAFE_RELEASE(pEnum);
  SAFE_RELEASE(pAudioRenderer);
  SAFE_RELEASE(pGraph2);

  /*
   *  If we succeeded to this point, make sure we rendered at least one stream
   */
  if (SUCCEEDED(hr) && !bRenderedAnyPin)
  {
    PrintDebug(szDbmFail, szModulePlayer, L"RenderStreams: Cannot render"
        L"renderer");
    hr = VFW_E_CANNOT_RENDER;
  }

  return hr;
}
