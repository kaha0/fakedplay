#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include<windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<time.h>
#ifdef _DEBUG
#include<stdio.h>
#endif
#include "fakedplay.h"
#include "FakeDirectPlayServer.h"
#include "util.h"

//#define FDP_SELECT
#define FDP_RECV

typedef struct dpClientInfo
{
	DPID dpid;
	SOCKET sock;
	char* name;
	clock_t LastTransmission;
	CRITICAL_SECTION lock;
} dpClientInfo;

SOCKET dpServerSocket;
int dpNumClients;
dpClientInfo** dpClients;
DPID dpNextDPID;
int dpNumPlayers;		// šlo by spoèítat v dpClients ale je lepší si to takhle nacacheovat
int dpMaxPlayers;
DWORD dpSessionFlags;
char* dpSessionName;		// until we start supporting multiple sessions on one server
char* dpSessionPassword;	// until we start supporting multiple sessions on one server
GUID dpAppGuid;			// -||-
GUID dpInstanceGuid;
CRITICAL_SECTION dpDataLock;

void RemoveClient(dpClientInfo* client)  // mam si zamykat sám ? asi jo
{
	int index;
	if (client->dpid)
	{
		client->dpid = 0;  // vynulovat dpid? rozhodnì.
		--dpNumPlayers;  // pokud má DPID tak nebylo voláno DestroyPlayer. BroadcastSysMsgDestroyPlayer bylo (pokud potøeba) voláno pøed zavoláním této fce manuálnì
	}
	closesocket(client->sock);
	client->sock = 0;
	if (client->name)
	{
		free(client->name);
		client->name = NULL;
	}

	// ani nepøealokovávat, jen posunout ty za currplayer
	EnterCriticalSection(&dpDataLock);
	for (index = 0; index < dpNumClients; ++index) if (dpClients[index] == client) break;
	if (index < dpNumClients)
	{
		if (index < dpNumClients - 1) memmove(dpClients + index, dpClients + index + 1, (dpNumClients - index - 1) * sizeof(dpClientInfo*));
		else memset(dpClients + index, 0, sizeof(dpClientInfo*));
		--dpNumClients;
	}
	LeaveCriticalSection(&dpDataLock);
}

void BroadcastSysMsgCreatePlayer(dpClientInfo* client)
{
	int i, j, l, r;
	char* buf;
	DPMSG_CREATEPLAYERORGROUP* p;
#ifdef _DEBUG
	myputs("\r\nBroadcastSysMsgCreatePlayer\r\n");
#endif
	if (client->name)
	{
		l = 12 + sizeof(DPMSG_CREATEPLAYERORGROUP) + strlen(client->name) + 1;  // terminating null character
		buf = malloc(l);
		memcpy(buf + 12 + sizeof(DPMSG_CREATEPLAYERORGROUP), client->name, strlen(client->name) + 1);
	}
	else  // nemìlo by bejt ale pro jistotu
	{
		l = 12 + sizeof(DPMSG_CREATEPLAYERORGROUP);
		buf = malloc(l);
	}
	
	*((DPID*)buf) = 0;		// to (DPID_ALLPLAYERS == 0)
	*((DPID*)(buf + 4)) = 0;  // from (DPID_SYSMSG == 0)
	*((DWORD*)(buf + 8)) = l - 12;  // length of data
	memset(buf + 12, 0, sizeof(DPMSG_CREATEPLAYERORGROUP));
	p = (DPMSG_CREATEPLAYERORGROUP*)(buf + 12);
	p->dwType = DPSYS_CREATEPLAYERORGROUP;
	p->dwPlayerType = DPPLAYERTYPE_PLAYER;
	p->dpId = client->dpid;
	p->dwCurrentPlayers = dpNumPlayers;
	p->dpnName.dwSize = sizeof(DPNAME);

	EnterCriticalSection(&dpDataLock);
	for (i = 0; i < dpNumClients; ++i)
	{
		if (dpClients[i] == client) continue;
		if (dpClients[i]->dpid == 0) continue;

		j = 0;
		EnterCriticalSection(&dpClients[i]->lock);
		while (j < l)
		{
			r = send(dpClients[i]->sock, buf + j, l - j, 0);
			if (r == 0 || r == SOCKET_ERROR) break;
			j += r;
		}
		LeaveCriticalSection(&dpClients[i]->lock);
	}
	LeaveCriticalSection(&dpDataLock);

	free(buf);
}

