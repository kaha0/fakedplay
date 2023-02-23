#pragma once
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include<windows.h>
#include "fakedplay.h"

HRESULT __stdcall IDirectPlayLobby_QueryInterface(LPUNKNOWN This, IID* piid, LPVOID * ppvObj);
ULONG __stdcall IDirectPlayLobby_AddRef(LPUNKNOWN This);
ULONG __stdcall IDirectPlayLobby_Release(LPUNKNOWN This);
/*  IDirectPlayLobby Methods	*/
HRESULT __stdcall IDirectPlayLobby_Connect(LPUNKNOWN This, DWORD, LPDIRECTPLAY2 *, IUnknown FAR *);
HRESULT __stdcall IDirectPlayLobby_CreateAddress(LPUNKNOWN This, GUID*, GUID*, LPVOID, DWORD, LPVOID, LPDWORD);
HRESULT __stdcall IDirectPlayLobby_EnumAddress(LPUNKNOWN This, LPDPENUMADDRESSCALLBACK, LPVOID, DWORD, LPVOID);
HRESULT __stdcall IDirectPlayLobby_EnumAddressTypes(LPUNKNOWN This, LPDPLENUMADDRESSTYPESCALLBACK, GUID*, LPVOID, DWORD);
HRESULT __stdcall IDirectPlayLobby_EnumLocalApplications(LPUNKNOWN This, LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD);
HRESULT __stdcall IDirectPlayLobby_GetConnectionSettings(LPUNKNOWN This, DWORD, LPVOID, LPDWORD);
HRESULT __stdcall IDirectPlayLobby_ReceiveLobbyMessage(LPUNKNOWN This, DWORD, DWORD, LPDWORD, LPVOID, LPDWORD);
HRESULT __stdcall IDirectPlayLobby_RunApplication(LPUNKNOWN This, DWORD, LPDWORD, LPDPLCONNECTION, HANDLE);
HRESULT __stdcall IDirectPlayLobby_SendLobbyMessage(LPUNKNOWN This, DWORD, DWORD, LPVOID, DWORD);
HRESULT __stdcall IDirectPlayLobby_SetConnectionSettings(LPUNKNOWN This, DWORD, DWORD, LPDPLCONNECTION);
HRESULT __stdcall IDirectPlayLobby_SetLobbyMessageEvent(LPUNKNOWN This, DWORD, DWORD, HANDLE);
/*  IDirectPlayLobby2 Methods	*/
HRESULT __stdcall IDirectPlayLobby_CreateCompoundAddress(LPUNKNOWN This, LPDPCOMPOUNDADDRESSELEMENT,DWORD,LPVOID,LPDWORD);
/*  IDirectPlayLobby3 Methods	*/
HRESULT __stdcall IDirectPlayLobby_ConnectEx(LPUNKNOWN This, DWORD, IID* piid, LPVOID *, IUnknown FAR *);
HRESULT __stdcall IDirectPlayLobby_RegisterApplication(LPUNKNOWN This, DWORD, LPVOID);
HRESULT __stdcall IDirectPlayLobby_UnregisterApplication(LPUNKNOWN This, DWORD, GUID*);
HRESULT __stdcall IDirectPlayLobby_WaitForConnectionSettings(LPUNKNOWN This, DWORD);