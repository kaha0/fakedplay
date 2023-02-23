#pragma once

//budou messages symetrický? client -> server == server -> client ?

#define FDP_MSG_EnumSessions				1
#define FDP_MSG_JoinSession					2
#define FDP_MSG_CreateSession				3
#define FDP_MSG_CreatePlayer				4
#define FDP_MSG_EnumPlayers					5
#define FDP_MSG_DestroyPlayer				6
#define FDP_MSG_Send						7
#define FDP_MSG_Receive						8
#define FDP_MSG_GetSessionDesc				9
#define FDP_MSG_SetSessionDesc				10
#define FDP_MSG_TerminateSession			0x7F


// potvrzovací message? 0xFF ?
// zamítací message ??
#define FDP_MSG_Yes							0xFF
#define FDP_MSG_No							0