void BroadcastSysMsgDestroyPlayer(dpClientInfo* client)
{
	int i, j, l, r;
	char* buf;
	DPMSG_DESTROYPLAYERORGROUP* p;
#ifdef _DEBUG
	myputs("\r\nBroadcastSysMsgDestroyPlayer\r\n");
#endif
	if (client->name)
	{
		l = 12 + sizeof(DPMSG_DESTROYPLAYERORGROUP) + strlen(client->name) + 1;  // terminating null character
		buf = malloc(l);
		memcpy(buf + 12 + sizeof(DPMSG_DESTROYPLAYERORGROUP), client->name, strlen(client->name) + 1);
	}
	else
	{
		l = 12 + sizeof(DPMSG_DESTROYPLAYERORGROUP);
		buf = malloc(l);
	}

	*((DPID*)buf) = 0;		// to (DPID_ALLPLAYERS == 0)
	*((DPID*)(buf + 4)) = 0;  // from (DPID_SYSMSG == 0)
	*((DWORD*)(buf + 8)) = l - 12;  // length of data
	memset(buf + 12, 0, sizeof(DPMSG_DESTROYPLAYERORGROUP));
	p = (DPMSG_DESTROYPLAYERORGROUP*)(buf + 12);
	p->dwType = DPSYS_DESTROYPLAYERORGROUP;
	p->dwPlayerType = DPPLAYERTYPE_PLAYER;
	p->dpId = client->dpid;
	p->dpnName.dwSize = sizeof(DPNAME);

	EnterCriticalSection(&dpDataLock);
	for (i = 0; i < dpNumClients; ++i)
	{
		if (dpClients[i] == client) continue;
		if (dpClients[i]->dpid == 0) continue;

		j = 0;
		EnterCriticalSection(&dpClients[i]->lock);
		while (j < l)
		{
			r = send(dpClients[i]->sock, buf + j, l - j, 0);
			if (r == 0 || r == SOCKET_ERROR) break;
			j += r;
		}
		LeaveCriticalSection(&dpClients[i]->lock);
	}
	LeaveCriticalSection(&dpDataLock);

	free(buf);
}

void BroadcastSysMsgSetSessionDesc(dpClientInfo* client)
{
	int i, j, l, r;
	char* buf;
	DPMSG_SETSESSIONDESC* p;
#ifdef _DEBUG
	myputs("\r\nBroadcastSysMsgSetSessionDesc\r\n");
#endif
	if (dpSessionName)
	{
		l = 12 + sizeof(DPMSG_SETSESSIONDESC) + strlen(dpSessionName) + 1;  // terminating null character
		buf = malloc(l);
		memcpy(buf + 12 + sizeof(DPMSG_SETSESSIONDESC), dpSessionName, strlen(dpSessionName) + 1);
	}
	else
	{
		l = 12 + sizeof(DPMSG_SETSESSIONDESC);
		buf = malloc(l);
	}

	*((DPID*)buf) = 0;		// to (DPID_ALLPLAYERS == 0)
	*((DPID*)(buf + 4)) = 0;  // from (DPID_SYSMSG == 0)
	*((DWORD*)(buf + 8)) = l - 12;  // length of data
	memset(buf + 12, 0, sizeof(DPMSG_SETSESSIONDESC));
	p = (DPMSG_SETSESSIONDESC*)(buf + 12);
	p->dwType = DPSYS_SETSESSIONDESC;
	p->dpDesc.dwSize = sizeof(DPSESSIONDESC2);
	p->dpDesc.dwFlags = dpSessionFlags;
	p->dpDesc.guidApplication = dpAppGuid;
	p->dpDesc.dwMaxPlayers = dpMaxPlayers;
	p->dpDesc.dwCurrentPlayers = dpNumPlayers;

	EnterCriticalSection(&dpDataLock);
	for (i = 0; i < dpNumClients; ++i)
	{
		if (dpClients[i] == client) continue;
		if (dpClients[i]->dpid == 0) continue;

		j = 0;
		EnterCriticalSection(&dpClients[i]->lock);
		while (j < l)
		{
			r = send(dpClients[i]->sock, buf + j, l - j, 0);
			if (r == 0 || r == SOCKET_ERROR) break;
			j += r;
		}
		LeaveCriticalSection(&dpClients[i]->lock);
	}
	LeaveCriticalSection(&dpDataLock);

	free(buf);
}

void BroadcastSysMsgSessionLost(dpClientInfo* client)
{
	int i, j, r;
	char* buf = malloc(16);
#ifdef _DEBUG
	myputs("\r\nBroadcastSysMsgSessionLost\r\n");
#endif
	*((DPID*)buf) = 0;		// to (DPID_ALLPLAYERS == 0)
	*((DPID*)(buf + 4)) = 0;  // from (DPID_SYSMSG == 0)
	*((DWORD*)(buf + 8)) = 4;  // length of data
	*((DWORD*)(buf + 12)) = DPSYS_SESSIONLOST;

	EnterCriticalSection(&dpDataLock);
	for (i = 0; i < dpNumClients; ++i)
	{
		if (dpClients[i] == client) continue;
		if (dpClients[i]->dpid == 0) continue;

		j = 0;
		EnterCriticalSection(&dpClients[i]->lock);
		while (j < 16)
		{
			r = send(dpClients[i]->sock, buf + j, 16 - j, 0);
			if (r == 0 || r == SOCKET_ERROR) break;
			j += r;
		}
		LeaveCriticalSection(&dpClients[i]->lock);
	}
	LeaveCriticalSection(&dpDataLock);

	free(buf);
}

