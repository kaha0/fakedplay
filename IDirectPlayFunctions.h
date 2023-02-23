#pragma once
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include<windows.h>
#include "fakedplay.h"

void IDirectPlay_Constructor(IDirectPlay4 * This, BOOL bUnicode);

/*** IUnknown methods ***/
HRESULT __stdcall IDirectPlay_QueryInterface(LPUNKNOWN This, IID * piid, LPVOID * ppvObj);
ULONG __stdcall IDirectPlay_AddRef(LPUNKNOWN This) ;
ULONG __stdcall IDirectPlay_Release(LPUNKNOWN This);
/*** IDirectPlay2 methods ***/
HRESULT __stdcall IDirectPlay_AddPlayerToGroup(LPUNKNOWN This, DPID dpid1, DPID dpid2);
HRESULT __stdcall IDirectPlay_Close(LPUNKNOWN This);
HRESULT __stdcall IDirectPlay_CreateGroup(LPUNKNOWN This, LPDPID lpdpid,LPDPNAME lpdpname,LPVOID pv,DWORD dw1,DWORD dw2);
HRESULT __stdcall IDirectPlay_CreatePlayer(LPUNKNOWN This, LPDPID lpdpid,LPDPNAME lpdpname,HANDLE h,LPVOID pv,DWORD dw1,DWORD dw2);
HRESULT __stdcall IDirectPlay_DeletePlayerFromGroup(LPUNKNOWN This, DPID dpid1,DPID dpid2);
HRESULT __stdcall IDirectPlay_DestroyGroup(LPUNKNOWN This, DPID dpid);
HRESULT __stdcall IDirectPlay_DestroyPlayer(LPUNKNOWN This, DPID dpid);
HRESULT __stdcall IDirectPlay_EnumGroupPlayers(LPUNKNOWN This, DPID dpid,LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw);
HRESULT __stdcall IDirectPlay_EnumGroups(LPUNKNOWN This, LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw);
HRESULT __stdcall IDirectPlay_EnumPlayers(LPUNKNOWN This, LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw);
HRESULT __stdcall IDirectPlay_EnumSessions(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw1,LPDPENUMSESSIONSCALLBACK2 lpdpenumsessionscallback,LPVOID pv,DWORD dw2);
HRESULT __stdcall IDirectPlay_GetCaps(LPUNKNOWN This, LPDPCAPS lpdpcaps,DWORD dw);
HRESULT __stdcall IDirectPlay_GetGroupData(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw,DWORD dw);
HRESULT __stdcall IDirectPlay_GetGroupName(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_GetMessageCount(LPUNKNOWN This, DPID dpid, LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_GetPlayerAddress(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_GetPlayerCaps(LPUNKNOWN This, DPID dpid,LPDPCAPS lpdpcaps,DWORD dw);
HRESULT __stdcall IDirectPlay_GetPlayerData(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw,DWORD dw);
HRESULT __stdcall IDirectPlay_GetPlayerName(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_GetSessionDesc(LPUNKNOWN This, LPVOID pv,LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_Initialize(LPUNKNOWN This, LPGUID lpguid);
HRESULT __stdcall IDirectPlay_Open(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw);
HRESULT __stdcall IDirectPlay_Receive(LPUNKNOWN This, LPDPID lpdpid1,LPDPID lpdpid2,DWORD dw,LPVOID pv,LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_Send(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw1, LPVOID pv, DWORD dw2);
HRESULT __stdcall IDirectPlay_SetGroupData(LPUNKNOWN This, DPID dpid,LPVOID pv,DWORD dw1,DWORD dw2);
HRESULT __stdcall IDirectPlay_SetGroupName(LPUNKNOWN This, DPID dpid,LPDPNAME lpdpname,DWORD dw);
HRESULT __stdcall IDirectPlay_SetPlayerData(LPUNKNOWN This, DPID dpid,LPVOID pv,DWORD dw1,DWORD dw2);
HRESULT __stdcall IDirectPlay_SetPlayerName(LPUNKNOWN This, DPID dpid,LPDPNAME lpdpname,DWORD dw);
HRESULT __stdcall IDirectPlay_SetSessionDesc(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw);
/*** IDirectPlay3 methods ***/
HRESULT __stdcall IDirectPlay_AddGroupToGroup(LPUNKNOWN This, DPID dpid1, DPID dpid2);
HRESULT __stdcall IDirectPlay_CreateGroupInGroup(LPUNKNOWN This, DPID dpid,LPDPID lpdpid,LPDPNAME lpdpname,LPVOID pv,DWORD dw1,DWORD dw2);
HRESULT __stdcall IDirectPlay_DeleteGroupFromGroup(LPUNKNOWN This, DPID dpid1,DPID dpid2);
HRESULT __stdcall IDirectPlay_EnumConnections(LPUNKNOWN This, LPCGUID lpguid,LPDPENUMCONNECTIONSCALLBACK lpdpenumconnectionscallback,LPVOID pv,DWORD dw);
HRESULT __stdcall IDirectPlay_EnumGroupsInGroup(LPUNKNOWN This, DPID dpid,LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumconnectionscallback,LPVOID pv,DWORD dw);
HRESULT __stdcall IDirectPlay_GetGroupConnectionSettings(LPUNKNOWN This, DWORD dw, DPID dpid, LPVOID pv, LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_InitializeConnection(LPUNKNOWN This, LPVOID pv,DWORD dw);
HRESULT __stdcall IDirectPlay_SecureOpen(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw,LPDPSECURITYDESC lpdpsecuritydesc,LPDPCREDENTIALS lpdpcredentials);
HRESULT __stdcall IDirectPlay_SendChatMessage(LPUNKNOWN This, DPID dpid1,DPID dpid2,DWORD dw,LPDPCHAT lpdpchat);
HRESULT __stdcall IDirectPlay_SetGroupConnectionSettings(LPUNKNOWN This, DWORD dw,DPID dpid,LPDPLCONNECTION lpdplconnection);
HRESULT __stdcall IDirectPlay_StartSession(LPUNKNOWN This, DWORD dw,DPID dpid);
HRESULT __stdcall IDirectPlay_GetGroupFlags(LPUNKNOWN This, DPID dpid,LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_GetGroupParent(LPUNKNOWN This, DPID dpid1,LPDPID dpid2);
HRESULT __stdcall IDirectPlay_GetPlayerAccount(LPUNKNOWN This, DPID dpid, DWORD dw, LPVOID pv, LPDWORD lpdw);
HRESULT __stdcall IDirectPlay_GetPlayerFlags(LPUNKNOWN This, DPID dpid,LPDWORD lpdw);
/*** IDirectPlay4 methods ***/
HRESULT __stdcall IDirectPlay_GetGroupOwner(LPUNKNOWN This, DPID dpid, LPDPID lpdpid);
HRESULT __stdcall IDirectPlay_SetGroupOwner(LPUNKNOWN This, DPID dpid1, DPID dpid2);
HRESULT __stdcall IDirectPlay_SendEx(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw1, LPVOID pv1, DWORD dw2, DWORD dw3, DWORD dw4, LPVOID pv2, DWORD_PTR * lplpdw);
HRESULT __stdcall IDirectPlay_GetMessageQueue(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw, LPDWORD lpdw1, LPDWORD lpdw2);
HRESULT __stdcall IDirectPlay_CancelMessage(LPUNKNOWN This, DWORD dw1, DWORD dw2);
HRESULT __stdcall IDirectPlay_CancelPriority(LPUNKNOWN This, DWORD dw1, DWORD dw2, DWORD dw3);
