#include "video.h"
#include "evrdefs.h"
#include <assert.h>
#include <vfwmsgs.h>
#include "strsafe.h"

const WCHAR szDbmOk[] = L" OK ";
const WCHAR szDbmInfo[] = L"INFO";
const WCHAR szDbmWarn[] = L"WARN";
const WCHAR szDbmFail[] = L"FAIL";

typedef struct _tagCEVR {
  IBaseFilter* m_pEVR;
  IMFVideoDisplayControl* m_pVideoDisplay;
} CEVR, *LPCEVR;

static const WCHAR szModuleCEVR[] = L"CEVR";

void PrintDebug(const WCHAR szStatus[], const WCHAR szModule[],
    WCHAR szMessage[])
{
  WCHAR szDest[MAX_PATH];

  StringCchPrintf(szDest, MAX_PATH, L"[%s][%s] %s", szStatus, szModule,
      szMessage);
  OutputDebugString(szDest);
}

LPVOID CEVR_new()
{
  return calloc(1, sizeof(CEVR));
}

void CEVR_free(LPVOID pCEVR)
{
  if (!pCEVR)
    return;

  SAFE_RELEASE(((LPCEVR)pCEVR)->m_pEVR);
  SAFE_RELEASE(((LPCEVR)pCEVR)->m_pVideoDisplay);

  free(pCEVR);
}

static LPCEVR CEVR_getInstance(LPVOID pThis, PCWSTR szModule)
{
  LPCEVR pCEVR = (LPCEVR)pThis;
  assert(pCEVR);
  if (!pThis)
    PrintDebug(szDbmFail, szModule, L"Invalid 'this' pointer passed");
  return pCEVR;
}

static const WCHAR szModule_CEVR_HasVideo[] = L"CEVR::HasVideo";

HRESULT CEVR_HasVideo(LPVOID pThis)
{
  LPCEVR pCEVR = CEVR_getInstance(pThis, szModule_CEVR_HasVideo);
  if (!pCEVR)
    return E_FAIL;

  return (pCEVR->m_pVideoDisplay != NULL);
}

static const WCHAR szModule_CEVR_AddToGraph[] = L"CEVR::AddToGraph";

HRESULT CEVR_AddToGraph(LPVOID pThis, IGraphBuilder* pGraph, HWND hWnd)
{
  LPCEVR pCEVR = CEVR_getInstance(pThis, szModule_CEVR_AddToGraph);
  if (!pCEVR)
    return E_FAIL;

  HRESULT hr = E_FAIL;
  IBaseFilter* pEVR = NULL;

  hr = AddFilterByCLSID(pGraph, &CLSID_EnhancedVideoRenderer, &pEVR, L"EVR");
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_CEVR_AddToGraph,
        L"Unable to add EVR filter to graph");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_CEVR_AddToGraph,
      L"EVR filter added to the graph");

  hr = InitializeEVR(pEVR, hWnd, &pCEVR->m_pVideoDisplay);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModule_CEVR_AddToGraph, L"Can't initialize EVR");
    goto done;
  }
  PrintDebug(szDbmOk, szModule_CEVR_AddToGraph, L"EVR Initialized");

  /*
   *  Note: Because IMFVideoDisplayControl is a service interface, you cannot
   *  QI the pointer to get back the IBaseFilter pointer.
   *  Therefore, we need to cache the IBaseFilter pointer.
   */
  pCEVR->m_pEVR = pEVR;
  IBaseFilter_AddRef(pCEVR->m_pEVR);

done:

  SAFE_RELEASE(pEVR);
  return hr;
}

static const WCHAR szModule_CEVR_FinalizeGraph[] = L"CEVR::FinalizeGraph";

HRESULT CEVR_FinalizeGraph(LPVOID pThis, IGraphBuilder* pGraph)
{
  LPCEVR pCEVR = CEVR_getInstance(pThis, szModule_CEVR_FinalizeGraph);
  if (!pCEVR)
    return E_FAIL;

  HRESULT hr = E_FAIL;

  if (pCEVR->m_pEVR == NULL)
    return S_OK;

  BOOL bRemoved;
  hr = RemoveUnconnectedRenderer(pGraph, pCEVR->m_pEVR, &bRemoved);
  if (bRemoved)
  {
    SAFE_RELEASE(pCEVR->m_pEVR);
    SAFE_RELEASE(pCEVR->m_pVideoDisplay);
  }

  return hr;
}

static const WCHAR szModule_CEVR_UpdateVideoWindow[] =
    L"CEVR::UpdateVideoWindow";