DWORD __stdcall FakeDirectPlayServer_Start(LPVOID pv)
{
#ifdef _DEBUG
	AllocConsole();  // https://docs.microsoft.com/en-us/windows/console/allocconsole
	freopen("con", "w", stdout);
	myputs("Server started\r\n");
#endif
	dpNumClients = 0;
	dpClients = NULL;
	dpNextDPID = 1;
	dpNumPlayers = 0;
	dpMaxPlayers = 0;
	dpSessionFlags = 0;
	dpSessionName = NULL;
	dpSessionPassword = NULL;
	dpServerSocket = (SOCKET)pv;  // socket abych ho moh v klientovi zavøít a tim ukonèit server
	InitializeCriticalSection(&dpDataLock);

	memset(&dpInstanceGuid, 0, 16);  // TEMPORARY HACK
	
	FakeDirectPlayServer_AcceptLoop(NULL);

	// TODO : make this NOT global!
	if (dpSessionName) free(dpSessionName);
	if (dpSessionPassword) free(dpSessionPassword);
	if (dpClients) free(dpClients);
	DeleteCriticalSection(&dpDataLock);

#ifdef _DEBUG
	myputs("Server exiting\r\n");
	//FreeConsole();
#endif

	return 0;
}

DWORD __stdcall FakeDirectPlayServer_AcceptLoop(LPVOID pv)
{
	struct sockaddr remote;
	int remotelen;
	SOCKET s;
	dpClientInfo* client;
	dpClientInfo** aux;

	while (1)
	{
		if (!dpServerSocket)
		{
#ifdef _DEBUG
			myputs("FakeDirectPlayServer_AcceptLoop: ServerSocket = 0\r\n");
#endif
			break;
		}
		remotelen = sizeof(remote);
		s = accept(dpServerSocket, &remote, &remotelen);
		if (s == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			if (err == WSAEINTR) continue;  // blocking operation was interrupted by a call to WSACancelBlockingCall
#ifdef _DEBUG
			printf("FakeDirectPlayServer_AcceptLoop: accept returned INVALID_SOCKET (%d)\r\n", err);
#endif
			dpServerSocket = 0;
			break;
		}

#ifdef _DEBUG
		myputs("Accepted connection from: ");
		myputs(inet_ntoa(((struct sockaddr_in*)(&remote))->sin_addr));
		myputs(":");
		myputd(((struct sockaddr_in*)(&remote))->sin_port);
		myputs("\r\n");
#endif						//https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-getsockopt
		remotelen = TRUE;	//https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-setsockopt
		setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &remotelen, 4);
			// (timeout_infinite = 0, but we do want a timeout to prevent getting stuck in the switch)
		remotelen = 10000;	// 10s?
		setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &remotelen, 4);
		setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &remotelen, 4);

		// add to dpClients right here ! that means we need another lock for global data structures
		client = malloc(sizeof(dpClientInfo));
		client->sock = s;
		client->dpid = 0;
		client->name = NULL;
		client->LastTransmission = clock();
		InitializeCriticalSection(&client->lock);

		EnterCriticalSection(&dpDataLock);
		aux = malloc((dpNumClients + 1) * sizeof(dpClientInfo*));
		if (dpClients)
		{
			memcpy(aux, dpClients, dpNumClients * sizeof(dpClientInfo*));
			free(dpClients);
		}
		aux[dpNumClients] = client;
		dpClients = aux;
		++dpNumClients;
		LeaveCriticalSection(&dpDataLock);

		// and then start a new thread and pass newly accepted socket as param
		CreateThread(NULL, 0, FakeDirectPlayServer_MessageLoop, client, 0, NULL);
	}
#ifdef _DEBUG
	myputs("FakeDirectPlayServer_AcceptLoop: exiting\r\n");
#endif
	return 0;
}

void FakeDirectPlayServer_MessageLoop_Inner(dpClientInfo* client, int bufsize, char* buf);

DWORD __stdcall FakeDirectPlayServer_MessageLoop(LPVOID pv)
{
	dpClientInfo* client = (dpClientInfo*)pv;

	FakeDirectPlayServer_MessageLoop_Inner(client);

	DeleteCriticalSection(&client->lock);
	free(client);
	return 0;
}

