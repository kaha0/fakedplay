#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include<windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include "fakedplay.h"
#include "FakeDirectPlayClient.h"

int CheckAndCompactMessageQueue(IDirectPlay4* t, int length)  // call only when locked !
{
	// zkontrolovat jestli se vejde do queue nebo ne, pøidat
	if (t->MessageQueueEnd + length > t->MessageQueueBeginning + dpMessageQueueLength)  // z dpMessageQueueLength udìlat promìnnou místo konstanty
	{
		int d;
		while (t->MessageQueueEnd + length - (t->MessageQueue - t->MessageQueueBeginning) > t->MessageQueueBeginning + dpMessageQueueLength)
		{
			//MessageBox(GetForegroundWindow(), "MessageQueue is full, and not even compacting it would help!", "FakeDirectPlay", MB_ICONERROR);
			//return 0;
			LeaveCriticalSection(&t->Lock);
			Sleep(100);
			EnterCriticalSection(&t->Lock);
		}
		
		// posunout (aka queue compacting)
		d = t->MessageQueueEnd - t->MessageQueue;
		memmove(t->MessageQueueBeginning, t->MessageQueue, d);
		t->MessageQueueEnd = t->MessageQueueBeginning + d;
		t->MessageQueue = t->MessageQueueBeginning;
		return 1;
	}
	return 1;
}

void AppendSysMsgSessionLost(IDirectPlay4* t)
{
#ifdef _DEBUG
	MessageBoxA(GetForegroundWindow(), "FakeDirectPlayClient::AppendSysMsgSessionLost", "FakeDirectPlayClient::AppendSysMsgSessionLost", 0);
#endif
	EnterCriticalSection(&t->Lock);
	if (CheckAndCompactMessageQueue(t, 16))
	{
		*((DPID*)t->MessageQueueEnd) = 0;
		t->MessageQueueEnd += 4;
		*((DPID*)t->MessageQueueEnd) = 0;
		t->MessageQueueEnd += 4;
		*((DWORD*)t->MessageQueueEnd) = 4;
		t->MessageQueueEnd += 4;
		*(DWORD*)t->MessageQueueEnd = DPSYS_SESSIONLOST;
		t->MessageQueueEnd += 4;
	}
	LeaveCriticalSection(&t->Lock);
}

int HandleSocketRetVal(IDirectPlay4* t, int r)  // vrací 1 pokud má vlákno pokraèovat v msgloopu, 0 pokud má skonèit
{
	if (r == 0)
	{
		// connection was closed -> šoupnout DPSYS_SESSIONLOST, ale nenulovat socket protože fce IDP_xxx spoléhají na nenulovost socketu (jinak vrátí rovnou DPERR_UNINITIALIZED)
		AppendSysMsgSessionLost(t);
		return 0;
	}
	else if (r == SOCKET_ERROR)
	{
		if ((r = WSAGetLastError()) == WSAETIMEDOUT)  // WSAETIMEDOUT == no data -> just continue
			return 1;
		else
		{
			// check r?
			// šoupnout DPSYS_SESSIONLOST
			AppendSysMsgSessionLost(t);
			return 0;
		}
	}
	return 0;
}

DWORD __stdcall FakeDirectPlayClient_Start(LPVOID pv)
{
	IDirectPlay4 * t = (IDirectPlay4*)pv;
	int bufsize = 2048;
	char* buf = malloc(bufsize);
	int i, r;
	DPID from, to;
	DWORD l;

	while (t->Socket)
	{
		r = recv(t->Socket, buf, 12, 0);
		if (r <= 0)  // TODO : if (r != 12) ??
		{
			if (HandleSocketRetVal(t, r)) continue;
			else break;
		}
		to = *((DPID*)buf);
		from = *((DPID*)(buf + 4));
		l = *((DWORD*)(buf + 8));
		if (l > bufsize)
		{
			bufsize = l;
			free(buf);
			buf = malloc(bufsize);
		}

		i = 0;
		while (i < l)
		{
			r = recv(t->Socket, buf + i, l - i, 0);
			if (r <= 0) break;
			i += r;
		}
		if (r <= 0)
		{
			if (HandleSocketRetVal(t, r)) continue;  // TODO : má smysl tady dávat continue? moc ne, byla by rozjetá synchronizace
			else break;
		}

		// append to msgqueue
		EnterCriticalSection(&t->Lock);
		if (CheckAndCompactMessageQueue(t, l + 12))
		{
			*((DPID*)t->MessageQueueEnd) = to;
			t->MessageQueueEnd += 4;
			*((DPID*)t->MessageQueueEnd) = from;
			t->MessageQueueEnd += 4;
			*((DWORD*)t->MessageQueueEnd) = l;
			t->MessageQueueEnd += 4;
			memcpy(t->MessageQueueEnd, buf, l);
			t->MessageQueueEnd += l;
		}
		LeaveCriticalSection(&t->Lock);
	}

	free(buf);
	return 0;
}