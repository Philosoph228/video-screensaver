#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <scrnsave.h>
#include <assert.h>
#include "playback.h"

const UINT WM_GRAPH_EVENT = (WM_USER + 1);

LPVOID g_pDPlayer;

static const WCHAR szModuleAppWnd[] = L"AppWindow";

void OnPaint(HWND);
void OnSize(HWND);
void CALLBACK OnGraphEvent(HWND, long, LONG_PTR, LONG_PTR);
void NotifyError(HWND, PCWSTR);

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  if (WM_GRAPH_EVENT == message)
  {
    DShowPlayer_HandleGraphEvent(g_pDPlayer, OnGraphEvent);
  }

  switch (message)
  {
    case WM_CREATE:
    {
      HRESULT hr = E_FAIL;
      hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
      assert(SUCCEEDED(hr));
      if (FAILED(hr))
      {
        PrintDebug(szDbmFail, szModuleAppWnd, L"COM Initialization failed");
      }
      PrintDebug(szDbmOk, szModuleAppWnd, L"COM Initialized");

      g_pDPlayer = DShowPlayer_new(hWnd);
      if (g_pDPlayer == NULL)
      {
        PrintDebug(szDbmFail, szModuleAppWnd, L"Unalbe to create. Invalid "
            L"DPlayer object");
        return -1;
      }
      PrintDebug(szDbmOk, szModuleAppWnd, L"Created");

      hr = DShowPlayer_OpenFile(g_pDPlayer, L".\\eatery_loop.mp4");
      assert(SUCCEEDED(hr));
      if (FAILED(hr))
      {
        PrintDebug(szDbmFail, szModuleAppWnd, L"Unable to open file");
        return 1;
        break;
      }
      PrintDebug(szDbmOk, szModuleAppWnd, L"File opened");

      DShowPlayer_Play(g_pDPlayer);
    }
      return 1;
      break;
    case WM_PAINT:
      OnPaint(hWnd);
      break;
    case WM_SIZE:
      OnSize(hWnd);
      break;
    case WM_DESTROY:
      PrintDebug(szDbmInfo, szModuleAppWnd, L"Marked to destroy");
      DShowPlayer_free(g_pDPlayer);
      return 1;
      break;
    default:
      return DefScreenSaverProc(hWnd, message, wParam, lParam);
  }

  return 0;
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
  return TRUE;
}

void CALLBACK OnGraphEvent(HWND hWnd, long evCode, LONG_PTR param1,
    LONG_PTR param2)
{
  PrintDebug(szDbmInfo, szModuleAppWnd, L"OnGraphEvent callback Fired");

  UNREFERENCED_PARAMETER(param1);
  UNREFERENCED_PARAMETER(param2);

  switch (evCode)
  {
  case EC_COMPLETE:
  case EC_USERABORT:
    DShowPlayer_Stop(g_pDPlayer);
    break;

  case EC_ERRORABORT:
    NotifyError(hWnd, L"Playback error.");
    DShowPlayer_Stop(g_pDPlayer);
    break;
  }
}

void OnPaint(HWND hWnd)
{
  PAINTSTRUCT ps;
  HDC hdc;

  hdc = BeginPaint(hWnd, &ps);

  if (DShowPlayer_State(g_pDPlayer) != STATE_NO_GRAPH &&
      DShowPlayer_HasVideo(g_pDPlayer))
  {
    PrintDebug(szDbmOk, szModuleAppWnd, L"Painting frame");
    if (FAILED(DShowPlayer_Repaint(g_pDPlayer, hdc)))
      PrintDebug(szDbmFail, szModuleAppWnd, L"Failed to paint frame");
  }
  else
  {
    PrintDebug(szDbmWarn, szModuleAppWnd, L"No graph or video. Painting "
        L"background color");
    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
  }

  EndPaint(hWnd, &ps);
}

void OnSize(HWND hWnd)
{
  PrintDebug(szDbmInfo, szModuleAppWnd, L"Sized");

  if (g_pDPlayer)
  {
    RECT rc;
    GetClientRect(hWnd, &rc);

    DShowPlayer_UpdateVideoWindow(g_pDPlayer, &rc);
  }
}

void NotifyError(HWND hWnd, PCWSTR pszMessage)
{
  MessageBox(hWnd, pszMessage, L"Error", MB_OK | MB_ICONERROR);
}