void FakeDirectPlayServer_MessageLoop_Inner(dpClientInfo* client)  // do not call this function directly, call FakeDirectPlayServer_MessageLoop instead
{
	int bufsize = 1024;
	char* buf = malloc(bufsize);
	int r;
	SOCKET s = client->sock;
#ifdef _DEBUG
	unsigned int nummessages = 0;
	unsigned int longestmessage = 0;
#endif
#ifdef FDP_RECV
	int timeout;
#endif
#ifdef FDP_SELECT
	fd_set rset;
	TIMEVAL timeval;
	timeval.tv_sec = 0;
	timeval.tv_usec = 500000;
#endif
	
	while (1)
	{
		if (!dpServerSocket)  // host pošle TerminateSession -> klientùm se rozešle SessionLost (vlákno hostujícího klienta zavolá Broadcast() ). pak se vynuluje serversocket. => teï už mùžu v klidu ukonèit
		{
			RemoveClient(client);
			return;
		}

		/*if ((clock() - dpClients[currclient].LastTransmission) / CLOCKS_PER_SEC > 60)
		{
			// assume connection was interrupted quietly (ie cable unplugged)
#ifdef _DEBUG
			struct sockaddr_in sa;
			r = sizeof(sa);
			memset(&sa, 0, r);
			getpeername(s, &sa, &r);
			myputd(nummessages++);
			myputs(": no transmission from ");
			if (dpClients[currclient].dpid)
			{
				myputs("player ");
				myputs(dpClients[currclient].name);
				myputs(" (DPID:");
				myputd(dpClients[currclient].dpid);
				myputs(") (");
			}
			myputs(inet_ntoa(sa.sin_addr));
			myputs(":");
			myputd(sa.sin_port);
			if (dpClients[currclient].dpid) myputs(")");
			myputs(" in 60 seconds, assuming connection closed\r\n");
#endif
			if (dpClients[currclient].dpid) BroadcastSysMsgDestroyPlayer(currclient);
			closesocket(s);
			RemoveClient(currclient);
			continue;
		}*/

#ifdef FDP_RECV
		timeout = 10;
		r = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, 4);
		if (r != 0)						//https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-setsockopt
		{								//https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-getsockopt
			myputs("setsockopt returns error: ");
			myputd(WSAGetLastError());
			myputs("\r\n");
		}
#endif
#ifdef FDP_SELECT
		rset.fd_count = 1;
		rset.fd_array[0] = s;
		r = select(0, &rset, NULL, NULL, &timeval);  // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select
		if (r == 0) continue;  // select() timed out
#endif
		r = recv(s, buf, 1, 0);
		if (r == 0 ||  // connection closed -> odstranit z dpClients
			(r == SOCKET_ERROR && (r = WSAGetLastError()) != WSAETIMEDOUT))  // WSAETIMEDOUT == no data
		{
#ifdef _DEBUG
			struct sockaddr_in sa;
			r = sizeof(sa);
			memset(&sa, 0, r);
			getpeername(s, &sa, &r);
			myputd(nummessages++);
			myputs(": connection to ");
			if (client->dpid)
			{
				myputs("player ");
				myputs(client->name);
				myputs(" (DPID:");
				myputd(client->dpid);
				myputs(") (");
			}
			myputs(inet_ntoa(sa.sin_addr));
			myputs(":");
			myputd(sa.sin_port);
			if (client->dpid) myputs(")");
			myputs(" closed\r\n");
#endif
			if (client->dpid) BroadcastSysMsgDestroyPlayer(client);  // má dpid => ještì jsme ho nedestruovali
			RemoveClient(client);
			return;
		}
		else if (r == WSAETIMEDOUT) continue;

#ifdef FDP_RECV
		// reset timeout to old value (timeout_infinite = 0, but we do want a timeout to prevent getting stuck in the switch)
		timeout = 10000;  // 10 seconds?
		r = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, 4);
		if (r != 0)
		{
			myputs("setsockopt 2 returns error: ");
			myputd(WSAGetLastError());
			myputs("\r\n");
		}
#endif
		client->LastTransmission = clock();
#ifdef _DEBUG
		myputs("clientSOCK: ");
		myputd(client->sock);
		if (client->dpid)
		{
			myputs(", DPID:");
			myputd(client->dpid);
		}
		myputs(", #clients: ");
		myputd(dpNumClients);
		myputs(", #players: ");
		myputd(dpNumPlayers);
		myputs(", *buf: ");
		myputd((unsigned char)*buf);
		myputs(", longestmsg=");
		myputd(longestmessage);
		myputs("\r\n");
