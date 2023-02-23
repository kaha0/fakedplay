#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include<windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include "IDirectPlayFunctions.h"
#include "IDirectPlayLobbyFunctions.h"
#include "util.h"

unsigned long gdwDPlaySPRefCount = 0;  // is a global "refcount" variable used by modules for specific Service Providers! see dpmodemx.dll, dpwsockx.dll

int wsainitialized = 0;
IClassFactoryVtbl * __ICF_VTBL = NULL;
IDirectPlay4Vtbl * __IDP4_VTBL = NULL;
IDirectPlayLobby3Vtbl * __IDPL3_VTBL = NULL;

// https://docs.microsoft.com/en-us/windows/win32/com/functions
// https://docs.microsoft.com/en-us/windows/win32/learnwin32/creating-an-object-in-com
// https://docs.microsoft.com/en-us/windows/win32/learnwin32/com-coding-practices

// https://docs.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-coinitialize
// https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-cocreateinstance

HRESULT __stdcall IClassFactory_QueryInterface(IClassFactory * This, IID * riid, void **ppvObject)
{//https://docs.microsoft.com/en-us/cpp/atl/queryinterface?view=vs-2019
 //https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-queryinterface%28refiid_void%29
	char* buf = malloc(39);
	GuidToStr(riid, buf);
	MessageBox(GetForegroundWindow(), buf, "IClassFactory_QueryInterface IID:", 0);
	free(buf);
	return E_NOINTERFACE;
}

ULONG __stdcall IClassFactory_AddRef(IClassFactory * This)
{//https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-addref
#ifdef _DEBUG
	ShowMemory(&This->dwRefCount, 4, "IClassFactory_AddRef (before incrementing)");
#endif
	return ++This->dwRefCount;
}

ULONG __stdcall IClassFactory_Release(IClassFactory * This)
{//https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release
#ifdef _DEBUG
	ShowMemory(&This->dwRefCount, 4, "IClassFactory_Release (before decreasing)");
#endif
	--This->dwRefCount;
	if (This->dwRefCount == 0)
	{
		free(This);
#ifdef _DEBUG
		MessageBoxA(GetForegroundWindow(), "IClassFactory_Release (object freed)", "IClassFactory_Release (object freed)", 0);
#endif
		return 0;
	}
	return This->dwRefCount;
}

HRESULT __stdcall IClassFactory_CreateInstance(IClassFactory * This, IUnknown *pUnk, IID * riid, void **ppv)
{//https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iclassfactory-createinstance
	if (IsEqualGUID(riid, &IID_IDirectPlay4A) ||
		IsEqualGUID(riid, &IID_IDirectPlay3A) ||
		IsEqualGUID(riid, &IID_IDirectPlay2A))
	{
		IDirectPlay4* idp4 = malloc(sizeof(IDirectPlay4));
#ifdef _DEBUG
		char* buf = malloc(39);
		GuidToStr(riid, buf);
		MessageBoxA(GetForegroundWindow(), buf, "IClassFactory_CreateInstance(IID_IDirectPlayA)", 0);
		free(buf);
#endif
		idp4->lpVtbl = __IDP4_VTBL;
		IDirectPlay_Constructor(idp4, FALSE);
		*ppv = idp4;
		return S_OK;
	}
	else if (IsEqualGUID(riid, &IID_IDirectPlay4) ||
		IsEqualGUID(riid, &IID_IDirectPlay3) ||
		IsEqualGUID(riid, &IID_IDirectPlay2))
	{
		// unicode
		IDirectPlay4* idp4 = malloc(sizeof(IDirectPlay4));
#ifdef _DEBUG
		char* buf = malloc(39);
		GuidToStr(riid, buf);
		MessageBoxA(GetForegroundWindow(), buf, "IClassFactory_CreateInstance(IID_IDirectPlay)", 0);
		free(buf);
#endif
		idp4->lpVtbl = __IDP4_VTBL;
		IDirectPlay_Constructor(idp4, TRUE);
		*ppv = idp4;
		return S_OK;
	}
	else if (IsEqualGUID(riid, &IID_IDirectPlayLobby3A) ||
		IsEqualGUID(riid, &IID_IDirectPlayLobby3) ||
		IsEqualGUID(riid, &IID_IDirectPlayLobby2A) ||
		IsEqualGUID(riid, &IID_IDirectPlayLobby2) ||
		IsEqualGUID(riid, &IID_IDirectPlayLobbyA) ||
		IsEqualGUID(riid, &IID_IDirectPlayLobby))
	{
		IDirectPlayLobby3* idpl3 = malloc(sizeof(IDirectPlayLobby3));
#ifdef _DEBUG
		char* buf = malloc(39);
		GuidToStr(riid, buf);
		MessageBoxA(GetForegroundWindow(), buf, "IClassFactory_CreateInstance(IID_IDirectPlayLobby)", 0);
		free(buf);
#endif
		idpl3->lpVtbl = __IDPL3_VTBL;
		idpl3->dwRefCount = 1;
		*ppv = idpl3;
		return S_OK;
	}
	else
	{
		char* buf = malloc(39);
		GuidToStr(riid, buf);
		MessageBox(GetForegroundWindow(), buf, "IClassFactory_CreateInstance requests IID:", 0);
		free(buf);
		return E_NOINTERFACE;
	}
}

