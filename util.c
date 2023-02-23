#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include<windows.h>
#include "util.h"

HMODULE __FDP_HM = 0;

LRESULT WINAPI DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			return TRUE;
		}
	case WM_COMMAND:
		{
			if (wParam == ((BN_CLICKED << 16) | IDOK))
			{
				char* r = malloc(22);
				GetDlgItemText(hWnd, ID_EDIT, r, 22);
				EndDialog(hWnd, r);
			}
			return TRUE;
		}
	case WM_CLOSE:
		{
			EndDialog(hWnd, 0);
			return TRUE;
		}
	default:
		{
			return FALSE;
		}
	}
}

void GuidToStr(GUID* guid, char* buf)  // stringová reprezentace guid zabere 2*4 + 2*2 + 2*2 + 2*8  + 4 + 1 = 32 + 4 + 1 = 37B
{										// + 2 = 39B na složený závorky
	*buf++ = '{';
	sprintf(buf, "%08X", guid->Data1);
	buf += 8;
	*buf++ = '-';
	sprintf(buf, "%04X", guid->Data2);
	buf+=4;
	*buf++ = '-';
	sprintf(buf, "%04X", guid->Data3);
	buf+=4;
	*buf++ = '-';
	sprintf(buf, "%02X", guid->Data4[0]);
	buf += 2;
	sprintf(buf, "%02X", guid->Data4[1]);
	buf += 2;
	*buf++ = '-';
	sprintf(buf, "%02X", guid->Data4[2]);
	buf += 2;
	sprintf(buf, "%02X", guid->Data4[3]);
	buf += 2;
	sprintf(buf, "%02X", guid->Data4[4]);
	buf += 2;
	sprintf(buf, "%02X", guid->Data4[5]);
	buf += 2;
	sprintf(buf, "%02X", guid->Data4[6]);
	buf += 2;
	sprintf(buf, "%02X", guid->Data4[7]);
	buf += 2;
	*buf++ = '}';
	*buf = 0;
}

int IsIPAddressInUNICODE(char* a)
{
	int i = 0;
	if (!a) return 0;
	while (1)
	{
		if ((*a == '.') && (i > 0)) break;
		if (*a < '0' || *a > '9') return 0;
		++a;
		if (*a++ != 0) return 0;
		++i;
		if (i > 3) return 0;
	}
	++a;
	if (*a++ != 0) return 0;
	i = 0;
	while (1)
	{
		if ((*a == '.') && (i > 0)) break;
		if (*a < '0' || *a > '9') return 0;
		++a;
		if (*a++ != 0) return 0;
		++i;
		if (i > 3) return 0;
	}
	++a;
	if (*a++ != 0) return 0;
	i = 0;
	while (1)
	{
		if ((*a == '.') && (i > 0)) break;
		if (*a < '0' || *a > '9') return 0;
		++a;
		if (*a++ != 0) return 0;
		++i;
		if (i > 3) return 0;
	}
	++a;
	if (*a++ != 0) return 0;
	i = 0;
	while (1)
	{
		if ((*a == 0) && (*(a + 1) == 0) && (i > 0)) return 1;
		if ((*a == ':') && (i > 0)) break;
		if (*a < '0' || *a > '9') return 0;
		++a;
		if (*a++ != 0) return 0;
		++i;
		if (i > 3) return 0;
	}
	++a;
	if (*a++ != 0) return 0;
	i = 0;
	while (1)
	{
		if ((*a == 0) && (*(a + 1) == 0) && (i > 0)) return 1;
		if (*a < '0' || *a > '9') return 0;
		++a;
		if (*a++ != 0) return 0;
		++i;
		if (i > 5) return 0;
	}
}

//#ifdef _DEBUG
void ShowMemory(void* pv, long l, char* t)
{
	int i;
	unsigned char c;
	unsigned char * p = pv;
	char* b;
	char* buf;
	char* bt;

	if (pv == NULL)
	{
		MessageBox(GetForegroundWindow(), "NULL!", t, 0);
		return;
	}
	buf = malloc(3 * l + 1);
	b = buf;
	bt = malloc(strlen(t) + 15);
	bt[0] = 'T';
	bt[1] = 'I';
	bt[2] = 'D';
	bt[3] = ':';
	itoa(GetCurrentThreadId(), bt + 4, 10);
	strcat(bt, t);
	for (i = 0; i < l; ++i)
	{
		c = (*p) >> 4;
		*b = c > 9 ? c - 10 + 'A' : c + '0';
		++b;
		c = (*p) & 0xF;
		*b = c > 9 ? c - 10 + 'A' : c + '0';
		++b;
		*b = ' ';
		++b;
		++p;
	}
	*b = 0;
	MessageBox(GetForegroundWindow(), buf, bt, 0);
	free(buf);
	free(bt);
}
void PrintMemory(void* pv, long l)
{
	int i, d;
	unsigned char c;
	unsigned char * p = pv;
	DWORD b;

	if (pv == NULL) return;
	for (i = 0; i < l; ++i)
	{
		b = 0x200000;
		c = (*p) >> 4;
		b |= c > 9 ? c - 10 + 'A' : c + '0';
		c = (*p) & 0xF;
		b |= (c > 9 ? c - 10 + 'A' : c + '0') << 8;
		++p;
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &b, 3, &d, NULL);
	}
}
void myputs(char* c)
{
	int d;
	if (c == NULL) return;
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), c, mystrlen(c), &d, NULL);
}
void myputd(unsigned int v)
{
	char b[12];
	char* a = &b[11];
	*a = 0;
	if (v == 0)
	{
		*--a = '0';
		myputs(a);
		return;
	}
	while (v)
	{
		*--a = (v % 10) + '0';
		v /= 10;
	}
	myputs(a);
}
int mystrlen(char* c)
{
	char* a = c;
	if (c == NULL) return 0;
	while (*a) ++a;
	return a - c;
}
//#endif