#endif

		switch (*buf)
		{
		case FDP_MSG_EnumSessions:
			{
#ifdef _DEBUG
				printf("%d: serving EnumSessions msg... ", nummessages++);
#endif
				if (dpSessionName)
				{
					int l, i = 1;
					send(s, &i, 4, 0);
					send(s, &dpSessionFlags, 4, 0);
					send(s, &dpMaxPlayers, 4, 0);
					send(s, &dpNumPlayers, 4, 0);
					l = strlen(dpSessionName);
					//if (l > 255) l = 255;  zbyteèný, server dostal od klienta a tam to kontrolujeme
					r = send(s, &l, 1, 0);
					if (r == 0 || r == SOCKET_ERROR)
					{
#ifdef _DEBUG
						puts("ERROR");
#endif
						continue;  // mìli bysme RemoveClient(currplayer) ale mùžeme nechat na prvním recv nahoøe
					}// vlastnì je ideální nechat to na 1. recv nahoøe, museli bysme tady volat i BroadcastSysMsgDestroyPlayer(currclient);
					
					i = 0;
					while (i < l)
					{
						r = send(s, dpSessionName + i, l - i, 0);
						if (r == 0 || r == SOCKET_ERROR) break;
						i += r;
					}
					//if (r == 0 || r == SOCKET_ERROR) break;
				}
				else
				{
					int i = 0;
					r = send(s, &i, 4, 0);
					//if (r == 0 || r == SOCKET_ERROR) break;
				}
#ifdef _DEBUG
				puts("OK");
#endif
				continue;
			}  //</EnumSessions>
		case FDP_MSG_JoinSession:
			{
				int i = 0;
				int l;
#ifdef _DEBUG
				printf("%d: serving JoinSession msg... ", nummessages++);
#endif
				if (dpSessionFlags & DPSESSION_JOINDISABLED)
				{
					l = FDP_MSG_No;
					send(s, &l, 1, 0);
					RemoveClient(client);
					closesocket(s);
#ifdef _DEBUG
					myputs("REJECTED\r\n");
#endif
					continue;
				}
				l = FDP_MSG_Yes;
				send(s, &l, 1, 0);
				l = 0;

				recv(s, &l, 1, 0);
				while (i < l)
				{
					r = recv(s, buf + i, l - i, 0); // password
					if (r == 0 || r == SOCKET_ERROR) break;
					i += r;
				}
				*(buf + l) = 0;

				if (((dpSessionFlags & DPSESSION_PASSWORDREQUIRED) == 0)
					|| (strcmp(dpSessionPassword, buf) == 0))
				{
					i = FDP_MSG_Yes;
					send(s, &i, 1, 0);  // neni tøeba kontrolovat r, pokraèovalo by se stejnì
#ifdef _DEBUG
					puts("OK");
#endif
					continue;
				}
				else
				{
					i = FDP_MSG_No;
					send(s, &i, 1, 0);
					closesocket(s);
					RemoveClient(client);
#ifdef _DEBUG
					myputs("INVALID PASSWORD (");
					myputs(buf);
					myputs(")\r\n");
#endif
					continue;
				}
			}  // </JoinSession>
		case FDP_MSG_CreateSession:
			{
				int i = 0;
				int l = 0;
#ifdef _DEBUG
				printf("%d: serving CreateSession msg... ", nummessages++);
#endif
				// if (dpSessionName) nemá smysl kontrolovat, createsession se posílá jenom po vytvoøení serveru
				recv(s, &i, 4, 0);
				dpSessionFlags = DPSESSION_KEEPALIVE;// | DPSESSION_CLIENTSERVER;
				if (i & DPSESSION_NEWPLAYERSDISABLED) dpSessionFlags |= DPSESSION_NEWPLAYERSDISABLED;
				if (i & DPSESSION_JOINDISABLED) dpSessionFlags |= DPSESSION_JOINDISABLED;
				if (i & DPSESSION_PASSWORDREQUIRED) dpSessionFlags |= DPSESSION_PASSWORDREQUIRED;
				if (i & DPSESSION_NOSESSIONDESCMESSAGES) dpSessionFlags |= DPSESSION_NOSESSIONDESCMESSAGES;
				recv(s, &dpAppGuid, 16, 0);
				r = recv(s, &l, 1, 0);
				if (r == 0 || r == SOCKET_ERROR)
				{
#ifdef _DEBUG
					puts("ERROR");
#endif
					continue;
				}
				dpSessionName = malloc(l + 1);
				i = 0;
				while (i < l)
				{
					r = recv(s, dpSessionName + i, l - i, 0);  // session name
					if (r == 0 || r == SOCKET_ERROR) break;
					i += r;
				}
				if (r == 0 || r == SOCKET_ERROR)
				{
#ifdef _DEBUG
					puts("ERROR");
#endif
					free(dpSessionName);
					dpSessionName = NULL;
					continue;
				}
				*(dpSessionName + l) = 0;
#ifdef _DEBUG
				printf("(%s) ", dpSessionName);
#endif

				l = 0;
				r = recv(s, &l, 1, 0);  // necheckovat r tady, až po while-cyklu
				if (l)
				{
					dpSessionPassword = malloc(l + 1);
					i = 0;
					while (i < l)
					{
						r = recv(s, dpSessionPassword + i, l - i, 0); // password
						if (r == 0 || r == SOCKET_ERROR) break;
						i += r;
					}
					if (r == 0 || r == SOCKET_ERROR)
					{
#ifdef _DEBUG
						puts("ERROR");
#endif
						free(dpSessionPassword);
						dpSessionPassword = NULL;
						free(dpSessionName);
						dpSessionName = NULL;
						continue;
					}
					*(dpSessionPassword + l) = 0;
#ifdef _DEBUG
					printf("(%s) ", dpSessionPassword);
#endif
				}
				recv(s, &dpMaxPlayers, 4, 0);
				i = FDP_MSG_Yes;
				r = send(s, &i, 1, 0);
				if (r == 0 || r == SOCKET_ERROR)
				{
#ifdef _DEBUG
					puts("ERROR");
#endif
					if (dpSessionPassword) free(dpSessionPassword);
					dpSessionPassword = NULL;
					free(dpSessionName);
					dpSessionName = NULL;
					continue;
				}
#ifdef _DEBUG
				puts("OK");
#endif
				continue;
			}  // </CreateSession>
		case FDP_MSG_CreatePlayer:  // <CreatePlayer>
			{
				int i, l = 0;
				char* b;
#ifdef _DEBUG
				printf("%d: serving CreatePlayer msg... ", nummessages++);
#endif
				if ((dpSessionFlags & DPSESSION_NEWPLAYERSDISABLED) || (dpNumPlayers >= dpMaxPlayers))
				{
					i = FDP_MSG_No;
					send(s, &i, 1, 0);
#ifdef _DEBUG
					puts("FAIL");
#endif
					continue;
				}

				i = FDP_MSG_Yes;
				send(s, &i, 1, 0);
				r = recv(s, &l, 1, 0);
				if (r == 0 || r == SOCKET_ERROR)
				{
#ifdef _DEBUG
					puts("ERROR");
#endif
					continue;
				}
				b = malloc(l + 1);
				i = 0;
				while (i < l)
				{
					r = recv(s, b + i, l - i, 0);
					if (r == 0 || r == SOCKET_ERROR) break;
					i += r;
				}
				// mùžeme si dovolit tady nekontrolovat r, zkontroluje se o kousek níž
				*(b + l) = 0;
#ifdef _DEBUG
				printf("(%s) ", b);
#endif

				EnterCriticalSection(&dpDataLock);
				r = send(s, &dpNextDPID, 4, 0);  // není žádoucí teï kontrolovat r. pokud se socket zavøel tak tady na serveru dokonèit CreatePlayer a jsou struktury v konzistentním stavu a až potom ošetøit
				client->dpid = dpNextDPID;  // client->dpid nastavit až na konci, aby mi mezitim do socketu jiný vlákno nechtìlo posílat sendy (nebo se zamknout?)
				++dpNextDPID;
				client->name = b;
				// TODO : všechno nechat zamèený po celou dobu!
				if (r == 0 || r == SOCKET_ERROR)	// ani si nemusíme uschovávat retval z recv(name), pokud byl error tam tak bude i tady
				{
					// neposílat BroadcastSysMsgDestroyPlayer(currclient);
					LeaveCriticalSection(&dpDataLock);
					RemoveClient(client);
#ifdef _DEBUG
					puts("ERROR");
#endif
					continue;
				}
				
				// then send relevant subset of sessiondesc
				send(s, &dpSessionFlags, 4, 0);
				send(s, &dpAppGuid, 16, 0);
				send(s, &dpInstanceGuid, 16, 0);
				send(s, &dpMaxPlayers, 4, 0);
				send(s, &dpNumPlayers, 4, 0);
				l = strlen(dpSessionName);
				r = send(s, &l, 1, 0);
				if (r == 0 || r == SOCKET_ERROR)  // staèí liberálnì, pokud fail tak sendy spolehlivì zfailujou a chybu objevíme v recv(s,buf,1,0) nahoøe. ale technicky bysme tady mìli držet dpDataLock celou dobu
				{
					LeaveCriticalSection(&dpDataLock);
					RemoveClient(client);
#ifdef _DEBUG
					puts("ERROR");
#endif
					continue;
				}
				i = 0;
				while (i < l)
				{
					r = send(s, dpSessionName + i, l - i, 0);
					if (r == 0 || r == SOCKET_ERROR) break;
					i += r;
				}
				if (r == 0 || r == SOCKET_ERROR)
				{
					LeaveCriticalSection(&dpDataLock);
					RemoveClient(client);
#ifdef _DEBUG
					puts("ERROR");
#endif
					continue;
				}

				// then send players info (ID, namelen, name)
				for (i = 0; i < dpNumClients; ++i)
				{
					int j;
					if (dpClients[i]->dpid == 0) continue;
					if (dpClients[i] == client) continue;

					send(s, &dpClients[i]->dpid, 4, 0);
					if (dpClients[i]->name) l = strlen(dpClients[i]->name);
					else l = 0;
					send(s, &l, 1, 0);
					j = 0;
					while (j < l)
					{
						r = send(s, dpClients[i]->name + j, l - j, 0);
						if (r == 0 || r == SOCKET_ERROR) break;
						j += r;
					}
				}
				LeaveCriticalSection(&dpDataLock);
				++dpNumPlayers;
				BroadcastSysMsgCreatePlayer(client);  // client nemá dpid! TODO : napøed komplet vytvoøit klienta (v zamèenym bloku), až pak broadcast! (zamknout aby na novýho klienta nešahaly ostatní vlákna)
				// TODO : pak taky novej klient nedostane sám sebe v enumeraci playerù! pøidá si sám sebe manuálnì pøed návratem z IDP_CreatePlayer ? a dpNumPlayers !
#ifdef _DEBUG
				puts("OK");
#endif
				
				continue;
			}  // </CreatePlayer>
		case FDP_MSG_EnumPlayers:  // <EnumPlayers>
			{
				int i, j, l;
#ifdef _DEBUG
				printf("%d: serving EnumPlayers msg... ", nummessages++);
#endif
				r = send(s, &dpNumPlayers, 4, 0);
				if (r == 0 || r == SOCKET_ERROR)
				{
#ifdef _DEBUG
					myputs("ERROR\r\n");
#endif
					continue;
				}
				for (i = 0; i < dpNumClients; ++i)
				{  // posílat pouze ty klienty který jsou opravdu players, tzn volali CreatePlayer a mají validní DPID
					if (dpClients[i]->dpid)
					{
						send(s, &dpClients[i]->dpid, 4, 0);
						l = strlen(dpClients[i]->name);
						r = send(s, &l, 1, 0);
						j = 0;
						while (j < l)
						{
							r = send(s, dpClients[i]->name + j, l - j, 0);
							if (r == 0 || r == SOCKET_ERROR) break;
							j += r;
						}
						if (r == 0 || r == SOCKET_ERROR) break;
					}
				}
#ifdef _DEBUG
				puts("OK");
#endif
				continue;
			}  // </EnumPlayers>
		case FDP_MSG_DestroyPlayer:
			{
#ifdef _DEBUG
				myputd(nummessages++);
				myputs(": serving DestroyPlayer msg... (");
				myputs(client->name);
				myputs(") ");
#endif
				// klient mi posílá abych znièil jeho playera
				BroadcastSysMsgDestroyPlayer(client);
				client->dpid = 0;
				if (client->name)
				{
					free(client->name);
					client->name = NULL;
				}
				--dpNumPlayers;
#ifdef _DEBUG
				puts("OK");
#endif
				continue;
			}
		case FDP_MSG_Send:  // <Send>  DPID to, DPID from, DWORD len, data
			{
				DPID to, from;  // do MessageQueue musim dávat obojí: to i from
				int i, l = 0;
				recv(s, &to, 4, 0);
				recv(s, &from, 4, 0);
				r = recv(s, &l, 4, 0);
#ifdef _DEBUG
				myputd(nummessages++);
				myputs(": serving Send msg... (who:");
				myputd(client->dpid);
				myputs(") (to:");
				myputd(to);
				myputs(") (from:");
				myputd(from);
				if (from != client->dpid)
				{
					myputs(") (really from:");
					myputd(client->dpid);
				}
				myputs(") (length:");
				myputd(l);
				myputs(") ");
#endif
				if (l + 12 > bufsize)
				{
					free(buf);
					bufsize = l + 12;
					buf = malloc(l + 12);
				}
				*(DPID*)buf = to;
				*(DPID*)(buf + 4) = from;
				*(DWORD*)(buf + 8) = l;
				i = 0;
				while (i < l)
				{
					r = recv(s, buf + 12 + i, l - i, 0);
					if (r == 0 || r == SOCKET_ERROR) break;
					i += r;
				}
				if (r == 0 || r == SOCKET_ERROR)
				{
#ifdef _DEBUG
					puts("ERROR");
#endif
					continue;
				}
				//if (dpClients[currclient].dpid == 0) continue; ? nemìlo by nastat
/*#ifdef _DEBUG
				myputs("\r\n");
				PrintMemory(buf, l);
				myputs("\r\n    ");
#endif*/

				EnterCriticalSection(&dpDataLock);
				for (i = 0; i < dpNumClients; ++i)
				{
					//if (from == dpClients[i].dpid) continue;  // message neposíláme tomu, od koho pøišla (kontroloval jsem v oficiálním dplayx)
					if (dpClients[i] == client) continue;
					if (dpClients[i]->dpid == 0) continue;
					if (to == 0 || dpClients[i]->dpid == to)
					{
						int j = 0;
						EnterCriticalSection(&dpClients[i]->lock);
						while (j < l)
						{
							r = send(dpClients[i]->sock, buf, l + 12, 0);
							if (r == 0 || r == SOCKET_ERROR) break;
							j += r;
						}
						LeaveCriticalSection(&dpClients[i]->lock);
						if (dpClients[i]->dpid == to) break;
					}
				}
				LeaveCriticalSection(&dpDataLock);
#ifdef _DEBUG
				if (l > longestmessage) longestmessage = l;
				puts("OK");
#endif
				continue;
			}  // </Send>
		case FDP_MSG_GetSessionDesc:
			{
				int i = 0;
				int l;
#ifdef _DEBUG
				myputd(nummessages++);
				myputs(": serving GetSessionDesc msg (");
				myputs(dpSessionName);
				myputs(")... ");
#endif
				if (dpSessionName) l = strlen(dpSessionName);
				else l = 0;  // nemìlo by se stát? app by mìla volat GetSessionDesc až po CreateSession, ale...
				send(s, &l, 1, 0);
				recv(s, &i, 1, 0);
				if (i == FDP_MSG_No)
				{
#ifdef _DEBUG
					puts("(rejected)");
#endif
					continue;
				}
				send(s, &dpSessionFlags, 4, 0);
				send(s, &dpAppGuid, 16, 0);
				send(s, &dpMaxPlayers, 4, 0);
				r = send(s, &dpNumPlayers, 4, 0);
				if (r == 0 || r == SOCKET_ERROR)
				{
#ifdef _DEBUG
					puts("ERROR");
#endif
					continue;
				}
				i = 0;
				while (i < l)
				{
					r = send(s, dpSessionName + i, l - i, 0);
					if (r == 0 || r == SOCKET_ERROR) break;
					i += r;
				}
				// není tøeba tady kontrolovat r
#ifdef _DEBUG
				puts("OK");
#endif
				continue;
			}  // </GetSessionDesc>
		case FDP_MSG_SetSessionDesc:
			{
				// check that it's the host player sending this msg?
				// check that we're in a session already??
				// password?? doesn't make sense i think
				int i = 0;
				int l = 0;
				char* b;
#ifdef _DEBUG
				printf("%d: serving SetSessionDesc msg... ", nummessages++);
#endif
				recv(s, &i, 4, 0);
				dpSessionFlags = DPSESSION_KEEPALIVE;// | DPSESSION_CLIENTSERVER;
				if (i & DPSESSION_NEWPLAYERSDISABLED) dpSessionFlags |= DPSESSION_NEWPLAYERSDISABLED;
				if (i & DPSESSION_JOINDISABLED) dpSessionFlags |= DPSESSION_JOINDISABLED;
				if (i & DPSESSION_PASSWORDREQUIRED) dpSessionFlags |= DPSESSION_PASSWORDREQUIRED;
				if (i & DPSESSION_NOSESSIONDESCMESSAGES) dpSessionFlags |= DPSESSION_NOSESSIONDESCMESSAGES;
				recv(s, &dpMaxPlayers, 4, 0);
				r = recv(s, &l, 1, 0);
				b = malloc(l + 1);
				i = 0;
				while (i < l)
				{
					r = recv(s, b + i, l - i, 0);
					if (r == 0 || r == SOCKET_ERROR) break;
					i += r;
				}
				if (r == 0 || r == SOCKET_ERROR)
				{
#ifdef _DEBUG
					puts("ERROR");
#endif
					free(b);
					continue;
				}
				*(b + l) = 0;
#ifdef _DEBUG
				printf("(%s) ", b);
#endif

				if (dpSessionName) free(dpSessionName);
				dpSessionName = b;
				if ((dpSessionFlags & DPSESSION_NOSESSIONDESCMESSAGES) == 0) BroadcastSysMsgSetSessionDesc(client);
#ifdef _DEBUG
				puts("OK");
#endif
				continue;
			}  // </SetSessionDesc>
		case FDP_MSG_TerminateSession:
			{
				BroadcastSysMsgSessionLost(client);
				return;
			}
		default: break;
		}

#ifdef _DEBUG
		r = recv(s, buf + 1, bufsize, 0);
		myputs("fell out of switch! data:");
		PrintMemory(buf, r + 1);
		myputs("\r\n");
#endif
	}

	return 0;
}