HRESULT __stdcall IClassFactory_LockServer(IClassFactory * This, BOOL fLock)
{//https://docs.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iclassfactory-lockserver
	MessageBox(GetForegroundWindow(), "IClassFactory_LockServer", "IClassFactory_LockServer", 0);
	return S_OK;
}

BOOL init_vtables()
{
	__IDP4_VTBL = malloc(sizeof(IDirectPlay4Vtbl));
	if (__IDP4_VTBL == NULL) return FALSE;
	__IDP4_VTBL->AddGroupToGroup = IDirectPlay_AddGroupToGroup;
	__IDP4_VTBL->AddPlayerToGroup = IDirectPlay_AddPlayerToGroup;
	__IDP4_VTBL->AddRef = IDirectPlay_AddRef;
	__IDP4_VTBL->CancelMessage = IDirectPlay_CancelMessage;
	__IDP4_VTBL->CancelPriority = IDirectPlay_CancelPriority;
	__IDP4_VTBL->Close = IDirectPlay_Close;
	__IDP4_VTBL->CreateGroup = IDirectPlay_CreateGroup;
	__IDP4_VTBL->CreateGroupInGroup = IDirectPlay_CreateGroupInGroup;
	__IDP4_VTBL->CreatePlayer = IDirectPlay_CreatePlayer;
	__IDP4_VTBL->DeleteGroupFromGroup = IDirectPlay_DeleteGroupFromGroup;
	__IDP4_VTBL->DeletePlayerFromGroup = IDirectPlay_DeletePlayerFromGroup;
	__IDP4_VTBL->DestroyGroup = IDirectPlay_DestroyGroup;
	__IDP4_VTBL->DestroyPlayer = IDirectPlay_DestroyPlayer;
	__IDP4_VTBL->EnumConnections = IDirectPlay_EnumConnections;
	__IDP4_VTBL->EnumGroupPlayers = IDirectPlay_EnumGroupPlayers;
	__IDP4_VTBL->EnumGroups = IDirectPlay_EnumGroups;
	__IDP4_VTBL->EnumGroupsInGroup = IDirectPlay_EnumGroupsInGroup;
	__IDP4_VTBL->EnumPlayers = IDirectPlay_EnumPlayers;
	__IDP4_VTBL->EnumSessions = IDirectPlay_EnumSessions;
	__IDP4_VTBL->GetCaps = IDirectPlay_GetCaps;
	__IDP4_VTBL->GetGroupConnectionSettings = IDirectPlay_GetGroupConnectionSettings;
	__IDP4_VTBL->GetGroupData = IDirectPlay_GetGroupData;
	__IDP4_VTBL->GetGroupFlags = IDirectPlay_GetGroupFlags;
	__IDP4_VTBL->GetGroupName = IDirectPlay_GetGroupName;
	__IDP4_VTBL->GetGroupOwner = IDirectPlay_GetGroupOwner;
	__IDP4_VTBL->GetGroupParent = IDirectPlay_GetGroupParent;
	__IDP4_VTBL->GetMessageCount = IDirectPlay_GetMessageCount;
	__IDP4_VTBL->GetMessageQueue = IDirectPlay_GetMessageQueue;
	__IDP4_VTBL->GetPlayerAccount = IDirectPlay_GetPlayerAccount;
	__IDP4_VTBL->GetPlayerAddress = IDirectPlay_GetPlayerAddress;
	__IDP4_VTBL->GetPlayerCaps = IDirectPlay_GetPlayerCaps;
	__IDP4_VTBL->GetPlayerData = IDirectPlay_GetPlayerData;
	__IDP4_VTBL->GetPlayerFlags = IDirectPlay_GetPlayerFlags;
	__IDP4_VTBL->GetPlayerName = IDirectPlay_GetPlayerName;
	__IDP4_VTBL->GetSessionDesc = IDirectPlay_GetSessionDesc;
	__IDP4_VTBL->Initialize = IDirectPlay_Initialize;
	__IDP4_VTBL->InitializeConnection = IDirectPlay_InitializeConnection;
	__IDP4_VTBL->Open = IDirectPlay_Open;
	__IDP4_VTBL->QueryInterface = IDirectPlay_QueryInterface;
	__IDP4_VTBL->Receive = IDirectPlay_Receive;
	__IDP4_VTBL->Release = IDirectPlay_Release;
	__IDP4_VTBL->SecureOpen = IDirectPlay_SecureOpen;
	__IDP4_VTBL->Send = IDirectPlay_Send;
	__IDP4_VTBL->SendChatMessage = IDirectPlay_SendChatMessage;
	__IDP4_VTBL->SendEx = IDirectPlay_SendEx;
	__IDP4_VTBL->SetGroupConnectionSettings = IDirectPlay_SetGroupConnectionSettings;
	__IDP4_VTBL->SetGroupData = IDirectPlay_SetGroupData;
	__IDP4_VTBL->SetGroupName = IDirectPlay_SetGroupName;
	__IDP4_VTBL->SetGroupOwner = IDirectPlay_SetGroupOwner;
	__IDP4_VTBL->SetPlayerData = IDirectPlay_SetPlayerData;
	__IDP4_VTBL->SetPlayerName = IDirectPlay_SetPlayerName;
	__IDP4_VTBL->SetSessionDesc = IDirectPlay_SetSessionDesc;
	__IDP4_VTBL->StartSession = IDirectPlay_StartSession;

	__IDPL3_VTBL = malloc(sizeof(IDirectPlayLobby3Vtbl));
	if (__IDPL3_VTBL == NULL) return FALSE;
	__IDPL3_VTBL->AddRef = IDirectPlayLobby_AddRef;
	__IDPL3_VTBL->Connect = IDirectPlayLobby_Connect;
	__IDPL3_VTBL->ConnectEx = IDirectPlayLobby_ConnectEx;
	__IDPL3_VTBL->CreateAddress = IDirectPlayLobby_CreateAddress;
	__IDPL3_VTBL->CreateCompoundAddress = IDirectPlayLobby_CreateCompoundAddress;
	__IDPL3_VTBL->EnumAddress = IDirectPlayLobby_EnumAddress;
	__IDPL3_VTBL->EnumAddressTypes = IDirectPlayLobby_EnumAddressTypes;
	__IDPL3_VTBL->EnumLocalApplications = IDirectPlayLobby_EnumLocalApplications;
	__IDPL3_VTBL->GetConnectionSettings = IDirectPlayLobby_GetConnectionSettings;
	__IDPL3_VTBL->QueryInterface = IDirectPlayLobby_QueryInterface;
	__IDPL3_VTBL->ReceiveLobbyMessage = IDirectPlayLobby_ReceiveLobbyMessage;
	__IDPL3_VTBL->RegisterApplication = IDirectPlayLobby_RegisterApplication;
	__IDPL3_VTBL->Release = IDirectPlayLobby_Release;
	__IDPL3_VTBL->RunApplication = IDirectPlayLobby_RunApplication;
	__IDPL3_VTBL->SendLobbyMessage = IDirectPlayLobby_SendLobbyMessage;
	__IDPL3_VTBL->SetConnectionSettings = IDirectPlayLobby_SetConnectionSettings;
	__IDPL3_VTBL->SetLobbyMessageEvent = IDirectPlayLobby_SetLobbyMessageEvent;
	__IDPL3_VTBL->UnregisterApplication = IDirectPlayLobby_UnregisterApplication;
	__IDPL3_VTBL->WaitForConnectionSettings = IDirectPlayLobby_WaitForConnectionSettings;

	__ICF_VTBL = malloc(sizeof(IClassFactoryVtbl));
	if (__ICF_VTBL == NULL) return FALSE;
	__ICF_VTBL->AddRef = IClassFactory_AddRef;
	__ICF_VTBL->CreateInstance = IClassFactory_CreateInstance;
	__ICF_VTBL->LockServer = IClassFactory_LockServer;
	__ICF_VTBL->QueryInterface = IClassFactory_QueryInterface;
	__ICF_VTBL->Release = IClassFactory_Release;

	return TRUE;
}