GUID IID_IUnknown = {0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

GUID IID_IClassFactory = {0x00000001, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

/*<DirectPlay GUIDs*/
GUID CLSID_DirectPlay = {0xd1eb6d20, 0x8923, 0x11d0, {0x9d, 0x97, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb}};

GUID IID_IDirectPlay2 = {0x2b74f7c0, 0x9154, 0x11cf, {0xa9, 0xcd, 0x0, 0xaa, 0x0, 0x68, 0x86, 0xe3}};
GUID IID_IDirectPlay2A = {0x9d460580, 0xa822, 0x11cf, {0x96, 0xc, 0x0, 0x80, 0xc7, 0x53, 0x4e, 0x82}};

GUID IID_IDirectPlay3 = {0x133efe40, 0x32dc, 0x11d0, {0x9c, 0xfb, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb}};
GUID IID_IDirectPlay3A = {0x133efe41, 0x32dc, 0x11d0, {0x9c, 0xfb, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb}};

GUID IID_IDirectPlay4 = {0xab1c530, 0x4745, 0x11d1, {0xa7, 0xa1, 0x0, 0x0, 0xf8, 0x3, 0xab, 0xfc}};
GUID IID_IDirectPlay4A = {0xab1c531, 0x4745, 0x11d1, {0xa7, 0xa1, 0x0, 0x0, 0xf8, 0x3, 0xab, 0xfc}};

GUID CLSID_DPLobby = {0x2FE8F810, 0xB2A5, 0x11D0, {0xA7, 0x87, 0x0, 0x0, 0xF8, 0x3, 0xAB, 0xFC}};

GUID IID_IDirectPlayLobby = {0xaf465c71, 0x9588, 0x11cf, {0xa0, 0x20, 0x0, 0xaa, 0x0, 0x61, 0x57, 0xac}};
GUID IID_IDirectPlayLobbyA = {0x26c66a70, 0xb367, 0x11cf, {0xa0, 0x24, 0x0, 0xaa, 0x0, 0x61, 0x57, 0xac}};

GUID IID_IDirectPlayLobby2 = {0x194c220, 0xa303, 0x11d0, {0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e}};
GUID IID_IDirectPlayLobby2A = {0x1bb4af80, 0xa303, 0x11d0, {0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e}};

GUID IID_IDirectPlayLobby3 = {0x2db72490, 0x652c, 0x11d1, {0xa7, 0xa8, 0x0, 0x0, 0xf8, 0x3, 0xab, 0xfc}};
GUID IID_IDirectPlayLobby3A = {0x2db72491, 0x652c, 0x11d1, {0xa7, 0xa8, 0x0, 0x0, 0xf8, 0x3, 0xab, 0xfc}};

GUID DPSP_TCPIP = {0x36E95EE0, 0x8577, 0x11cf, {0x96, 0xc, 0x0, 0x80, 0xc7, 0x53, 0x4e, 0x82}};
GUID DPSP_Serial = {0xf1d6860, 0x88d9, 0x11cf, {0x9c, 0x4e, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e}};  // {0F1D6860-88D9-11CF-9C4E-00A0C905425E}
/*</DirectPlay GUIDs*/

/*<Data Type GUIDs>*/
GUID DATATYPE_TOTALSIZE = {0x1318F560, 0x912C, 0x11d0, {0x9D, 0xAA, 0x0, 0xA0, 0xC9, 0xA, 0x43, 0xCB}};
GUID DATATYPE_DPSP = {0x07D916C0, 0xE0AF, 0x11CF, {0x9C, 0x4E, 0x0, 0xA0, 0xC9, 0x5, 0x42, 0x5E}};
GUID DATATYPE_IP_CSTRING = {0xC4A54DA0, 0xE0AF, 0x11CF, {0x9C, 0x4E, 0x0, 0xA0, 0xC9, 0x5, 0x42, 0x5E}};
GUID DATATYPE_IP_UNICODE = {0xE63232A0, 0x9DBF, 0x11D0, {0x9C, 0xC1, 0x0, 0xA0, 0xC9, 0x5, 0x42, 0x5E}};
/*</Data Type GUIDs>*/