HRESULT CEVR_UpdateVideoWindow(LPVOID pThis, HWND hWnd, const LPRECT prc)
{
  LPCEVR pCEVR = CEVR_getInstance(pThis, szModule_CEVR_UpdateVideoWindow);
  if (!pCEVR)
    return E_FAIL;

  /* no-op */
  if (pCEVR->m_pVideoDisplay == NULL)
    return S_OK;

  if (prc)
    return IMFVideoDisplayControl_SetVideoPosition(pCEVR->m_pVideoDisplay,
        NULL, prc);
  else
  {
    RECT rc;
    GetClientRect(hWnd, &rc);
    return IMFVideoDisplayControl_SetVideoPosition(pCEVR->m_pVideoDisplay,
        NULL, &rc);
  }
}

static const WCHAR szModule_CEVR_Repaint[] = L"CEVR::Repaint";

HRESULT CEVR_Repaint(LPVOID pThis, HWND hWnd, HDC hdc)
{
  UNREFERENCED_PARAMETER(hWnd)
  UNREFERENCED_PARAMETER(hdc)

  LPCEVR pCEVR = CEVR_getInstance(pThis, szModule_CEVR_Repaint);
  if (!pCEVR)
    return E_FAIL;

  if (pCEVR->m_pVideoDisplay)
    return IMFVideoDisplayControl_RepaintVideo(pCEVR->m_pVideoDisplay);
  else
    return S_OK;
}

static const WCHAR szModule_CEVR_DisplayModeChanged[] =
    L"CEVR::DisplayModeChanged";

HRESULT CEVR_DisplayModeChanged(LPVOID pThis)
{
  LPCEVR pCEVR = CEVR_getInstance(pThis, szModule_CEVR_DisplayModeChanged);
  if (!pCEVR)
    return E_FAIL;

  return S_OK;
}

HRESULT InitializeEVR(
    IBaseFilter* pEVR,  /* Pointer to the EVR */
    HWND hWnd,          /* Clipping window */
    IMFVideoDisplayControl** ppDisplayControl)
{
  HRESULT hr = E_FAIL;

  IMFGetService* pGS = NULL;
  IMFVideoDisplayControl* pDisplay = NULL;

  hr = IBaseFilter_QueryInterface(pEVR, &IID_IMFGetService, (LPVOID*)&pGS);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModuleCEVR, L"EVR Initialization: Can't "
        L"query IMFGetService");
    goto done;
  }
  PrintDebug(szDbmOk, szModuleCEVR, L"EVR Initialization: Acquired "
      L"IMFGetService");

  hr = IMFGetService_GetService(pGS, &MR_VIDEO_RENDER_SERVICE,
      &IID_IMFVideoDisplayControl, (LPVOID*)&pDisplay);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModuleCEVR, L"IMFGetService: Can't get "
        L"IMFVideoDisplayControl service");
    goto done;
  }
  PrintDebug(szDbmOk, szModuleCEVR, L"IMFGetService: Acquired "
      L"IMFVideoDisplayControl service");

  /* Set the clipping window */
  hr = IMFVideoDisplayControl_SetVideoWindow(pDisplay, hWnd);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModuleCEVR, L"pDisplay->SetVideoWindow(hWnd)");
    goto done;
  }
  PrintDebug(szDbmOk, szModuleCEVR, L"pDisplay->SetVideoWindow(hWnd)");

  /* Preserve aspect ratio by letter-boxing */
  hr = IMFVideoDisplayControl_SetAspectRatioMode(pDisplay,
      MFVideoARMode_PreservePicture);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModuleCEVR, L"pDisplay->SetAspectRatioMode()");
    goto done;
  }
  PrintDebug(szDbmOk, szModuleCEVR, L"pDisplay->SetAspectRatioMode()");

  /* Return the IMFVideoDisplayControl pointer to the caller */
  *ppDisplayControl = pDisplay;
  IMFVideoDisplayControl_AddRef(*ppDisplayControl);

done:
  SAFE_RELEASE(pGS);
  SAFE_RELEASE(pDisplay);
  return hr;
}

static const WCHAR szModulePinRemover[] = L"PinRemover";