STDAPI DirectPlayCreate(LPGUID lpGUID, LPDIRECTPLAY *lplpDP, IUnknown *pUnk)  // DirectPlay 1
{  // lpGUID == {0-0-0-0}, pUnk == NULL
	MessageBox(GetForegroundWindow(), "DirectPlayCreate", "DirectPlayCreate", 0);
	return E_FAIL;
}
STDAPI DirectPlayEnumerate(LPDPENUMDPCALLBACKA lpEnumCallback, LPVOID pv)  // Outlaws (olwin.exe) mi dá jako callback cdecl fci! ale skuteènì to má být stdcall
{  // tato fce má vracet POUZE serial connection, nic dalšího!
	MessageBoxA(GetForegroundWindow(), "DirectPlayEnumerate", "DirectPlayEnumerate", 0);
	lpEnumCallback(&DPSP_Serial, "Serial Connection For DirectPlay", 6, 0, pv);
	return S_OK;
}
STDAPI DirectPlayEnumerateA(LPDPENUMDPCALLBACKA lpEnumCallback, LPVOID pv)
{
	MessageBoxA(GetForegroundWindow(), "DirectPlayEnumerateA", "DirectPlayEnumerateA", 0);
	lpEnumCallback(&DPSP_Serial, "Serial Connection For DirectPlay", 6, 0, pv);
	return S_OK;
}
STDAPI DirectPlayEnumerateW(LPDPENUMDPCALLBACK lpEnumCallback, LPVOID pv)
{
	MessageBoxA(GetForegroundWindow(), "DirectPlayEnumerateW", "DirectPlayEnumerateW", 0);
	lpEnumCallback(&DPSP_Serial, L"Serial Connection For DirectPlay", 6, 0, pv);
	MessageBoxA(GetForegroundWindow(), "DirectPlayEnumerateW exiting", "DirectPlayEnumerateW exiting", 0);
	return S_OK;
}
STDAPI DirectPlayLobbyCreateA(LPGUID guid, LPDIRECTPLAYLOBBYA * lpdpLobby, IUnknown *pUnk, LPVOID pv, DWORD dw)
{
	MessageBox(GetForegroundWindow(), "DirectPlayLobbyCreateA", "DirectPlayLobbyCreateA", 0);
	return E_FAIL;
}
STDAPI DirectPlayLobbyCreateW(LPGUID guid, LPDIRECTPLAYLOBBY * lpdpLobby, IUnknown *pUnk, LPVOID pv, DWORD dw)
{
	MessageBox(GetForegroundWindow(), "DirectPlayLobbyCreateW", "DirectPlayLobbyCreateW", 0);
	return E_FAIL;
}
STDAPI DllCanUnloadNow(void)
//https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-dllcanunloadnow
{
	return S_FALSE;  // return S_OK if DLL is not in use so it can be unloaded, but we can just keep it in memory the whole time
}
STDAPI DllGetClassObject(CLSID *clsid, IID *iid, LPVOID *ppv)
//https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-dllgetclassobject
{
	if (!wsainitialized)
	{
		WORD wsa_ver = 0x202;
		WSADATA wsadata;
		if (WSAStartup(wsa_ver, &wsadata)) MessageBox(GetForegroundWindow(), "WSAStartup() failed! Things probably will not work!", "WSAStartup() failed!", MB_ICONERROR);
		// I should technically call WSACleanup(); at some point... but NOT in DllMain on DLL_PROCESS_DETACH
		// where then?
		wsainitialized = 1;
	}

	if (IsEqualGUID(&CLSID_DirectPlay, clsid) || IsEqualGUID(&CLSID_DPLobby, clsid))
	{
		if (IsEqualGUID(&IID_IClassFactory, iid))
		{
			IClassFactory* icf = malloc(sizeof(IClassFactory));
#ifdef _DEBUG
			char* buf = malloc(79);
			GuidToStr(clsid, buf);
			buf[38] = '\r';
			buf[39] = '\n';
			GuidToStr(iid, buf + 40);
			if (IsEqualGUID(&CLSID_DirectPlay, clsid)) MessageBoxA(GetForegroundWindow(), buf, "FakeDPlay::DllGetClassObject(CLSID_DirectPlay,IID_IClassFactory)", 0);
			else MessageBoxA(GetForegroundWindow(), buf, "FakeDPlay::DllGetClassObject(CLSID_DPLobby,IID_IClassFactory)", 0);
			free(buf);
#endif
			icf->lpVtbl = __ICF_VTBL;
			icf->dwRefCount = 1;
			*ppv = icf;
			return S_OK;
		}
		else
		{
			char* buf = malloc(39);
			GuidToStr(iid, buf);
			if (IsEqualGUID(&CLSID_DirectPlay, clsid)) MessageBox(GetForegroundWindow(), buf, "DllGetClassObject(CLSID_DirectPlay) requests IID:", 0);
			else MessageBox(GetForegroundWindow(), buf, "DllGetClassObject(CLSID_DPLobby) requests IID:", 0);
			free(buf);
			return E_FAIL;
		}
	}
	else
	{
		char* buf = malloc(39);
		GuidToStr(clsid, buf);
		MessageBox(GetForegroundWindow(), buf, "DllGetClassObject requests CLSID:", 0);
		free(buf);
		return E_FAIL;
	}
}
STDAPI DllRegisterServer()
//https://docs.microsoft.com/en-us/windows/win32/api/olectl/nf-olectl-dllregisterserver
// Instructs an in-process server to create its registry entries for all classes supported in this server module
{
	MessageBox(GetForegroundWindow(), "DllRegisterServer", "DllRegisterServer", 0);
	return S_OK;
}
STDAPI DllUnregisterServer()
//https://docs.microsoft.com/en-us/windows/win32/api/olectl/nf-olectl-dllunregisterserver
// Instructs an in-process server to remove only those entries created through DllRegisterServer
{
	MessageBox(GetForegroundWindow(), "DllUnregisterServer", "DllUnregisterServer", 0);
	return S_OK;
}

