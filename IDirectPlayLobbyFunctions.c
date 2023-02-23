#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include<windows.h>
#include "IDirectPlayLobbyFunctions.h"
#include "util.h"

HRESULT __stdcall IDirectPlayLobby_QueryInterface(LPUNKNOWN This, IID* piid, LPVOID * ppv)
{
	if (IsEqualGUID(piid, &IID_IDirectPlayLobby3A) ||
		IsEqualGUID(piid, &IID_IDirectPlayLobby3) ||
		IsEqualGUID(piid, &IID_IDirectPlayLobby2A) ||
		IsEqualGUID(piid, &IID_IDirectPlayLobby2) ||
		IsEqualGUID(piid, &IID_IDirectPlayLobbyA) ||
		IsEqualGUID(piid, &IID_IDirectPlayLobby))
	{
		*ppv = This;
		++((IDirectPlayLobby3*)This)->dwRefCount;
		return S_OK;
	}
	else
	{
		char* buf = malloc(39);
		GuidToStr(piid, buf);
		MessageBox(GetForegroundWindow(), buf, "IDirectPlay_QueryInterface requests IID:", 0);
		free(buf);
		return E_FAIL;
	}
}
ULONG __stdcall IDirectPlayLobby_AddRef(LPUNKNOWN This)
{
#ifdef _DEBUG
	ShowMemory(&((IDirectPlayLobby3*)This)->dwRefCount, 4, "IDirectPlayLobby_AddRef (before incrementing)");
#endif
	return ++((IDirectPlayLobby3*)This)->dwRefCount;
}
ULONG __stdcall IDirectPlayLobby_Release(LPUNKNOWN This)
{
#ifdef _DEBUG
	ShowMemory(&((IDirectPlayLobby3*)This)->dwRefCount, 4, "IDirectPlayLobby_Release (before decreasing)");
#endif
	--((IDirectPlayLobby3*)This)->dwRefCount;
	if (((IDirectPlayLobby3*)This)->dwRefCount == 0)
	{
		free(This);
#ifdef _DEBUG
		MessageBoxA(GetForegroundWindow(), "IDirectPlayLobby_Release (object freed)", "IDirectPlayLobby_Release (object freed)", 0);
#endif
		return 0;
	}
	return ((IDirectPlayLobby3*)This)->dwRefCount;
}
/*  IDirectPlayLobby Methods	*/
HRESULT __stdcall IDirectPlayLobby_Connect(LPUNKNOWN This, DWORD dw, LPDIRECTPLAY2 * pdp, IUnknown FAR * punk)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_Connect", "IDirectPlayLobby_Connect", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_CreateAddress(LPUNKNOWN This, GUID* lpguid1, GUID* lpguid2, LPVOID pv1, DWORD dw, LPVOID pv2, LPDWORD lpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_CreateAddress", "IDirectPlayLobby_CreateAddress", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_EnumAddress(LPUNKNOWN This, LPDPENUMADDRESSCALLBACK lpdpenumaddresscallback, LPVOID pv1, DWORD dw, LPVOID pv2)
{
	// pv1: lpconnection, dw length of lpconnection, pv2 je lpcontext !
	// *pv2 =  pointr?
	// *(pv2+4) = length of following field == DPCAPS -> 40
	// *(pv2+8) = DPCAPS
	// *(pv2+44) = pointr?
	// *(pv2+48) = guid

	// it really just iterates over DPCOMPOUNDADDRESSELEMENTs of lpconnection! so that's easy...
	// not exactly, it's DPADDRESS which means last member is not a pointer, data is directly appended after dwSize (dplobby.h:689)
	DPCOMPOUNDADDRESSELEMENT* p = pv1;  // hack abysme rovnou dostali pointr na zaèátek dat
	int r = 1;
	while (r && (p < (char*)pv1 + dw))
	{
#ifdef _DEBUG
		char* buf = malloc(39);
		GuidToStr(&p->guidDataType, buf);
		MessageBox(GetForegroundWindow(), buf, "IDirectPlayLobby_EnumAddress:GUID", 0);
		ShowMemory(&p->dwDataSize, 4, "IDirectPlayLobby_EnumAddress:size");
		ShowMemory(&p->lpData, p->dwDataSize, "IDirectPlayLobby_EnumAddress:data");
		free(buf);
#endif
		r = lpdpenumaddresscallback(&p->guidDataType, p->dwDataSize, &p->lpData, pv2);
		p = ((char*)p) + 20 + p->dwDataSize;  // pozor, v lpData jsou rovnou nasypaný data! musíme p posunout podle jejich délky!
	}
	return S_OK;
}
HRESULT __stdcall IDirectPlayLobby_EnumAddressTypes(LPUNKNOWN This, LPDPLENUMADDRESSTYPESCALLBACK lpdpenumaddresstypescallback, GUID* lpguid, LPVOID pv, DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_EnumAddressTypes", "IDirectPlayLobby_EnumAddressTypes", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_EnumLocalApplications(LPUNKNOWN This, LPDPLENUMLOCALAPPLICATIONSCALLBACK lpdpenumapplicationscallback, LPVOID pv, DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_EnumLocalApplications", "IDirectPlayLobby_EnumLocalApplications", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_GetConnectionSettings(LPUNKNOWN This, DWORD dw, LPVOID pv, LPDWORD lpdw)
{
#ifdef _DEBUG
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_GetConnectionSettings", "IDirectPlayLobby_GetConnectionSettings", 0);
#endif
	return DPERR_NOTLOBBIED;
}
HRESULT __stdcall IDirectPlayLobby_ReceiveLobbyMessage(LPUNKNOWN This, DWORD dw1, DWORD dw2, LPDWORD lpdw1, LPVOID pv, LPDWORD lpdw2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_ReceiveLobbyMessage", "IDirectPlayLobby_ReceiveLobbyMessage", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_RunApplication(LPUNKNOWN This, DWORD dw, LPDWORD lpdw, LPDPLCONNECTION lpdplconnection, HANDLE h)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_RunApplication", "IDirectPlayLobby_RunApplication", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_SendLobbyMessage(LPUNKNOWN This, DWORD dw1, DWORD dw2, LPVOID pv, DWORD dw3)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_SendLobbyMessage", "IDirectPlayLobby_SendLobbyMessage", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_SetConnectionSettings(LPUNKNOWN This, DWORD dw1, DWORD dw2, LPDPLCONNECTION lpdplconnection)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_SetConnectionSettings", "IDirectPlayLobby_SetConnectionSettings", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_SetLobbyMessageEvent(LPUNKNOWN This, DWORD dw1, DWORD dw2, HANDLE h)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_SetLobbyMessageEvent", "IDirectPlayLobby_SetLobbyMessageEvent", 0);
	return E_FAIL;
}
/*  IDirectPlayLobby2 Methods	*/
HRESULT __stdcall IDirectPlayLobby_CreateCompoundAddress(LPUNKNOWN This, LPDPCOMPOUNDADDRESSELEMENT lpdpcompoundaddresselement, DWORD dw, LPVOID pv, LPDWORD lpdw)
{
#ifdef _DEBUG
	ShowMemory(lpdpcompoundaddresselement, dw * sizeof(DPCOMPOUNDADDRESSELEMENT), "IDirectPlayLobby_CreateCompoundAddress");
#endif
	// DPCOMPOUNDADDRESSELEMENT*, dw: # of DPCOMPOUNDADDRESSELEMENT items, lpconnection, addrsize
	if (lpdpcompoundaddresselement && dw >= 2)
	{
		if (IsEqualGUID(&lpdpcompoundaddresselement[0].guidDataType, &DATATYPE_DPSP) && (lpdpcompoundaddresselement[0].dwDataSize == 16))
		{
			if (IsEqualGUID(lpdpcompoundaddresselement[0].lpData, &DPSP_TCPIP))
			{
				if (IsEqualGUID(&lpdpcompoundaddresselement[1].guidDataType, &DATATYPE_IP_CSTRING) ||
					IsEqualGUID(&lpdpcompoundaddresselement[1].guidDataType, &DATATYPE_IP_UNICODE))
				{
					int requiredsize;
					DWORD* dp = (DWORD*)pv;
#ifdef _DEBUG  // bulánci mi pøedají IP string v UNICODE, ale datatype dají ANSI a dwDataSize == 2 !!!
					ShowMemory(lpdpcompoundaddresselement[1].lpData, 40, "IDirectPlayLobby_CreateCompoundAddress ADDR:", 0);
#endif
					if ((lpdpcompoundaddresselement[1].dwDataSize < 8) && IsIPAddressInUNICODE(lpdpcompoundaddresselement[1].lpData))
					{
						lpdpcompoundaddresselement[1].guidDataType = DATATYPE_IP_UNICODE;
						lpdpcompoundaddresselement[1].dwDataSize = 2 * (wcslen(lpdpcompoundaddresselement[1].lpData) + 1);
					}
					requiredsize = 80 + lpdpcompoundaddresselement[1].dwDataSize;
					if (pv == NULL || lpdw == NULL || *lpdw < requiredsize)
					{
						*lpdw = requiredsize;
						return DPERR_BUFFERTOOSMALL;
					}
					// jinak naplnit lpconnection do pv, address prostì zkopírovat
					// lpconnection je DPADDRESS* ! (viz IDirectPlayLobby_EnumAddress)
					memcpy(pv, &DATATYPE_TOTALSIZE, 16);
					dp[4] = 4;
					dp[5] = requiredsize;
					memcpy(dp + 6, &DATATYPE_DPSP, 16);
					dp[10] = 16;
					memcpy(dp + 11, &DPSP_TCPIP, 16);
					memcpy(dp + 15, &lpdpcompoundaddresselement[1].guidDataType, 16);
					dp[19] = lpdpcompoundaddresselement[1].dwDataSize;
					memcpy(dp + 20, lpdpcompoundaddresselement[1].lpData, lpdpcompoundaddresselement[1].dwDataSize);
					return S_OK;
				}
				else
				{
					MessageBox(GetForegroundWindow(), "service provider is TCP/IP but type of address is invalid", "IDirectPlayLobby_CreateCompoundAddress", 0);
					return DPERR_INVALIDPARAM;
				}
			}
			else  // nenastane, pro všechny DPSP kromì TCP/IP dostaneme jenom jeden DPCOMPOUNDADDRESSELEMENT. ale pro jistotu
			{
				DPCOMPOUNDADDRESSELEMENT* p;  // hack
				if (pv == NULL || lpdw == NULL || *lpdw < 60)
				{
					if (lpdw) *lpdw = 60;
					return DPERR_BUFFERTOOSMALL;
				}
				p = pv;
				memcpy(pv, &DATATYPE_TOTALSIZE, 16);
				p->dwDataSize = 4;
				p->lpData = 60;
				p = (char*)pv + 24;
				memcpy(p, &DATATYPE_DPSP, 16);
				p->dwDataSize = 16;
				memcpy(&p->lpData, lpdpcompoundaddresselement->lpData, 16);
				return S_OK;
			}
		}
		else  // tohle by opravdu nemìlo nastat
		{
			MessageBox(GetForegroundWindow(), "first compoundaddresselement is not DATATYPE_DPSP", "IDirectPlayLobby_CreateCompoundAddress", 0);
			return DPERR_INVALIDPARAM;
		}
	}
	else
	{
		// for every DPSP other than TCP/IP: lpdpcompoundaddresselement contains ONLY chunk for DPSP !
		// -> return some dummy lpconnection, it's going to be rejected in InitializeConnection anyway
		// return chunk TOTALSIZE? yeah why not
		// DATATYPE_TOTALSIZE, DWORD, DWORD, DATATYPE_DPSP, DWORD, GUID SP => length = 60
		// check that dw == 1? check that lpdp->datatype == DPSP? check that lpdp->datasize == 16?
		DPCOMPOUNDADDRESSELEMENT* p;  // hack
		if (pv == NULL || lpdw == NULL || *lpdw < 60)
		{
			if (lpdw) *lpdw = 60;
			return DPERR_BUFFERTOOSMALL;
		}
		p = pv;
		memcpy(pv, &DATATYPE_TOTALSIZE, 16);
		p->dwDataSize = 4;
		p->lpData = 60;
		p = (char*)pv + 24;
		memcpy(p, &DATATYPE_DPSP, 16);
		p->dwDataSize = 16;
		memcpy(&p->lpData, &lpdpcompoundaddresselement->lpData, 16);
		return S_OK;
	}
}
/*  IDirectPlayLobby3 Methods	*/
HRESULT __stdcall IDirectPlayLobby_ConnectEx(LPUNKNOWN This, DWORD dw, IID* piid, LPVOID * ppv, IUnknown FAR * punk)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_ConnectEx", "IDirectPlayLobby_ConnectEx", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlayLobby_RegisterApplication(LPUNKNOWN This, DWORD dw, LPVOID pv)
{
	// *pv = DWORD, length
	// *(pv+4) = DWORD, 0
	// *(pv+8) = DWORD, char* to app name
	// *(pv+12) = app GUID
	// *(pv+28) = DWORD, char* to that pseudo image file ("Heroes3.icd")
	// *(pv+32) = DWORD, pointr? -> NULL
	// *(pv+36) = DWORD, char* to containing folder / folder process is running in?
	// *(pv+40) = DWORD, pointr, same as the previous one
	// *(pv+44) = DWORD, 0
	// *(pv+48) = DWORD, 0
	// *(pv+52) = DWORD, char* to image file name
#ifdef _DEBUG
	ShowMemory(&dw, 4, "IDirectPlayLobby_RegisterApplication: dw");
	ShowMemory(&pv, 4, "IDirectPlayLobby_RegisterApplication: pv");
	if (pv)
	{
		DWORD* dp = pv;
		ShowMemory(pv, 64, "IDirectPlayLobby_RegisterApplication: *pv");
		ShowMemory(*(dp + 2), 64, "*(pv+8)");
		ShowMemory(*(dp + 7), 64, "*(pv+28)");
		ShowMemory(*(dp + 8), 64, "*(pv+32)");
		ShowMemory(*(dp + 9), 64, "*(pv+36)");
		ShowMemory(*(dp + 10), 64, "*(pv+40)");
		ShowMemory(*(dp + 13), 64, "*(pv+52)");
	}
#endif
	return S_OK;
}
HRESULT __stdcall IDirectPlayLobby_UnregisterApplication(LPUNKNOWN This, DWORD dw, GUID* lpguid)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_UnregisterApplication", "IDirectPlayLobby_UnregisterApplication", 0);
	return S_OK;
}
HRESULT __stdcall IDirectPlayLobby_WaitForConnectionSettings(LPUNKNOWN This, DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlayLobby_WaitForConnectionSettings", "IDirectPlayLobby_WaitForConnectionSettings", 0);
	return E_FAIL;
}