HRESULT RemoveUnconnectedRenderer(
    IGraphBuilder* pGraph,
    IBaseFilter* pRenderer,
    BOOL *pbRemoved)
{
  HRESULT hr = E_FAIL;

  IPin* pPin = NULL;
  *pbRemoved = FALSE;

  /* Look for a connected input pin on the renderer. */
  hr = FindConnectedPin(pRenderer, PINDIR_INPUT, &pPin);
  SAFE_RELEASE(pPin);

  /*
   *  If this function succeeds, the renderer is connected,
   *  so we DON'T remove it.
   *
   *  If it fails, it means the renderer is not connected to anything,
   *  so we REMOVE It.
   */
  if (FAILED(hr))
  {
    PrintDebug(szDbmInfo, szModulePinRemover, L"Renderer not connected, "
        L"removing.");
    hr = IGraphBuilder_RemoveFilter(pGraph, pRenderer);
    *pbRemoved = TRUE;
  }
  else {
    PrintDebug(szDbmInfo, szModulePinRemover, L"Renderer connected, keeping.");
  }

  return hr;
}

HRESULT IsPinConnected(IPin* pPin, BOOL* pResult)
{
  HRESULT hr = E_FAIL;

  IPin* pTmp = NULL;
  hr = IPin_ConnectedTo(pPin, &pTmp);
  if (SUCCEEDED(hr))
    *pResult = TRUE;
  else if (hr == VFW_E_NOT_CONNECTED)
  {
    *pResult = FALSE;
    hr = S_OK;
  }

  SAFE_RELEASE(pTmp);
  return hr;
}

HRESULT IsPinDirection(IPin* pPin, PIN_DIRECTION dir, BOOL* pResult)
{
  HRESULT hr = E_FAIL;

  PIN_DIRECTION pinDir;
  hr = IPin_QueryDirection(pPin, &pinDir);
  if (SUCCEEDED(hr))
    *pResult = (pinDir == dir);

  return hr;
}

static const WCHAR szModulePinFinder[] = L"ConnectionFinder";

HRESULT FindConnectedPin(IBaseFilter* pFilter, PIN_DIRECTION PinDir,
    IPin** ppPin)
{
  HRESULT hr = E_FAIL;
  *ppPin = NULL;

  IEnumPins* pEnum = NULL;
  IPin* pPin = NULL;

  hr = IBaseFilter_EnumPins(pFilter, &pEnum);
  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModulePinFinder, L"Unable to enumerate filter "
        L"pins");
    /* Possibly memory leak */
    return hr;
  }
  PrintDebug(szDbmOk, szModulePinFinder, L"Filter pins enumerated");

  BOOL bFound = FALSE;
  while (S_OK == IEnumPins_Next(pEnum, 1, &pPin, NULL))
  {
    BOOL bIsConnected;
    hr = IsPinConnected(pPin, &bIsConnected);
    if (SUCCEEDED(hr) && bIsConnected)
    {
      PrintDebug(szDbmInfo, szModulePinFinder, L"Found possibly connected "
          L"pin");
      hr = IsPinDirection(pPin, PinDir, &bFound);
    }

    if (FAILED(hr))
    {
      PrintDebug(szDbmWarn, szModulePinFinder, L"No connected pins found");
      SAFE_RELEASE(pPin);
      break;
    }

    if (bFound)
    {
      PrintDebug(szDbmWarn, szModulePinFinder, L"Found actually connected "
          L"pin");
      *ppPin = pPin;
      break;
    }

    SAFE_RELEASE(pPin);
  }

  SAFE_RELEASE(pEnum);

  if (!bFound)
    hr = VFW_E_NOT_FOUND;

  return hr;
}

static const WCHAR szModuleAddByCLSID[] = L"AddFilterByCLSID";

HRESULT AddFilterByCLSID(IGraphBuilder* pGraph, REFGUID clsid,
    IBaseFilter** ppF, LPCWSTR wszName)
{
  HRESULT hr = E_FAIL;

  IBaseFilter *pFilter = NULL;
  *ppF = NULL;

  hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter,
      (LPVOID*)&pFilter);
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModuleAddByCLSID, L"Can't create IBaseFilter "
        L"instance");
    goto done;
  }
  PrintDebug(szDbmOk, szModuleAddByCLSID, L"IBaseFilter instance created");

  hr = IGraphBuilder_AddFilter(pGraph, pFilter, wszName);
  if (FAILED(hr))
  {
    PrintDebug(szDbmFail, szModuleAddByCLSID, L"Unable to add filter to "
        L"graph");
    goto done;
  }
  PrintDebug(szDbmOk, szModuleAddByCLSID, L"Added filter to graph");

  *ppF = pFilter;
  IBaseFilter_AddRef(*ppF);

done:
  SAFE_RELEASE(pFilter);

  return hr;
}