// see C:\Program Files\Microsoft DirectX SDK (August 2007)\Include\dplay.h
// and also C:\Program Files\Microsoft SDKs\Windows\v7.0A\Include\ObjBase.h:100,161

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			{
				__FDP_HM = hinstDLL;
				// NE WSAStartup !
				if (init_vtables() == TRUE) return TRUE;
				else return FALSE;
			}
		case DLL_PROCESS_DETACH:
			{
				if (__ICF_VTBL) free(__ICF_VTBL);
				if (__IDP4_VTBL) free(__IDP4_VTBL);
				if (__IDPL3_VTBL) free(__IDPL3_VTBL);
				// NE WSACleanup(); !
				return TRUE;
			}
		default:
			return TRUE;
	}
}

#ifdef _DEBUGEXE
int main(int argc, char** argv)
{
	/*LPCLASSFACTORY i;
	HRESULT hr;
	DllGetClassObject(&CLSID_DirectPlay, &IID_IClassFactory, &i);
	hr = i->lpVtbl->AddRef(i);
	hr = i->lpVtbl->Release(i);
	i->lpVtbl->QueryInterface(i, &IID_IUnknown, &i);
	i->lpVtbl->CreateInstance(i, i, &IID_IUnknown, &i);
	i->lpVtbl->LockServer(i, 0);*/
	/*char*p;
	p = (char*)(DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_GETPORTDLG), GetForegroundWindow(), DialogProc, NULL));
	p = (char*)(DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_GETENDPOINTDLG), GetForegroundWindow(), DialogProc, NULL));*/
	return 0;
}
#endif
