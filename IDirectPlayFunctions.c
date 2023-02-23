#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include<windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<time.h>
#include "IDirectPlayFunctions.h"
#include "util.h"
#include "FakeDirectPlayServer.h"
#include "FakeDirectPlayClient.h"

void IDirectPlay_Constructor(IDirectPlay4 * This, BOOL bUnicode)
{
	This->State = FDP_STATE_UNINITIALIZED;
	This->dwRefCount = 1;
	This->UseUnicode = bUnicode;
	This->Socket = 0;
	This->MyDPID = 0;
	This->AmServer = 0;
	This->ServerSocket = 0;
	This->ReceivedSysMsgSessionLost = FALSE;
	This->DataMsgQueueLength = 64 * 1024;
	This->DataMsgQueueBeginning = malloc(This->DataMsgQueueLength);
	This->DataMsgQueue = This->DataMsgQueueBeginning;
	This->DataMsgQueueEnd = This->DataMsgQueueBeginning;
	InitializeCriticalSection(&This->DataMsgQueueLock);
	This->SysMsgQueueLength = 4096;
	This->SysMsgQueueBeginning = malloc(This->SysMsgQueueLength);
	This->SysMsgQueue = This->SysMsgQueueBeginning;
	This->SysMsgQueueEnd = This->SysMsgQueueBeginning;
	InitializeCriticalSection(&This->SysMsgQueueLock);
	memset(&This->SessionDesc, 0, sizeof(DPSESSIONDESC2));
	This->Players = NULL;
	This->PlayersCapacity = 0;
}

void FreePlayerInfo(IDirectPlay4 * t)
{
	int i;
	for (i = 0; i < t->PlayersCapacity; ++i)
	{
		t->Players[i].dpid = 0;
		if (t->Players[i].name) free(t->Players[i].name);
		t->Players[i].name = NULL;
	}
}

