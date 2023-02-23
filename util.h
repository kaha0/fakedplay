#pragma once
#include "resource.h"

HMODULE __FDP_HM;

LRESULT WINAPI DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void GuidToStr(GUID* guid, char* buf);

int IsIPAddressInUNICODE(char* a);

//#ifdef _DEBUG
void ShowMemory(void* pv, long l, char* t);
void PrintMemory(void* pv, long l);
void myputs(char* c);
void myputd(unsigned int v);
int mystrlen(char* c);
//#endif

GUID IID_IUnknown;

GUID IID_IClassFactory;

/*<DirectPlay GUIDs*/
GUID CLSID_DirectPlay;

GUID IID_IDirectPlay2;
GUID IID_IDirectPlay2A;
GUID IID_IDirectPlay3;
GUID IID_IDirectPlay3A;
GUID IID_IDirectPlay4;
GUID IID_IDirectPlay4A;

GUID CLSID_DPLobby;

GUID IID_IDirectPlayLobby;
GUID IID_IDirectPlayLobbyA;
GUID IID_IDirectPlayLobby2;
GUID IID_IDirectPlayLobby2A;
GUID IID_IDirectPlayLobby3;
GUID IID_IDirectPlayLobby3A;

GUID DPSP_TCPIP;
GUID DPSP_Serial;
/*</DirectPlay GUIDs*/

/*<Data Type GUIDs>*/
GUID DATATYPE_TOTALSIZE;
GUID DATATYPE_DPSP;
GUID DATATYPE_IP_CSTRING;
GUID DATATYPE_IP_UNICODE;
/*</Data Type GUIDs>*/