/*** IUnknown methods ***/
HRESULT __stdcall IDirectPlay_QueryInterface(LPUNKNOWN This, IID * piid, LPVOID * ppv)
{
	// TODO: zkontrolovat že QueryInterface v oficiální implementaci zvyšuje refcount
	// kontrolovat piid oproti This->UseUnicode ?
	if (IsEqualGUID(piid, &IID_IDirectPlay4A) ||
		IsEqualGUID(piid, &IID_IDirectPlay4) ||
		IsEqualGUID(piid, &IID_IDirectPlay3A) ||
		IsEqualGUID(piid, &IID_IDirectPlay3) ||
		IsEqualGUID(piid, &IID_IDirectPlay2A) ||
		IsEqualGUID(piid, &IID_IDirectPlay2))
	{
#ifdef _DEBUG
	if (IsEqualGUID(piid, &IID_IDirectPlay4A)) MessageBox(GetForegroundWindow(), "IID_IDirectPlay4A", "IDirectPlay_QueryInterface", 0);
	if (IsEqualGUID(piid, &IID_IDirectPlay4)) MessageBox(GetForegroundWindow(), "IID_IDirectPlay4", "IDirectPlay_QueryInterface", 0);
	if (IsEqualGUID(piid, &IID_IDirectPlay3A)) MessageBox(GetForegroundWindow(), "IID_IDirectPlay3A", "IDirectPlay_QueryInterface", 0);
	if (IsEqualGUID(piid, &IID_IDirectPlay3)) MessageBox(GetForegroundWindow(), "IID_IDirectPlay3", "IDirectPlay_QueryInterface", 0);
	if (IsEqualGUID(piid, &IID_IDirectPlay2A)) MessageBox(GetForegroundWindow(), "IID_IDirectPlay2A", "IDirectPlay_QueryInterface", 0);
	if (IsEqualGUID(piid, &IID_IDirectPlay2)) MessageBox(GetForegroundWindow(), "IID_IDirectPlay2", "IDirectPlay_QueryInterface", 0);
#endif
		*ppv = This;
		++((IDirectPlay4*)This)->dwRefCount;
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
ULONG __stdcall IDirectPlay_AddRef(LPUNKNOWN This)
{
	// když je interface vyrobeno tak refcount = 1, AddRef zvýší o 1, Release sníží o 1
	// poslední Release který vrátí 0 uvolní prostøedky
#ifdef _DEBUG
	ShowMemory(&((IDirectPlay4*)This)->dwRefCount, 4, "IDirectPlay_AddRef (before incrementing)");
#endif
	return ++((IDirectPlay4*)This)->dwRefCount;
}
ULONG __stdcall IDirectPlay_Release(LPUNKNOWN This)
{
	IDirectPlay4* t = (IDirectPlay4*)This;
#ifdef _DEBUG
	ShowMemory(&t->dwRefCount, 4, "IDirectPlay_Release (before decreasing)");
#endif
	--t->dwRefCount;
	if (t->dwRefCount == 0)
	{
		int i;
		if (t->Players)
		{
			// free all t->dpPlayers::name
			for (i = 0; i < t->PlayersCapacity; ++i)
			{
				if (t->Players[i].name) free(t->Players[i].name);
			}
			free(t->Players);
		}
		free(t->DataMsgQueueBeginning);
		DeleteCriticalSection(&t->DataMsgQueueLock);  // critical section should not be owned anymore - if FakeDirectPlayClient was running, IDP_Close() should have been called by this time
		free(t->SysMsgQueueBeginning);
		DeleteCriticalSection(&t->SysMsgQueueLock);
		free(This);
		return 0;
	}
	return t->dwRefCount;
}
/*** IDirectPlay2 methods ***/
HRESULT __stdcall IDirectPlay_AddPlayerToGroup(LPUNKNOWN This, DPID dpid1, DPID dpid2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_AddPlayerToGroup", "IDirectPlay_AddPlayerToGroup", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_Close(LPUNKNOWN This)
{   // close active session and connection
	IDirectPlay4* t = (IDirectPlay4*)This;
	int i;
#ifdef _DEBUG
	MessageBox(GetForegroundWindow(), "IDirectPlay_Close", "IDirectPlay_Close", 0);
#endif
	if (t->Socket)
	{
		if (t->AmServer)
		{
			i = FDP_MSG_TerminateSession;  // je stejnì k nièemu, klienti by poznali i tak ze zavøení socketu ale...
			send(t->Socket, &i, 1, 0);
		}
		// poslat serveru message LeaveSession když jsem jen klient? ne, buï klient volal DestroyPlayer nebo server detekuje zavøení socketu a pošle ostatním SYSMSG_DESTROYPLAYER
		closesocket(t->Socket);
		t->Socket = 0;
	}

	if (t->AmServer)
	{
		Sleep(500);  // give other clients a chance to Receive and process DESTROYPLAYER msg
		closesocket(t->ServerSocket);
		t->ServerSocket = 0;
	}

	// free all t->Players.name
	FreePlayerInfo(t);

	t->MyDPID = 0;
	t->AmServer = FALSE;
	t->ReceivedSysMsgSessionLost = FALSE;
	t->State = FDP_STATE_UNINITIALIZED;
	// potøebujeme zjistit, jestli po Close vždy následuje Initialize, nebo mùže rovnou Open (Join spíš ne, pro každý Join se volá Initialize(addr) )
	// použít dbg build, spustit heroes3 a zkusit Host -> Close -> Host
	// otestováno, vypadá to že po každym Close musí být voláno Initialize
	return S_OK;
}
HRESULT __stdcall IDirectPlay_CreateGroup(LPUNKNOWN This, LPDPID lpdpid,LPDPNAME lpdpname,LPVOID pv,DWORD dw1,DWORD dw2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_CreateGroup", "IDirectPlay_CreateGroup", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_CreatePlayer(LPUNKNOWN This, LPDPID lpdpid,LPDPNAME lpdpname,HANDLE h,LPVOID pv,DWORD dw1,DWORD dw2)
{  // h, pv - optional, user defined, dw1: flags, dw2: DPPLAYER_SERVERPLAYER == 0x100, otherwise 0 (dplay.h:1193)
	// 0 == DPID_SYSMSG == DPID_ALLPLAYERS, 1 == DPID_SERVERPLAYER (dplay.h:117)
	IDirectPlay4* t = (IDirectPlay4*)This;

#ifdef _DEBUG
	if (lpdpname && lpdpname->lpszShortNameA) MessageBox(GetForegroundWindow(), lpdpname->lpszShortNameA, "IDirectPlay_CreatePlayer", 0);
	else MessageBox(GetForegroundWindow(), "IDirectPlay_CreatePlayer", "IDirectPlay_CreatePlayer", 0);
#endif

	if (t->State == FDP_STATE_UNINITIALIZED) return DPERR_UNINITIALIZED;
	//if (t->State == FDP_STATE_INITIALIZED) return DPERR_NOT_IN_SESSION; ? no appropriate error in dplay.h
	if (t->State == FDP_STATE_IN_SESSION_HAS_PLAYER) return DPERR_CANTCREATEPLAYER;
	if (t->State == FDP_STATE_SESSION_LOST) return DPERR_SESSIONLOST;  // or DPERR_CONNECTIONLOST?

	if (t->MyDPID) return DPERR_CANTCREATEPLAYER;  // already have a player

	if (t->State == FDP_STATE_IN_SESSION)
	{
		// budeme posílat: 1B MSGTYPE, 1B namelen, namelen*B
		int i, j, l, r;
		char* buf;
		time_t begin;
		if (lpdpname && lpdpname->lpszShortNameA)  // bulánci: lpdpname == NULL
		{
			if (t->UseUnicode) l = 2 * wcslen(lpdpname->lpszShortName);
			else l = strlen(lpdpname->lpszLongNameA);
		}
		else l = 0;  // 0 means name == NULL, not ""
		if (l > 254) l = 254;  // musíme posílat celé wchary, kdybych z posledního wcharu "a\0" poslal jen "a", na druhé stranì by se appendoval wchar "\0\0" a dostal bych "a\0" "\0?" -> nemusel by být správnì ukonèený wstr
		buf = malloc(l + 2);  // skuteènì kopírovat??
		buf[0] = FDP_MSG_CreatePlayer;
		buf[1] = l;
		memcpy(buf + 2, lpdpname->lpszShortNameA, l);
		l += 2;

		i = 0;
		while (i < l)
		{
			r = send(t->Socket, buf + i, l - i, 0);
			if (r == 0 || r == SOCKET_ERROR) break;
			i += r;
		}
		free(buf);
		if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;

		// èekat v loopu dokud v t->SysMsgQueue nebude zpráva FDP_MSG_CreatePlayer (timeout?), pøeèíst z ní DPID, return
		time(&begin);
		while (1)
		{
			//if (t->Socket == 0) break;  ne! FakeDirectPlayClient_Start ani nic jinýho nenuluje socket, pouze IDirectPlay_Close() ! potøebujeme odlišit socket closed a socket == 0
			// a co pro UDP? tam žádný socket closed nebude. dát sem èistì timeout, tøeba 5s? asi jo
			if (difftime(begin, time(NULL)) >= 5) return DPERR_CONNECTIONLOST;

			EnterCriticalSection(t->SysMsgQueueLock);
			// check. if there is msg, remove, unlock, break
			// snad by ani nemìlo být tøeba PROCHÁZET SysMsgQueue, jen se podívat na tu první (pokud tam nìjaká je)
			//   víc msgs by tam mohlo být pouze kdyby se další transakèní fce zavolala døív než ta první skonèí, tedy z jiného vlákna
			//     to se sice jak víme mùže stát, ale v nejhorším pøípadì poèkáme než ta druhá fce odstraní svou msg, ne?
			// else unlock, continue
			if (t->SysMsgQueue != t->SysMsgQueueEnd)  // je tam msg
			{
				if (*t->SysMsgQueue == FDP_MSG_CreatePlayer)
				{
					t->MyDPID = *(DPID*)(t->SysMsgQueue + 1);
					t->SysMsgQueue += 5;
					LeaveCriticalSection(t->SysMsgQueueLock);
					break;
				}
			}
			LeaveCriticalSection(t->SysMsgQueueLock);
			Sleep(30);
		}

#ifdef _DEBUG
		ShowMemory(&t->MyDPID, 4, "IDirectPlay_CreatePlayer received DPID:");
#endif
		if (t->MyDPID == 0) return DPERR_CANTCREATEPLAYER;
		*lpdpid = t->MyDPID;
		return S_OK;

		// TODO: pøesunout do Join/Open: receive sessiondesc (neposílat celý, jenom podstatný položky jako v serveru)
		recv(t->Socket, &t->SessionDesc.dwFlags, 4, 0);
		recv(t->Socket, &t->SessionDesc.guidApplication, 16, 0);
		recv(t->Socket, &t->SessionDesc.guidInstance, 16, 0);
		recv(t->Socket, &t->SessionDesc.dwMaxPlayers, 4, 0);
		recv(t->Socket, &t->SessionDesc.dwCurrentPlayers, 4, 0);

		t->SessionDesc.dwCurrentPlayers++;  // hack, add self manually because CreatePlayer on server is not finished yet so MY player will not be sent over

		l = 0;
		r = recv(t->Socket, &l, 1, 0);
		if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
		if (l) t->SessionDesc.lpszSessionNameA = malloc(l + 1);
		else t->SessionDesc.lpszSessionNameA = NULL;
		i = 0;
		while (i < l)
		{
			r = recv(t->Socket, t->SessionDesc.lpszSessionNameA + i, l - i, 0);
			if (r == 0 || r == SOCKET_ERROR)
			{
				free(t->SessionDesc.lpszSessionNameA);
				return DPERR_CONNECTIONLOST;
			}
			i += r;
		}
		if (l) t->SessionDesc.lpszSessionNameA[l] = 0;
		t->SessionDesc.lpszPasswordA = NULL;

		// TODO: pøesunout do EnumPlayers: receive players info
		if (t->PlayersCapacity < t->SessionDesc.dwCurrentPlayers)
		{
			// free existing names
			FreePlayerInfo(t);
			free(t->Players);
			t->Players = malloc(t->SessionDesc.dwCurrentPlayers * sizeof(dpPlayerInfo));
			t->PlayersCapacity = t->SessionDesc.dwCurrentPlayers;
		}
		for (i = 0; i < t->SessionDesc.dwCurrentPlayers - 1; ++i)
		{
			recv(t->Socket, &t->Players[i].dpid, 4, 0);
			l = 0;
			recv(t->Socket, &l, 1, 0);
			if (l) t->Players[i].name = malloc(l + 1);
			else t->Players[i].name = NULL;
			j = 0;
			while (j < l)
			{
				r = recv(t->Socket, t->Players[i].name + j, l - j, 0);
				if (r == 0 || r == SOCKET_ERROR)
				{
					free(t->SessionDesc.lpszSessionNameA);
					// free names
					FreePlayerInfo(t);
					return DPERR_CONNECTIONLOST;
				}
				j += r;
			}
			if (l) t->Players[i].name[l] = 0;
		}

		// <HACK: add self>
		t->Players[i].dpid = t->MyDPID;
		if (lpdpname && lpdpname->lpszShortNameA)
		{
			l = strlen(lpdpname->lpszShortNameA) + 1;
			t->Players[i].name = malloc(l);
			memcpy(t->Players[i].name, lpdpname->lpszShortNameA, l);
		}
		else t->Players[i].name = NULL;
		// </HACK: add self>

		// TODO: pøesunout. start FakeDirectPlayClient thread
		CreateThread(NULL, 0, FakeDirectPlayClient_Start, t, 0, NULL);

		return S_OK;
	}
	else return DPERR_UNINITIALIZED;
}
HRESULT __stdcall IDirectPlay_DeletePlayerFromGroup(LPUNKNOWN This, DPID dpid1,DPID dpid2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_DeletePlayerFromGroup", "IDirectPlay_DeletePlayerFromGroup", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_DestroyGroup(LPUNKNOWN This, DPID dpid)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_DestroyGroup", "IDirectPlay_DestroyGroup", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_DestroyPlayer(LPUNKNOWN This, DPID dpid)
{
	IDirectPlay4* t = (IDirectPlay4*)This;
	if (dpid != t->MyDPID)
	{
		MessageBoxA(GetForegroundWindow(), "App is calling DestroyPlayer but NOT for ITS LOCAL player!", "App is calling DestroyPlayer but NOT for ITS LOCAL player!", 0);
		return DPERR_INVALIDPLAYER;
	}
	if (t->State != FDP_STATE_IN_SESSION_HAS_PLAYER) return E_FAIL;

	if (t->Socket)
	{
		int i = FDP_MSG_DestroyPlayer;
		int r;
		// poslat message (nìjaký potvrzení? neni tøeba)
		// mam zavøít socket? jsem stále v session (==connection) a teoreticky bych moh opìt zavolat CreatePlayer... takže ne
		r = send(t->Socket, &i, 1, 0);
		if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
		// poslat i DPID? nemìlo by být tøeba, server bude vìdìt podle socketu o koho se jedná

#ifdef _DEBUG
		ShowMemory(&dpid, 4, "IDirectPlay_DestroyPlayer DPID:");
#endif
		t->MyDPID = 0;
		t->State = FDP_STATE_IN_SESSION;
		return S_OK;
	}
	else return DPERR_UNINITIALIZED;
}
HRESULT __stdcall IDirectPlay_EnumGroupPlayers(LPUNKNOWN This, DPID dpid,LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_EnumGroupPlayers", "IDirectPlay_EnumGroupPlayers", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_EnumGroups(LPUNKNOWN This, LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_EnumGroups", "IDirectPlay_EnumGroups", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_EnumPlayers(LPUNKNOWN This, LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw)
{
	// lpguid, pv - context, dw - flags (dplay.h:1128)
	// if (lpguid) { enumerate players or groups in another session }
	// check value of dw ?

	IDirectPlay4* t = (IDirectPlay4*)This;

	if (lpguid)
	{
#ifdef _DEBUG
		char* buf = malloc(39);
		GuidToStr(lpguid, buf);
		MessageBox(GetForegroundWindow(), buf, "IDirectPlay_EnumPlayers LPGUID:");
		free(buf);
#endif
		return DPERR_INVALIDPARAM;
	}

	if (dw != 0 /*DPENUMPLAYERS_ALL*/)
	{
#ifdef _DEBUG
		ShowMemory(&dw, 4, "IDirectPlay_EnumPlayers FLAGS:");
#endif
		return DPERR_INVALIDFLAGS;  // should I return DPERR_INVALIDPARAM rather than this? test!
	}

	if ((t->State != FDP_STATE_IN_SESSION) && (t->State != FDP_STATE_IN_SESSION_HAS_PLAYER)) return E_FAIL;

	if (t->Socket)
	{
		int i = FDP_MSG_EnumPlayers;
		time_t begin;
		send(t->Socket, &i, 1, 0);
		// èekat v cyklu dokud nebude v SysMsgQueue zpráva FDP_MSG_EnumPlayers
		time(&begin);
		while (1)
		{
			if (difftime(begin, time(NULL)) >= 5) return DPERR_CONNECTIONLOST;  // timeout
			EnterCriticalSection(t->SysMsgQueueLock);
			if (t->SysMsgQueue != t->SysMsgQueueEnd)  // je tam msg
			{
				if (*t->SysMsgQueue == FDP_MSG_EnumPlayers)
				{
					DPID dpid;
					DPNAME dpname;
					int l = *(unsigned short*)(t->SysMsgQueue + 1);  // length of message
					int n = *(unsigned short*)(t->SysMsgQueue + 3);  // number of players
					char* p = t->SysMsgQueue + 5;  // následuje n*(4B DPID, 1B namelen, namelen*B name)
					int r;
					dpname.dwSize = sizeof(dpname);
					dpname.dwFlags = 0;
					dpname.lpszShortNameA = NULL;
					dpname.lpszLongNameA = NULL;
					for (i = 0; i < n; ++i)
					{
						dpid = *(DPID*)p;
						dpname.lpszShortNameA = NULL;  // TODO: jesti posíláme právì strlen(name) bajtù tak za name neni "\0" -> musel bych kopírovat :///
#ifdef _DEBUG
						ShowMemory(&t->Players[i].dpid, 4, dpname.lpszShortNameA);
#endif
						r = lpdpenumplayerscallback(t->Players[i].dpid, DPPLAYERTYPE_PLAYER, &dpname, 0, pv);
						if (r == 0) break;
					}
					t->SysMsgQueue += l;
					LeaveCriticalSection(t->SysMsgQueueLock);
					break;
				}
			}
			LeaveCriticalSection(t->SysMsgQueueLock);
			Sleep(30);
		}
		return S_OK;
	}
	else return DPERR_UNINITIALIZED;
}
HRESULT __stdcall IDirectPlay_EnumSessions(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw1,LPDPENUMSESSIONSCALLBACK2 lpdpenumsessionscallback,LPVOID pv,DWORD dw2)
{
	// on TCP/IP host/join selection screen, the game calls EnumSessions every 1s! keep in mind
	// dw1 is timeOut, pv is lpcontext (optional, user defined param for callback), dw2 is flags (dplay.h:1253)
	IDirectPlay4* t = (IDirectPlay4*)This;
	if (dw2 & 0x20 /*DPENUMSESSIONS_STOPASYNC*/) return S_OK;
#ifdef _DEBUG
	Beep(300, 100);
	ShowMemory(lpdpsessiondesc, sizeof(DPSESSIONDESC2), "IDirectPlay_EnumSessions lpdpsessiondesc:");
	ShowMemory(&dw2, 4, "IDirectPlay_EnumSessions: flags");
#endif
	
	if (t->Socket)  // TODO: už bychom se nemìli rozhodovat podle t->Socket ale podle t->State
	{
		int r = FDP_MSG_EnumSessions;
		time_t begin;
		// check dw2? let's just ignore it... EnumSessions flags: dplay.h:1253
		r = send(t->Socket, &r, 1, 0);
		if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
		// správnì bych mìl serveru poslat lpdpsessiondesc->guidApplication a on by vrátil jenom sessions pro tuto aplikaci
		// ale zatim mùžeme nechat, na serveru stejnì bìží jenom jedna aplikace

		time(&begin);
		while (1)
		{
			if (difftime(begin, time(NULL)) >= 5) return DPERR_CONNECTIONLOST;  // timeout
			EnterCriticalSection(t->SysMsgQueueLock);
			if ((t->SysMsgQueue != t->SysMsgQueueEnd)  // je tam msg
				&& (*t->SysMsgQueue == FDP_MSG_EnumSessions))
			{
				int msglength = *(unsigned short*)(t->SysMsgQueue + 1);
				int numsessions = *(int*)(t->SysMsgQueue + 3);
				if (numsessions > 0)
				{
					DPSESSIONDESC2 sd;
					char* p = t->SysMsgQueue + 7;
					char* buf = malloc(256);
					int callcallback = 1;
					int i, l;
					if (lpdpsessiondesc) sd = *lpdpsessiondesc;
					else
					{
						memset(&sd, 0, sizeof(sd));
						sd.dwSize = sizeof(sd);
					}
					sd.lpszSessionNameA = buf;
					//sd.dwFlags &= 0xFFFFFFFB;  clear DPSESSION_MIGRATEHOST flag because that's bananas (flags for dpsessiondesc=dplay.h:277)

					for (i = 0; i < numsessions; ++i)
					{
						// budeme posílat celou dpsessiondesc? asi zbyteèný...
						// flags, dwMaxPlayers, dwCurrentPlayers, length(session name), session name
						sd.dwFlags = *(DWORD*)p;
						p += 4;
						sd.dwMaxPlayers = *(DWORD*)p;
						p += 4;
						sd.dwCurrentPlayers = *(DWORD*)p;
						p += 4;
						l = *p++;
						memcpy(buf, p, l);  // TODO: jak teda ty jména?
						buf[l] = 0;
						if (t->UseUnicode) buf[l + 1] = 0;

						if (callcallback) callcallback = lpdpenumsessionscallback(&sd, &dw1, 0, pv);
						if (callcallback == 0) break;
						p += l;
					}
					free(buf);
					t->SysMsgQueue += msglength;
					LeaveCriticalSection(t->SysMsgQueueLock);
					return S_OK;
				}
				else  // TODO: mùže se vùbec stát, že mi server pošle odpovìï a v ní bude 0 sessions?
				{
					t->SysMsgQueue += 5;
					LeaveCriticalSection(t->SysMsgQueueLock);
					lpdpenumsessionscallback(NULL, &dw1, 1/*DPESC_TIMEDOUT*/, pv);  // pravý directplay to takhle zavolá
#ifdef _DEBUG
					MessageBox(GetForegroundWindow(), "IDirectPlay_EnumSessions NULL", "IDirectPlay_EnumSessions NULL", 0);
#endif
					return S_OK;
				}
			}
			LeaveCriticalSection(t->SysMsgQueueLock);
			Sleep(30);
		}
		return S_OK;  // we never break out of that while loop but whatever
	}
	else
	{
		lpdpenumsessionscallback(NULL, &dw1, 1/*DPESC_TIMEDOUT*/, pv);  // pravý directplay to takhle zavolá
#ifdef _DEBUG
		MessageBox(GetForegroundWindow(), "IDirectPlay_EnumSessions NULL", "IDirectPlay_EnumSessions NULL", 0);
#endif
	}
	return S_OK;
}
HRESULT __stdcall IDirectPlay_GetCaps(LPUNKNOWN This, LPDPCAPS lpdpcaps,DWORD dw)
{	// dplay.h:166
	// change dynamically based on connection status and stuff? nah
	IDirectPlay4* t = (IDirectPlay4*)This;
#ifdef _DEBUG
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetCaps", "IDirectPlay_GetCaps", 0);
#endif
	lpdpcaps->dwFlags = /*DPCAPS_GUARANTEEDSUPPORTED*/0x40 | /*DPCAPS_GUARANTEEDOPTIMIZED*/0x20;
	if (t->AmServer) lpdpcaps->dwFlags |= 2;  // DPCAPS_ISHOST
	lpdpcaps->dwMaxBufferSize = 65479;
	lpdpcaps->dwMaxQueueSize = 0;
	lpdpcaps->dwMaxPlayers = 65535;
	lpdpcaps->dwHundredBaud = 0;
	lpdpcaps->dwLatency = 500;
	lpdpcaps->dwMaxLocalPlayers = 1;
	lpdpcaps->dwHeaderLength = 20;
	lpdpcaps->dwTimeout = 5000;
	return S_OK;
}
HRESULT __stdcall IDirectPlay_GetGroupData(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw,DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetGroupData", "IDirectPlay_GetGroupData", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetGroupName(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetGroupName", "IDirectPlay_GetGroupName", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetMessageCount(LPUNKNOWN This, DPID dpid, LPDWORD lpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetMessageCount", "IDirectPlay_GetMessageCount", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetPlayerAddress(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw)
{
	// *pv = guid DATATYPE_TOTALSIZE
	// *(pv+16) = DWORD, 4 (length of following field)
	// *(pv+20) = DWORD, length of Address (130) (Address == lpconnection == DPADDRESS*)
	// *(pv+24) = GUID DATATYPE_DPSP
	// *(pv+40) = DWORD, length of following field (guid -> 16)
	// *(pv+44) = guid, service provider! (DPSP_TCPIP == 36E95EE0-8577-11cf-960C-0080c7534e82)
	// *(pv+60) = GUID DATATYPE_IP_CSTRING
	// *(pv+76) = DWORD, 10 - length of address as a c-string ("10.0.0.33\0")
	// *(pv+80) = address as a c-string ("10.0.0.33\0")
	// *(pv+80+*(pv+76)) = GUID DATATYPE_IP_UNICODE
	// *(pv+96+*(pv+76)) = DWORD, 20, length of address as a UNICODE string !
	// *(pv+100+*(pv+76)) = address as a UNICODE string !
	// => length = 100 + 3*(length of address as a c-string)
	//hostent* localHost = gethostbyname(NULL);
	//char * localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);
	IDirectPlay4* t = (IDirectPlay4*)This;
#ifdef _DEBUG
	ShowMemory(&dpid, 4, "IDirectPlay_GetPlayerAddress:dpid");
#endif
	if (t->State != FDP_STATE_IN_SESSION_HAS_PLAYER) return E_FAIL;  // pokud budeme pozdìji podporovat dotazy na adresy cizích players tak zde zmìnit (pøidat IN_SESSION)

	if (dpid == t->MyDPID)
	{
		struct hostent* localhost = gethostbyname(NULL);
		char * localip = inet_ntoa (*(struct in_addr *)*localhost->h_addr_list);
		char* p;
		DWORD* dp = (DWORD*)pv;
		int len = strlen(localip) + 1;
		int needed = 100 + 3 * len;
		if (pv == NULL || lpdw == NULL || *lpdw < needed)
		{
			if (lpdw) *lpdw = needed;
			return DPERR_BUFFERTOOSMALL;
		}

		memcpy(dp, &DATATYPE_TOTALSIZE, 16);
		dp[4] = 4;
		dp[5] = needed;

		memcpy(dp + 6, &DATATYPE_DPSP, 16);
		dp[10] = 16;
		memcpy(dp + 11, &DPSP_TCPIP, 16);

		memcpy(dp + 15, &DATATYPE_IP_CSTRING, 16);
		dp[19] = len;
		memcpy(dp + 20, localip, len);

		p = ((char*)(dp + 20)) + len;
		dp = (DWORD*)p;
		memcpy(dp, &DATATYPE_IP_UNICODE, 16);  // mùžu ten unicode vynechat?
		dp[4] = 2 * len;
		p = (char*)(dp + 5);
		while (*localip)
		{
			*p++ = *localip++;
			*p++ = 0;
		}
		*p++ = 0;
		*p = 0;
		*lpdw = needed;
		return S_OK;
	}
	else
	{
		// zeptat se serveru? nevrátit nic?
		*lpdw = 0;  // zatim nevracíme nic
		return S_OK;
	}
}
HRESULT __stdcall IDirectPlay_GetPlayerCaps(LPUNKNOWN This, DPID dpid,LPDPCAPS lpdpcaps,DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetPlayerCaps", "IDirectPlay_GetPlayerCaps", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetPlayerData(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw,DWORD dw)
{  // dpid, buf, bufsize, flags
	//data napøed musí aplikace nastavit zavoláním SetPlayerData (-> uloží data na server => message)
#ifdef _DEBUG
	ShowMemory(&dpid, 4, "IDirectPlay_GetPlayerData:dpid");
#endif
	if (lpdw) *lpdw = 0;
	return S_OK;
}
HRESULT __stdcall IDirectPlay_GetPlayerName(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetPlayerName", "IDirectPlay_GetPlayerName", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetSessionDesc(LPUNKNOWN This, LPVOID pv, LPDWORD lpdw)
{   // na pv mám uložit: DPSESSIONDESC2 hned následovanou bajtama session name (sessiondesc::lpszpasswordA je vždy NULL)
	IDirectPlay4* t = (IDirectPlay4*)This;
	if ((t->State == FDP_STATE_IN_SESSION) || (t->State == FDP_STATE_IN_SESSION_HAS_PLAYER))
	{
		int r = FDP_MSG_GetSessionDesc;
		time_t begin;
		//DPSESSIONDESC2* sd = (DPSESSIONDESC2*)pv;
		//char* p = (char*)pv + sizeof(DPSESSIONDESC2);
		r = send(t->Socket, &r, 1, 0);  // pokud budeme pozdìji podporovat více sessions na serveru, budeme muset poslat i sd.guidInstance
		if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;

		time(&begin);
		while (1)
		{
			if (difftime(begin, time(NULL)) >= 5) return DPERR_CONNECTIONLOST;  // timeout
			EnterCriticalSection(t->SysMsgQueueLock);
			if ((t->SysMsgQueue != t->SysMsgQueueEnd)  // je tam msg
				&& (*t->SysMsgQueue == FDP_MSG_GetSessionDesc))
			{
				int msglength = *(unsigned short*)(t->SysMsgQueue + 1);
				int l = 0;
				DPSESSIONDESC2* sd = (DPSESSIONDESC2*)(t->SysMsgQueue + 3);
				if (sd->lpszSessionNameA)
				{
					if (t->UseUnicode) l = 2 * (wcslen(sd->lpszSessionName) + 1);  // TODO: as of now we specifically DON'T include terminating null chars!
					else l = strlen(sd->lpszSessionNameA) + 1;
				}
				if (pv == NULL || lpdw == NULL || *lpdw < sizeof(DPSESSIONDESC2) + l)
				{
					if (lpdw) *lpdw = sizeof(DPSESSIONDESC2) + l;
					t->SysMsgQueue += msglength;
					LeaveCriticalSection(t->SysMsgQueueLock);
					return DPERR_BUFFERTOOSMALL;
				}

				memcpy(pv, sd, sizeof(DPSESSIONDESC2) + l);
				// write terminating null char!
				t->SysMsgQueue += msglength;
				LeaveCriticalSection(t->SysMsgQueueLock);
				*lpdw = sizeof(DPSESSIONDESC2) + l;
				return S_OK;
			}
			LeaveCriticalSection(t->SysMsgQueueLock);
			Sleep(30);
		}

#ifdef _DEBUG
		ShowMemory(pv, *lpdw, "returning from IDirectPlay_GetSessionDesc");
		ShowMemory(p, l + 1, "IDirectPlay_GetSessionDesc: session name");
#endif
		return S_OK;
	}
	else return DPERR_UNINITIALIZED;  // TODO: co mam vracet zde když jsem initialized ale not in session? check!
}
HRESULT __stdcall IDirectPlay_Initialize(LPUNKNOWN This, LPGUID lpguid)
{
	char* buf = malloc(39);
	GuidToStr(lpguid, buf);
	MessageBox(GetForegroundWindow(), buf, "IDirectPlay_Initialize GUID:", 0);
	free(buf);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_Open(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc, DWORD dw)
{  // open a SESSION, done after InitializeConnection
	// if DPOPEN_CREATE, show a dialog asking on which port to listen for connections
	IDirectPlay4* t = (IDirectPlay4*)This;
	int r;
#ifdef _DEBUG
	ShowMemory(lpdpsessiondesc, sizeof(DPSESSIONDESC2), "IDirectPlay_Open: lpdpsessiondesc");
#endif
	if (dw == 1 /*DPOPEN_JOIN*/)
	{
#ifdef _DEBUG
		MessageBox(GetForegroundWindow(), "IDirectPlay_Open DPOPEN_JOIN", "IDirectPlay_Open DPOPEN_JOIN", 0);
#endif
		if (t->Socket)
		{
			int i = FDP_MSG_JoinSession;
			int l;
			send(t->Socket, &i, 1, 0);
			r = recv(t->Socket, &i, 1, 0);
			if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
			if (i == FDP_MSG_No) return DPERR_ACCESSDENIED;
			if (lpdpsessiondesc->lpszPasswordA) l = strlen(lpdpsessiondesc->lpszPasswordA);
			else l = 0;
			if (l > 255) l = 255;
			r = send(t->Socket, &l, 1, 0);
			if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
			i = 0;
			while (i < l)
			{
				r = send(t->Socket, lpdpsessiondesc->lpszPasswordA + i, l - i, 0);
				if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
				i += r;
			}

			r = recv(t->Socket, &i, 1, 0);  // potvrzení od serveru
			if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
#ifdef _DEBUG
			MessageBox(GetForegroundWindow(), "IDirectPlay_Open DPOPEN_JOIN returns", "IDirectPlay_Open DPOPEN_JOIN returns", 0);
#endif
			if (i == FDP_MSG_No) return DPERR_INVALIDPASSWORD;
			return S_OK;
		}
		else return DPERR_UNINITIALIZED;
	}
	else if (dw == 2 /*DPOPEN_CREATE*/)
	{
		char* p, *pp;
		int port;
		struct sockaddr_in local;
		SOCKET ss;
#ifdef _DEBUG
		MessageBox(GetForegroundWindow(), "IDirectPlay_Open: DPOPEN_CREATE", "IDirectPlay_Open: DPOPEN_CREATE", 0);
#endif
		if (t->Socket) return DPERR_CANTCREATESESSION;  // already am in a session, app sould call Close() first

		if (lpdpsessiondesc->lpszPasswordA) lpdpsessiondesc->dwFlags |= DPSESSION_PASSWORDREQUIRED;

		if (lpdpsessiondesc->dwMaxPlayers == 0) lpdpsessiondesc->dwMaxPlayers = 4;  // fix for bulánci

		lpdpsessiondesc->dwFlags &= 0xFFFFFFFB;  // clear DPSESSION_MIGRATEHOST flag because that's bananas (flags for dpsessiondesc=dplay.h:277)

		while (1)
		{
			port = 0;
			pp = p = (char*)(DialogBoxParam(__FDP_HM, MAKEINTRESOURCE(IDD_GETLOCALPORTDLG), GetForegroundWindow(), DialogProc, NULL));
			if (p == NULL) return DPERR_USERCANCEL;  // uživatel zavøel dialog køížkem
			// try parse (use atoi?)
			while (*p)
			{
				if (*p >= '0' && *p <= '9')
				{
					port *= 10;
					port += *p - '0';
					++p;
				}
				else
				{
					port = -1;
					break;
				}
			}
			if (pp) free(pp);
			if (port <= 0 || port > 65535)
			{
				MessageBox(GetForegroundWindow(), "Enter a valid port number!", "Invalid port number", 0);
				continue;
			}

			// create socket, bind, listen
			ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			memset(&local, 0, sizeof(local));
			local.sin_family = AF_INET;
			local.sin_port = htons(port);
			port = bind(ss, &local, sizeof(local));
			if (port == SOCKET_ERROR)
			{
				closesocket(ss);
				MessageBox(GetForegroundWindow(), "Cannot bind that port. Already in use?", "Invalid port number", 0);
				continue;
			}
			port = listen(ss, 8);
			if (port == SOCKET_ERROR)
			{
				closesocket(ss);
				MessageBox(GetForegroundWindow(), "Cannot listen on that port.", "Invalid port number", 0);
				continue;
			}
			break;
		}

		// ss is a valid bound and listening socket -> start server and pass ss to it as arg
		// then immediately connect to it as client !
#ifdef _DEBUG
		MessageBox(GetForegroundWindow(), "IDirectPlay_Open: DPOPEN_CREATE: listening", "IDirectPlay_Open: DPOPEN_CREATE", 0);
#endif
		t->AmServer = 1;
		t->ServerSocket = ss;
		CreateThread(NULL, 0, FakeDirectPlayServer_Start, ss, 0, NULL);

		t->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		r = TRUE;
		setsockopt(t->Socket, IPPROTO_TCP, TCP_NODELAY, &r, 4);
		r = 500;
		setsockopt(t->Socket, SOL_SOCKET, SO_RCVTIMEO, &r, 4);
		local.sin_addr.S_un.S_addr = 0x100007F;
		if (connect(t->Socket, (struct sockaddr *)(&local), sizeof(local)) == 0)
		{			
			// and send CreateSession message
			int i, l;
#ifdef _DEBUG
			MessageBox(GetForegroundWindow(), "IDirectPlay_Open: DPOPEN_CREATE: connected to server", "IDirectPlay_Open: DPOPEN_CREATE", 0);
#endif
			l = FDP_MSG_CreateSession;
			send(t->Socket, &l, 1, 0);
			// tady snad neni potøeba kontrolovat? je to muj lokální server...
			send(t->Socket, &lpdpsessiondesc->dwFlags, 4, 0);
			send(t->Socket, &lpdpsessiondesc->guidApplication, 16, 0);
			l = strlen(lpdpsessiondesc->lpszSessionNameA);
			if (l > 255) l = 255;
			send(t->Socket, &l, 1, 0);
			i = 0;
			while (i < l)
			{
				r = send(t->Socket, lpdpsessiondesc->lpszSessionNameA + i, l - i, 0);
				if (r == 0 || r == SOCKET_ERROR) break;
				i += r;
			}

			if (lpdpsessiondesc->lpszPasswordA) l = strlen(lpdpsessiondesc->lpszPasswordA);
			else l = 0;
			if (l > 255) l = 255;
			send(t->Socket, &l, 1, 0);
			i = 0;
			while(i < l)
			{
				r = send(t->Socket, lpdpsessiondesc->lpszPasswordA, l, 0);
				if (r == 0 || r == SOCKET_ERROR) break;
				i += r;
			}
			send(t->Socket, &lpdpsessiondesc->dwMaxPlayers, 4, 0);
			r = recv(t->Socket, &l, 1, 0);
			if (r == 0 || r == SOCKET_ERROR)
			{
				closesocket(ss);
				t->Socket = 0;
				t->AmServer = 0;
				return DPERR_CONNECTIONLOST;
			}
			// assert(l == FDP_MSG_Yes), server only returns Yes as of now

			// promítnout do lokálních dat
			t->SessionDesc.dwSize = sizeof(DPSESSIONDESC2);
			t->SessionDesc.dwCurrentPlayers = 0;
			t->SessionDesc.dwFlags = lpdpsessiondesc->dwFlags;
			t->SessionDesc.dwMaxPlayers = lpdpsessiondesc->dwMaxPlayers;
			t->SessionDesc.guidApplication = lpdpsessiondesc->guidApplication;
			if (lpdpsessiondesc->lpszSessionNameA)
			{
				l = strlen(lpdpsessiondesc->lpszSessionNameA) + 1;
				t->SessionDesc.lpszSessionNameA = malloc(l);
				memcpy(t->SessionDesc.lpszSessionNameA, lpdpsessiondesc->lpszSessionNameA, l);
			}

#ifdef _DEBUG
			MessageBox(GetForegroundWindow(), "IDirectPlay_Open: DPOPEN_CREATE returning S_OK", "IDirectPlay_Open: DPOPEN_CREATE", 0);
#endif
			return S_OK;
		}
		else
		{
			closesocket(ss);
			t->Socket = 0;
			t->AmServer = 0;
			return E_FAIL;
		}
	}
	else return DPERR_INVALIDFLAGS;  // DPOPEN_RETURNSTATUS = 0x80, "Return status about progress of open instead of showing any status dialogs"
}
HRESULT __stdcall IDirectPlay_Receive_Locked(LPUNKNOWN This, LPDPID lpdpid1,LPDPID lpdpid2,DWORD dw,LPVOID pv,LPDWORD lpdw)
{  // from, to, flags (dplay.h:1367), data, bufsize (if DPERR_BUFFERTOOSMALL then *lpdw=requiredsize)
	// tato fce by šla zamykat tìsnìji, ne celá komplet ale jen úsek kde se manipuluje s msgqueue
	IDirectPlay4* t = (IDirectPlay4*)This;
	//DPRECEIVE_ALL == 1
	if (dw & 2 /*DPRECEIVE_TOPLAYER*/)
	{
		if (lpdpid2 == NULL) return DPERR_INVALIDPARAM;
		if (*lpdpid2 != t->MyDPID && *lpdpid2 != 0) return DPERR_INVALIDPLAYER;
	}
	if (dw & 4 /*DPRECEIVE_FROMPLAYER*/) return DPERR_INVALIDFLAGS;
	if (dw & 8 /*DPRECEIVE_PEEK*/) return DPERR_INVALIDFLAGS;

	if (t->ReceivedSysMsgSessionLost) return DPERR_CONNECTIONLOST;  // DPSYS_SESSIONLOST poslat jenom jednou, jinak livelock!

	if (t->Socket)
	{
		DPID to, from;
		DWORD l;
		if (t->MessageQueue == t->MessageQueueEnd) return DPERR_NOMESSAGES;

		to = *((DPID*)t->MessageQueue);
		from = *((DPID*)(t->MessageQueue + 4));
		l = *((DWORD*)(t->MessageQueue + 8));

		if (pv == NULL || lpdw == NULL || *lpdw < l)
		{
			if (lpdw) *lpdw = l;
			return DPERR_BUFFERTOOSMALL;
		}

		t->MessageQueue += 12;
		memcpy(pv, t->MessageQueue, l);
		t->MessageQueue += l;

		if (from == 0)
		{
			if (*(DWORD*)pv == DPSYS_CREATEPLAYERORGROUP)
			{
				DPMSG_CREATEPLAYERORGROUP* p = (DPMSG_CREATEPLAYERORGROUP*)pv;
				int playercount = t->SessionDesc.dwCurrentPlayers;

				if (playercount + 1 > t->PlayersCapacity)  // if dpPlayers full, allocate larger and copy existing over
				{
					dpPlayerInfo* aux = malloc((playercount + 1) * sizeof(dpPlayerInfo));
					memcpy(aux, t->Players, playercount * sizeof(dpPlayerInfo));
					free(t->Players);
					t->Players = aux;
					t->PlayersCapacity = playercount + 1;
				}

				// promítnout do lokálních dat
				t->Players[playercount].dpid = p->dpId;
				if (l > sizeof(DPMSG_CREATEPLAYERORGROUP))
				{
					char* name = (char*)pv + sizeof(DPMSG_CREATEPLAYERORGROUP);
					int len = l - sizeof(DPMSG_CREATEPLAYERORGROUP);
					p->dpnName.lpszShortNameA = name;  // pøepojit pointer v msg
					t->Players[playercount].name = malloc(len);  // message obsahuje terminating null character
					memcpy(t->Players[playercount].name, name, len);
				}
				else t->Players[playercount].name = NULL;
				t->SessionDesc.dwCurrentPlayers++;
#ifdef _DEBUG
				MessageBox(GetForegroundWindow(), ((DPMSG_CREATEPLAYERORGROUP*)pv)->dpnName.lpszShortNameA, "IDirectPlay_Receive: DPSYS_CREATEPLAYER name:", 0);
#endif
			}
			else if (*(DWORD*)pv == DPSYS_DESTROYPLAYERORGROUP)
			{
				// ty za odstranìnym pøesunout pøes nìj, t->dpPlayersCapacity zùstává
				DPMSG_DESTROYPLAYERORGROUP* p = (DPMSG_DESTROYPLAYERORGROUP*)pv;
				int i;
				if (l > sizeof(DPMSG_DESTROYPLAYERORGROUP)) p->dpnName.lpszShortNameA = (char*)pv + sizeof(DPMSG_DESTROYPLAYERORGROUP);  // pøepojit pointer v msg
				for (i = 0; i < t->SessionDesc.dwCurrentPlayers; ++i)
				{
					if (t->Players[i].dpid == p->dpId) break;
				}
				if (i < t->SessionDesc.dwCurrentPlayers)
				{
					if (t->Players[i].name) free(t->Players[i].name);
					if (i < t->SessionDesc.dwCurrentPlayers - 1)
					{
						memmove(t->Players + i, t->Players + i + 1, (t->SessionDesc.dwCurrentPlayers - i - 1) * sizeof(dpPlayerInfo));
					}
					else
					{
						t->Players[i].dpid = 0;
						t->Players[i].name = NULL;
					}
					t->SessionDesc.dwCurrentPlayers--;
				}
#ifdef _DEBUG
				MessageBox(GetForegroundWindow(), ((DPMSG_DESTROYPLAYERORGROUP*)pv)->dpnName.lpszShortNameA, "IDirectPlay_Receive: DPSYS_DESTROYPLAYER name:", 0);
				ShowMemory(&p->dpId, 4, "IDirectPlay_Receive: DPSYS_DESTROYPLAYER dpid:");
#endif
			}
			else if (*(DWORD*)pv == DPSYS_SETSESSIONDESC)
			{
				DPMSG_SETSESSIONDESC* p = (DPMSG_SETSESSIONDESC*)pv;
				if (t->SessionDesc.lpszSessionNameA) free(t->SessionDesc.lpszSessionNameA);
				t->SessionDesc = p->dpDesc;
				if (l > sizeof(DPMSG_SETSESSIONDESC))
				{
					char* name = (char*)pv + sizeof(DPMSG_SETSESSIONDESC);
					int len = l - sizeof(DPMSG_SETSESSIONDESC);
					p->dpDesc.lpszSessionNameA = name;  // pøepojit pointer v msg
					t->SessionDesc.lpszSessionNameA = malloc(len);  // message obsahuje terminating null character
					memcpy(t->SessionDesc.lpszSessionNameA, name, len);
				}
#ifdef _DEBUG
				MessageBox(GetForegroundWindow(), p->dpDesc.lpszSessionNameA, "IDirectPlay_Receive: DPSYS_SETSESSIONDESC name:", 0);
#endif
			}
			else if (*(DWORD*)pv == DPSYS_SESSIONLOST)
			{
				t->ReceivedSysMsgSessionLost = 1;
#ifdef _DEBUG
				MessageBox(GetForegroundWindow(), "IDirectPlay_Receive: DPSYS_SESSIONLOST", "IDirectPlay_Receive: DPSYS_SESSIONLOST", 0);
#endif
			}
		}

		*lpdw = l;
		*lpdpid1 = from;
		*lpdpid2 = to;
		return S_OK;
	}
	else return DPERR_UNINITIALIZED;
}
HRESULT __stdcall IDirectPlay_Receive(LPUNKNOWN This, LPDPID lpdpid1,LPDPID lpdpid2,DWORD dw,LPVOID pv,LPDWORD lpdw)
{  // bulanci.exe OBÈAS volají Send a Receive z jiných vláken
	IDirectPlay4* t = (IDirectPlay4*)This;
	HRESULT hr;
	if (t->MessageQueue == t->MessageQueueEnd) return DPERR_NOMESSAGES;  // bit of a hack, but works
	EnterCriticalSection(&t->Lock);
	hr = IDirectPlay_Receive_Locked(This, lpdpid1, lpdpid2, dw, pv, lpdw);
	LeaveCriticalSection(&t->Lock);
	return hr;
}
HRESULT __stdcall IDirectPlay_Send(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw1, LPVOID pv, DWORD dw2)
{  // dpid1 = from, dpid2 = to, dw1 = flags (dplay.h:1389), pv = data, dw2 = length
	IDirectPlay4* t = (IDirectPlay4*)This;
#ifdef _DEBUG
	if ((dpid1 != t->MyDPID)
		|| ((dw1 != 0) && (dw1 != 1))
		|| (dw2 > 1024))
	{
		char* b = malloc(255);
		sprintf(b, "IDirectPlay_Send(from: %d, to: %d, flags: %d, length: %d)", dpid1, dpid2, dw1, dw2);
		MessageBox(GetForegroundWindow(), b, "IDirectPlay_Send", 0);
		free(b);
	}
	//if ((dw1 != 0) && (dw1 != 1)) ShowMemory(&dw1, 4, "IDirectPlay_Send flags:");
#endif
	if (t->Socket)
	{
		int i, r;
		i = FDP_MSG_Send;
		send(t->Socket, &i, 1, 0);
		send(t->Socket, &dpid2, 4, 0);
		send(t->Socket, &dpid1, 4, 0);
		r = send(t->Socket, &dw2, 4, 0);
		if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
		i = 0;
		while (i < dw2)
		{
			r = send(t->Socket, (char*)pv + i, dw2 - i, 0);
			if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
			i += r;
		}
		return S_OK;
	}
	else return DPERR_UNINITIALIZED;
}
HRESULT __stdcall IDirectPlay_Send_With_Lock(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw1, LPVOID pv, DWORD dw2)
{  // bulanci.exe OBÈAS volají Send a Receive z jiných vláken
	// ale v pøípadì messagequeue na klientovi to nevadí
	IDirectPlay4* t = (IDirectPlay4*)This;
	HRESULT hr;
	EnterCriticalSection(&t->Lock);
	hr = IDirectPlay_Send/*_Locked*/(This, dpid1, dpid2, dw1, pv, dw2);
	LeaveCriticalSection(&t->Lock);
	return hr;
}
HRESULT __stdcall IDirectPlay_SetGroupData(LPUNKNOWN This, DPID dpid,LPVOID pv,DWORD dw1,DWORD dw2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SetGroupData", "IDirectPlay_SetGroupData", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_SetGroupName(LPUNKNOWN This, DPID dpid,LPDPNAME lpdpname,DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SetGroupName", "IDirectPlay_SetGroupName", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_SetPlayerData(LPUNKNOWN This, DPID dpid,LPVOID pv,DWORD dw1,DWORD dw2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SetPlayerData", "IDirectPlay_SetPlayerData", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_SetPlayerName(LPUNKNOWN This, DPID dpid,LPDPNAME lpdpname,DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SetPlayerName", "IDirectPlay_SetPlayerName", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_SetSessionDesc(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw)
{
	IDirectPlay4* t = (IDirectPlay4*)This;
#ifdef _DEBUG
	ShowMemory(lpdpsessiondesc, sizeof(DPSESSIONDESC2), "IDirectPlay_SetSessionDesc: lpdpsessiondesc");
	if (lpdpsessiondesc->lpszSessionNameA) MessageBox(GetForegroundWindow(), lpdpsessiondesc->lpszSessionNameA, "IDirectPlay_SetSessionDesc::name", 0);
#endif
	if (t->Socket)
	{
		int i, l, r;
		i = FDP_MSG_SetSessionDesc;
		send(t->Socket, &i, 1, 0);
		send(t->Socket, &lpdpsessiondesc->dwFlags, 4, 0);
		send(t->Socket, &lpdpsessiondesc->dwMaxPlayers, 4, 0);
		l = strlen(lpdpsessiondesc->lpszSessionNameA);
		if (l > 255) l = 255;
		r = send(t->Socket, &l, 1, 0);
		if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
		i = 0;
		while (i < l)
		{
			r = send(t->Socket, lpdpsessiondesc->lpszSessionNameA + i, l - i, 0);
			if (r == 0 || r == SOCKET_ERROR) return DPERR_CONNECTIONLOST;
			i += r;
		}

		if (t->SessionDesc.lpszSessionNameA) free(t->SessionDesc.lpszSessionNameA);
		t->SessionDesc = *lpdpsessiondesc;
		if (lpdpsessiondesc->lpszSessionNameA)
		{
			int len = strlen(lpdpsessiondesc->lpszSessionNameA) + 1;
			t->SessionDesc.lpszSessionNameA = malloc(len);
			memcpy(t->SessionDesc.lpszSessionNameA, lpdpsessiondesc->lpszSessionNameA, len);
		}
		return S_OK;
	}
	else return DPERR_UNINITIALIZED;
}
/*** IDirectPlay3 methods ***/
HRESULT __stdcall IDirectPlay_AddGroupToGroup(LPUNKNOWN This, DPID dpid1, DPID dpid2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_AddGroupToGroup", "IDirectPlay_AddGroupToGroup", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_CreateGroupInGroup(LPUNKNOWN This, DPID dpid,LPDPID lpdpid,LPDPNAME lpdpname,LPVOID pv,DWORD dw1,DWORD dw2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_CreateGroupInGroup", "IDirectPlay_CreateGroupInGroup", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_DeleteGroupFromGroup(LPUNKNOWN This, DPID dpid1,DPID dpid2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_DeleteGroupFromGroup", "IDirectPlay_DeleteGroupFromGroup", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_EnumConnections(LPUNKNOWN This, LPCGUID lpguid,LPDPENUMCONNECTIONSCALLBACK lpdpenumconnectionscallback,LPVOID pv,DWORD dw)
{
	// appGuid, callback, lpContext, flags - 0x1 == DPCONNECTION_DIRECTPLAY (dplay.h:1113)
	// lpConnection by aplikace pak použila v DP_InitializeConnection(), ale nemá smysl se tady ptát na remote endpoint
	// a mam tady aspoò naplnit prázdnou connection? asi jo...

	DWORD* buf;
	DPNAME dpname;
	buf = malloc(80);
	memset(buf, 0, 80);
	buf[0] = 0x1318F560;
	buf[1] = 0x11D0912C;
	buf[2] = 0xA000AA9D;
	buf[3] = 0xCB430AC9;
	buf[4] = 4;
	buf[5] = 80;
	buf[6] = 0x07D916C0;
	buf[7] = 0x11CFE0AF;
	buf[8] = 0xA0004E9C;
	buf[9] = 0x5E4205C9;
	buf[10] = 16;
	memcpy(buf + 11, &DPSP_TCPIP, 16);
	dpname.dwSize = sizeof(DPNAME);
	dpname.dwFlags = 0;
	dpname.lpszShortNameA = "TCP/IP";
	dpname.lpszLongNameA = NULL;
#ifdef _DEBUG
	MessageBox(GetForegroundWindow(), "IDirectPlay_EnumConnections", "IDirectPlay_EnumConnections", 0);
#endif
	// LPCGUID lpguidSP, LPVOID lpConnection, DWORD dwConnectionSize, LPCDPNAME lpName, DWORD flags, LPVOID lpContext
	lpdpenumconnectionscallback(&DPSP_TCPIP, buf, 80, &dpname, dw, pv);
	free(buf);
	return S_OK;
}
HRESULT __stdcall IDirectPlay_EnumGroupsInGroup(LPUNKNOWN This, DPID dpid,LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumconnectionscallback,LPVOID pv,DWORD dw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_EnumGroupsInGroup", "IDirectPlay_EnumGroupsInGroup", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetGroupConnectionSettings(LPUNKNOWN This, DWORD dw, DPID dpid, LPVOID pv, LPDWORD lpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetGroupConnectionSettings", "IDirectPlay_GetGroupConnectionSettings", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_InitializeConnection(LPUNKNOWN This, LPVOID pv,DWORD dw)
{
	// pv je DPADDRESS* (dplobby.h:689)
	// *pv je GUID DPADDRESS_DATATYPE_TOTALSIZE (dplobby.h:717)
	// *(pv + 16) je DWORD, VŽDYCKY 4 (length of following field)
	// *(pv + 20) je DWORD, length of pv
	// *(pv + 24) guid DATATYPE_DPSP (07D916C0-E0AF-11CF-4E9C-00A0C905425E)
	// *(pv + 40) je DWORD, VŽDYCKY 16 (length of following field = guid)
	// *(pv + 44) je guid pro service provider, viz dplay.h:60
	// *(pv + 60) je PRO TCP/IP guid DATATYPE_IP_CSTRING (C4A54DA0-E0AF-11CF-4E9C-00A0C905425E), mùže být samý 0
	// *(pv + 76) je DWORD, length of further data. pokud byla pøi vytváøení connection specifikována adresa tak je tady jako c-string
	// dw je flags, vždy 0
	IDirectPlay4* t = (IDirectPlay4*)This;
#ifdef _DEBUG
	ShowMemory(pv, *((char*)pv + 20), "IDirectPlay_InitializeConnection:pv");
#endif

	if (IsEqualGUID((char*)pv + 44, &DPSP_TCPIP))
	{
		if ((*((DWORD*)pv + 15) == 0) && (*((DWORD*)pv + 16) == 0) && (*((DWORD*)pv + 17) == 0) && (*((DWORD*)pv + 18) == 0))
			return S_OK;  // guid null
		else if (IsEqualGUID((char*)pv + 60, &DATATYPE_IP_CSTRING) || IsEqualGUID((char*)pv + 60, &DATATYPE_IP_UNICODE))
		{
			// if additional data contains a valid addr => I am client, connect!
			// kontrolovat datatype na (pv + 60) ? mùžeme, ale heroes3 stejnì dají valid datatype a pak ip string == "\0"
			// ošetøit DATATYPE_IP_UNICODE?
			if (*((DWORD*)pv + 19) >= 8)  // pokud to je c-string tak minimálnì 8 (1.1.1.1\0)
			{
				char* caddr, *cport;
				struct addrinfo * result = NULL;
				int freecaddr = 0;
				int freecport = 0;
				if (IsEqualGUID((char*)pv + 60, &DATATYPE_IP_UNICODE))
				{
					char* caddra;
					short* sp = (char*)pv + 80;
					caddra = caddr = malloc(wcslen(sp) + 1);
					while (*sp) *caddra++ = (char)(*sp++);
					*caddra = 0;
					freecaddr = 1;
				}
				else caddr = (char*)pv + 80;
				cport = strchr(caddr, ':');  // zkusit najít ':' -> port (když tam nebude tak dialog? asi jo)
				if (cport)							
				{
					*cport = 0;
					++cport;
				}
				else
				{
					cport = (char*)(DialogBoxParam(__FDP_HM, MAKEINTRESOURCE(IDD_GETREMOTEPORTDLG), GetForegroundWindow(), DialogProc, NULL));
					if (cport == NULL) return DPERR_USERCANCEL;  // uživatel zavøel dialog køížkem
					freecport = 1;
				}
				
				if (getaddrinfo(caddr, cport, NULL, &result) == 0)
				{
					int r = TRUE;
					struct sockaddr_in sin = *((struct sockaddr_in *)result->ai_addr);
					SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &r, 4);
					r = 500;
					setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &r, 4);
					freeaddrinfo(result);
					if (freecaddr) free(caddr);
					else if (freecport) free(cport);  // pokud jsme alokovali caddr tak cport je podmnožinou caddr
					if (connect(s, (struct sockaddr *)(&sin), sizeof(sin)) == 0)
					{
						t->Socket = s;
#ifdef _DEBUG
						MessageBox(GetForegroundWindow(), "IDirectPlay_InitializeConnection connected successfully", "IDirectPlay_InitializeConnection connected successfully", 0);
#endif
						return S_OK;
					}
					else
					{
						MessageBox(GetForegroundWindow(), "ws2::connect failed", "ws2::connect failed", 0);
						return E_FAIL;
					}
				}
				else
				{
					MessageBox(GetForegroundWindow(), "ws2::getaddrinfo failed", "ws2::getaddrinfo failed", 0);
					MessageBox(GetForegroundWindow(), caddr, "caddr", 0);
					MessageBox(GetForegroundWindow(), cport, "cport", 0);
					if (freecaddr) free(caddr);
					else if (freecport) free(cport);
					return E_FAIL;
				}
			}
			else return S_OK;
		}
		else return DPERR_INVALIDPARAM;  // DPSP_TCPIP, ale následující chunk neni ani GUID_NULL ani IP_CSTRING ani IP_UNICODE
	}
	else return DPERR_INVALIDPARAM;
}
HRESULT __stdcall IDirectPlay_SecureOpen(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw,LPDPSECURITYDESC lpdpsecuritydesc,LPDPCREDENTIALS lpdpcredentials)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SecureOpen", "IDirectPlay_SecureOpen", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_SendChatMessage(LPUNKNOWN This, DPID dpid1,DPID dpid2,DWORD dw,LPDPCHAT lpdpchat)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SendChatMessage", "IDirectPlay_SendChatMessage", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_SetGroupConnectionSettings(LPUNKNOWN This, DWORD dw,DPID dpid,LPDPLCONNECTION lpdplconnection)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SetGroupConnectionSettings", "IDirectPlay_SetGroupConnectionSettings", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_StartSession(LPUNKNOWN This, DWORD dw,DPID dpid)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_StartSession", "IDirectPlay_StartSession", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetGroupFlags(LPUNKNOWN This, DPID dpid,LPDWORD lpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetGroupFlags", "IDirectPlay_GetGroupFlags", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetGroupParent(LPUNKNOWN This, DPID dpid1,LPDPID dpid2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetGroupParent", "IDirectPlay_GetGroupParent", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetPlayerAccount(LPUNKNOWN This, DPID dpid, DWORD dw, LPVOID pv, LPDWORD lpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetPlayerAccount", "IDirectPlay_GetPlayerAccount", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetPlayerFlags(LPUNKNOWN This, DPID dpid,LPDWORD lpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetPlayerFlags", "IDirectPlay_GetPlayerFlags", 0);
	return E_FAIL;
}
/*** IDirectPlay4 methods ***/
HRESULT __stdcall IDirectPlay_GetGroupOwner(LPUNKNOWN This, DPID dpid, LPDPID lpdpid)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetGroupOwner", "IDirectPlay_GetGroupOwner", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_SetGroupOwner(LPUNKNOWN This, DPID dpid1, DPID dpid2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SetGroupOwner", "IDirectPlay_SetGroupOwner", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_SendEx(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw1, LPVOID pv1, DWORD dw2, DWORD dw3, DWORD dw4, LPVOID pv2, DWORD_PTR * lplpdw)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_SendEx", "IDirectPlay_SendEx", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_GetMessageQueue(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw, LPDWORD lpdw1, LPDWORD lpdw2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_GetMessageQueue", "IDirectPlay_GetMessageQueue", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_CancelMessage(LPUNKNOWN This, DWORD dw1, DWORD dw2)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_CancelMessage", "IDirectPlay_CancelMessage", 0);
	return E_FAIL;
}
HRESULT __stdcall IDirectPlay_CancelPriority(LPUNKNOWN This, DWORD dw1, DWORD dw2, DWORD dw3)
{
	MessageBox(GetForegroundWindow(), "IDirectPlay_CancelPriority", "IDirectPlay_CancelPriority", 0);
	return E_FAIL;
}