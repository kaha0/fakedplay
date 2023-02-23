#pragma once
#ifndef SOCKET
#define SOCKET UINT_PTR
#endif

// my own definitions of guids, interfaces etc. so I don't have to include entire dplay.h and ole.h

/*<IUnknown>*/
typedef struct IUnknown
{
	const struct IUnknownVtbl FAR *lpVtbl;
} IUnknown;
typedef struct IUnknownVtbl IUnknownVtbl;
struct IUnknownVtbl
{   
	HRESULT (__stdcall *QueryInterface)(IUnknown * This, IID * riid, void **ppvObject);
	ULONG (__stdcall *AddRef)(IUnknown * This);
	ULONG (__stdcall *Release)(IUnknown * This);
};
typedef IUnknown FAR *LPUNKNOWN;
/*</IUnknown>*/

/*<IClassFactory>*/
typedef struct IClassFactory
{
	const struct IClassFactoryVtbl FAR *lpVtbl;
	DWORD dwRefCount;
} IClassFactory;
typedef struct IClassFactoryVtbl IClassFactoryVtbl;
struct IClassFactoryVtbl
{
	HRESULT (__stdcall * QueryInterface) (IClassFactory FAR* This, IID FAR* riid, LPVOID FAR* ppvObj) ;
	ULONG (__stdcall * AddRef) (IClassFactory FAR* This) ;
	ULONG (__stdcall * Release) (IClassFactory FAR* This) ;
	HRESULT (__stdcall * CreateInstance) (IClassFactory FAR* This, LPUNKNOWN pUnkOuter, IID FAR* riid, LPVOID FAR* ppvObject);
	HRESULT (__stdcall * LockServer) (IClassFactory FAR* This, BOOL fLock);
};
typedef IClassFactory FAR *LPCLASSFACTORY;
/*</IClassFactory>*/

/*<DirectPlay errors: dplay.h:1848>*/
//#define _FACDP  0x877
//#define MAKE_DPHRESULT( code )    MAKE_HRESULT( 1, _FACDP, code )		== 0x88770000 | (unsigned short)code
#define DPERR_ACCESSDENIED		0x8877000A
#define DPERR_BUFFERTOOSMALL	0x8877001E
#define DPERR_CANTCREATEPLAYER	0x8877003C
#define DPERR_CANTCREATESESSION	0x88770046
#define DPERR_INVALIDFLAGS		0x88770078
#define DPERR_INVALIDPARAM		E_INVALIDARG
#define DPERR_INVALIDPLAYER		0x88770096
#define DPERR_NOCONNECTION		0x887700AA
#define DPERR_NOMESSAGES		0x887700BE
#define DPERR_UNSUPPORTED		E_NOTIMPL
#define DPERR_USERCANCEL		0x88770118
#define DPERR_PLAYERLOST		0x8877012C
#define DPERR_SESSIONLOST		0x88770136
#define DPERR_UNINITIALIZED		0x88770140
#define DPERR_INVALIDPASSWORD	0x88770154
#define DPERR_CONNECTIONLOST	0x88770168
#define DPERR_CANCELLED			0x8877019A
#define DPERR_ABORTED			0x887701A4
#define DPERR_NOSERVICEPROVIDER	0x88770410
#define DPERR_NOTLOBBIED		0x8877042E
/*</DirectPlay errors>*/

/*<DirectPlay structures, callbacks, enums etc>*/
typedef DWORD DPID, FAR *LPDPID;

typedef struct DPNAME
{
    DWORD   dwSize;             // Size of structure
    DWORD   dwFlags;            // Not used. Must be zero.
    union
    {                           // The short or friendly name
        LPWSTR  lpszShortName;  // Unicode
        LPSTR   lpszShortNameA; // ANSI
    };
    union
    {                           // The long or formal name
        LPWSTR  lpszLongName;   // Unicode
        LPSTR   lpszLongNameA;  // ANSI
    };

} DPNAME, FAR *LPDPNAME;

typedef BOOL (FAR PASCAL * LPDPENUMDPCALLBACK)(
    LPGUID      lpguidSP,
    LPWSTR      lpSPName,
    DWORD       dwMajorVersion,
    DWORD       dwMinorVersion,
    LPVOID      lpContext);

typedef BOOL (FAR PASCAL * LPDPENUMDPCALLBACKA)(
    LPGUID      lpguidSP,
    LPSTR       lpSPName,
    DWORD       dwMajorVersion,     
    DWORD       dwMinorVersion,
    LPVOID      lpContext);

//DPENUMPLAYERSCALLBACK is from IDirectPlay1 - obsolete (dplay.h:1992)
typedef BOOL (FAR PASCAL *LPDPENUMPLAYERSCALLBACK2)(
    DPID            dpId,
    DWORD           dwPlayerType,
    LPDPNAME       lpName,
    DWORD           dwFlags,
    LPVOID          lpContext );

// DPSESSIONDESC is from IDirectPlay1 - obsolete (dplay.h:1999)
typedef struct
{
    DWORD   dwSize;             // Size of structure
    DWORD   dwFlags;            // DPSESSION_xxx flags
    GUID    guidInstance;       // ID for the session instance
    GUID    guidApplication;    // GUID of the DirectPlay application.
                                // GUID_NULL for all applications.
    DWORD   dwMaxPlayers;       // Maximum # players allowed in session
    DWORD   dwCurrentPlayers;   // Current # players in session (read only)
    union
    {                           // Name of the session
        LPWSTR  lpszSessionName;    // Unicode
        LPSTR   lpszSessionNameA;   // ANSI
    };
    union
    {                           // Password of the session (optional)
        LPWSTR  lpszPassword;       // Unicode
        LPSTR   lpszPasswordA;      // ANSI
    };
    DWORD_PTR   dwReserved1;        // Reserved for future MS use.
    DWORD_PTR   dwReserved2;
    DWORD_PTR   dwUser1;            // For use by the application
    DWORD_PTR   dwUser2;
    DWORD_PTR   dwUser3;
    DWORD_PTR   dwUser4;
} DPSESSIONDESC2, FAR *LPDPSESSIONDESC2;

#define DPSESSION_NEWPLAYERSDISABLED	1
#define DPSESSION_JOINDISABLED			0x20
#define DPSESSION_KEEPALIVE				0x40
#define DPSESSION_PASSWORDREQUIRED		0x400
#define DPSESSION_CLIENTSERVER			0x1000
#define DPSESSION_NOSESSIONDESCMESSAGES	0x20000

// DPCAPS - dplay.h:147
typedef struct
{
    DWORD dwSize;               // Size of structure, in bytes
    DWORD dwFlags;              // DPCAPS_xxx flags
    DWORD dwMaxBufferSize;      // Maximum message size, in bytes,  for this service provider
    DWORD dwMaxQueueSize;       // Obsolete. 
    DWORD dwMaxPlayers;         // Maximum players/groups (local + remote)
    DWORD dwHundredBaud;        // Bandwidth in 100 bits per second units; 
                                // i.e. 24 is 2400, 96 is 9600, etc.
    DWORD dwLatency;            // Estimated latency; 0 = unknown
    DWORD dwMaxLocalPlayers;    // Maximum # of locally created players allowed
    DWORD dwHeaderLength;       // Maximum header length, in bytes, on messages
                                // added by the service provider
    DWORD dwTimeout;            // Service provider's suggested timeout value
                                // This is how long DirectPlay will wait for 
                                // responses to system messages
} DPCAPS, FAR *LPDPCAPS;

/* Callback for IDirectPlay3(A)::EnumConnections */
typedef BOOL (FAR PASCAL * LPDPENUMCONNECTIONSCALLBACK)(
    LPCGUID     lpguidSP,
	LPVOID		lpConnection,
	DWORD		dwConnectionSize,
    LPDPNAME   lpName,
	DWORD 		dwFlags,
	LPVOID 		lpContext);

/* Callback for IDirectPlay2::EnumSessions */
typedef BOOL (FAR PASCAL * LPDPENUMSESSIONSCALLBACK2)(
    LPDPSESSIONDESC2   lpThisSD,
    LPDWORD             lpdwTimeOut,
    DWORD               dwFlags,
    LPVOID              lpContext );
// LPDPENUMSESSIONSCALLBACK is from IDirectPlay1 - obsolete (dplay.h:2018)

typedef struct
{
    DWORD               dwSize;
    DWORD               dwFlags;
    union
    {                          // Message string
        LPWSTR  lpszMessage;   // Unicode
        LPSTR   lpszMessageA;  // ANSI
    };    
} DPCHAT, FAR * LPDPCHAT;

// DPLCONNECTION - dplay.h:489
typedef struct
{
    DWORD               dwSize;             // Size of this structure
    DWORD               dwFlags;            // Flags specific to this structure
    LPDPSESSIONDESC2    lpSessionDesc;      // Pointer to session desc to use on connect
    LPDPNAME            lpPlayerName;       // Pointer to Player name structure
    GUID                guidSP;             // GUID of the DPlay SP to use
    LPVOID              lpAddress;          // Address for service provider
    DWORD               dwAddressSize;      // Size of address data
} DPLCONNECTION, FAR *LPDPLCONNECTION;

typedef struct 
{
    DWORD dwSize;                   // Size of structure
    DWORD dwFlags;                  // Not used. Must be zero.
    union
    {                               // SSPI provider name
        LPWSTR  lpszSSPIProvider;   // Unicode
        LPSTR   lpszSSPIProviderA;  // ANSI
    };
    union
    {                               // CAPI provider name
        LPWSTR lpszCAPIProvider;    // Unicode
        LPSTR  lpszCAPIProviderA;   // ANSI
    };
    DWORD dwCAPIProviderType;       // Crypto Service Provider type
    DWORD dwEncryptionAlgorithm;    // Encryption Algorithm type
} DPSECURITYDESC, FAR *LPDPSECURITYDESC;

typedef struct 
{
    DWORD dwSize;               // Size of structure
    DWORD dwFlags;              // Not used. Must be zero.
    union
    {                           // User name of the account
        LPWSTR  lpszUsername;   // Unicode
        LPSTR   lpszUsernameA;  // ANSI
    };    
    union
    {                           // Password of the account
        LPWSTR  lpszPassword;   // Unicode
        LPSTR   lpszPasswordA;  // ANSI
    };    
    union
    {                           // Domain name of the account
        LPWSTR  lpszDomain;     // Unicode
        LPSTR   lpszDomainA;    // ANSI
    };    
} DPCREDENTIALS, FAR *LPDPCREDENTIALS;

typedef struct IDirectPlayLobby     FAR *LPDIRECTPLAYLOBBY;
typedef struct IDirectPlayLobby     FAR *LPDIRECTPLAYLOBBYA;

/*<DPSYSMSGs>*/  /*pøidat i DPSYS_xxx types ? (dplay.h:1509)*/
#define DPSYS_CREATEPLAYERORGROUP	3
#define DPSYS_DESTROYPLAYERORGROUP	5
#define DPSYS_SETSESSIONDESC		0x0104
#define DPSYS_SESSIONLOST			0x31
#define DPPLAYERTYPE_GROUP			0
#define DPPLAYERTYPE_PLAYER			1
typedef struct
{
    DWORD       dwType;         // Message type
    DWORD       dwPlayerType;   // Is it a player or group
    DPID        dpId;           // ID of the player or group
    DWORD       dwCurrentPlayers;   // current # players & groups in session
    LPVOID      lpData;         // pointer to remote data
    DWORD       dwDataSize;     // size of remote data
    DPNAME      dpnName;        // structure with name info
	// the following fields are only available when using
	// the IDirectPlay3 interface or greater
    DPID	    dpIdParent;     // id of parent group
	DWORD		dwFlags;		// player or group flags
} DPMSG_CREATEPLAYERORGROUP, FAR *LPDPMSG_CREATEPLAYERORGROUP;
typedef struct
{
    DWORD       dwType;         // Message type
    DWORD       dwPlayerType;   // Is it a player or group
    DPID        dpId;           // player ID being deleted
    LPVOID      lpLocalData;    // copy of players local data
    DWORD       dwLocalDataSize; // sizeof local data
    LPVOID      lpRemoteData;   // copy of players remote data
    DWORD       dwRemoteDataSize; // sizeof remote data
	// the following fields are only available when using
	// the IDirectPlay3 interface or greater
    DPNAME      dpnName;        // structure with name info
    DPID	    dpIdParent;     // id of parent group	
	DWORD		dwFlags;		// player or group flags
} DPMSG_DESTROYPLAYERORGROUP, FAR *LPDPMSG_DESTROYPLAYERORGROUP;
typedef struct
{
    DWORD           dwType;     // Message type
    DPSESSIONDESC2  dpDesc;     // Session desc
} DPMSG_SETSESSIONDESC, FAR *LPDPMSG_SETSESSIONDESC;
/*</DPSYSMSGs>*/

/*</structures, callbacks, enums etc>*/

// FDP_MSG types
// budou messages symetrický? client -> server == server -> client ?
// jde to ale neni to moc hezký... hezèí by bylo mít FDP_MSG_EnumPlayers a FDP_MSG_EnumPlayers_Response
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

typedef struct FDP_MSG_Generic
{
	char type;
	unsigned short length;
} FDP_MSG_Generic;

// FDP_STATE
#define FDP_STATE_UNINITIALIZED			0
#define FDP_STATE_INITIALIZED			1
#define FDP_STATE_IN_SESSION			2
#define FDP_STATE_IN_SESSION_HAS_PLAYER	3
#define FDP_STATE_SESSION_LOST			4
#define FDP_STATE_CLOSED				0xFF

/*<IDirectPlay interfaces themselves>*/

// IDirectPlay : dplay.h:2025
typedef struct IUnknown FAR *LPDIRECTPLAY;  // old, obsolete, incompatible, not going to implement

// IDirectPlay2 : dplay.h:623	- is a compatible subset of IDirectPlay4

// IDirectPlay3 : dplay.h:747	- is a compatible subset of IDirectPlay4

typedef struct dpPlayerInfo  // necessary only if we have players cached locally for IDirectPlay4::EnumPlayers
{
	DPID dpid;
	char* name;
} dpPlayerInfo;

/*<IDirectPlay4>  dplay.h:917*/
typedef struct IDirectPlay4
{
	const struct IDirectPlay4Vtbl FAR*lpVtbl;
	DWORD dwRefCount;
	dword State;
	BOOL UseUnicode;
	SOCKET Socket;
	DPID MyDPID;				// unnecessary?
	BOOL AmServer;
	SOCKET ServerSocket;
	BOOL ReceivedSysMsgSessionLost;  // unnecessary now that we have ::State ?
	char* DataMsgQueueBeginning;  // buffer for incoming Send messages, passed to application
	char* DataMsgQueue;  // == DataMsgQueueCurrent
	char* DataMsgQueueEnd;
	DWORD DataMsgQueueLength;
	CRITICAL_SECTION DataMsgQueueLock;  // as of now used for both IDP_Receive and IDP_Send. that's wrong (do we even need to lock for sends?)
	char* SysMsgQueueBeginning;  // buffer for FDP system messages (responses to transaction-based operations)
	char* SysMsgQueue;  // == SysMsgQueueCurrent
	char* SysMsgQueueEnd;
	DWORD SysMsgQueueLength;
	CRITICAL_SECTION SysMsgQueueLock;
	DPSESSIONDESC2 SessionDesc;
	dpPlayerInfo* Players;  // unnecessary
	DWORD PlayersCapacity;
} IDirectPlay4;
typedef struct IDirectPlay4Vtbl IDirectPlay4Vtbl;
struct IDirectPlay4Vtbl
{   
	/*** IUnknown methods ***/
	HRESULT (__stdcall * QueryInterface)	(LPUNKNOWN This, IID * piid, LPVOID * ppvObj);
	ULONG (__stdcall * AddRef)				(LPUNKNOWN This) ;
	ULONG (__stdcall * Release)				(LPUNKNOWN This);
	/*** IDirectPlay2 methods ***/
	HRESULT (__stdcall * AddPlayerToGroup)	(LPUNKNOWN This, DPID dpid1, DPID dpid2);
	HRESULT (__stdcall * Close)				(LPUNKNOWN This);
	HRESULT (__stdcall * CreateGroup)		(LPUNKNOWN This, LPDPID lpdpid,LPDPNAME lpdpname,LPVOID pv,DWORD dw1,DWORD dw2);
	HRESULT (__stdcall * CreatePlayer)		(LPUNKNOWN This, LPDPID lpdpid,LPDPNAME lpdpname,HANDLE h,LPVOID pv,DWORD dw1,DWORD dw2);
	HRESULT (__stdcall * DeletePlayerFromGroup)(LPUNKNOWN This, DPID dpid1,DPID dpid2);
	HRESULT (__stdcall * DestroyGroup)		(LPUNKNOWN This, DPID dpid);
	HRESULT (__stdcall * DestroyPlayer)		(LPUNKNOWN This, DPID dpid);
	HRESULT (__stdcall * EnumGroupPlayers)	(LPUNKNOWN This, DPID dpid,LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw);
	HRESULT (__stdcall * EnumGroups)		(LPUNKNOWN This, LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw);
	HRESULT (__stdcall * EnumPlayers)		(LPUNKNOWN This, LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw);
	HRESULT (__stdcall * EnumSessions)		(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw1,LPDPENUMSESSIONSCALLBACK2 lpdpenumplayerscallback,LPVOID pv,DWORD dw2);
	HRESULT (__stdcall * GetCaps)			(LPUNKNOWN This, LPDPCAPS lpdpcaps,DWORD dw);
	HRESULT (__stdcall * GetGroupData)		(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw,DWORD dw);
	HRESULT (__stdcall * GetGroupName)		(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw);
	HRESULT (__stdcall * GetMessageCount)	(LPUNKNOWN This, DPID dpid, LPDWORD lpdw);
	HRESULT (__stdcall * GetPlayerAddress)	(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw);
	HRESULT (__stdcall * GetPlayerCaps)		(LPUNKNOWN This, DPID dpid,LPDPCAPS lpdpcaps,DWORD dw);
	HRESULT (__stdcall * GetPlayerData)		(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw,DWORD dw);
	HRESULT (__stdcall * GetPlayerName)		(LPUNKNOWN This, DPID dpid,LPVOID pv,LPDWORD lpdw);
	HRESULT (__stdcall * GetSessionDesc)	(LPUNKNOWN This, LPVOID pv,LPDWORD lpdw);
	HRESULT (__stdcall * Initialize)		(LPUNKNOWN This, LPGUID lpguid);
	HRESULT (__stdcall * Open)				(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw);
	HRESULT (__stdcall * Receive)			(LPUNKNOWN This, LPDPID lpdpid1,LPDPID lpdpid2,DWORD dw,LPVOID pv,LPDWORD lpdw);
	HRESULT (__stdcall * Send)				(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw1, LPVOID pv, DWORD dw2);
	HRESULT (__stdcall * SetGroupData)		(LPUNKNOWN This, DPID dpid,LPVOID pv,DWORD dw1,DWORD dw2);
	HRESULT (__stdcall * SetGroupName)		(LPUNKNOWN This, DPID dpid,LPDPNAME lpdpname,DWORD dw);
	HRESULT (__stdcall * SetPlayerData)		(LPUNKNOWN This, DPID dpid,LPVOID pv,DWORD dw1,DWORD dw2);
	HRESULT (__stdcall * SetPlayerName)		(LPUNKNOWN This, DPID dpid,LPDPNAME lpdpname,DWORD dw);
	HRESULT (__stdcall * SetSessionDesc)	(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw);
	/*** IDirectPlay3 methods ***/
	HRESULT (__stdcall * AddGroupToGroup)	(LPUNKNOWN This, DPID dpid1, DPID dpid2);
	HRESULT (__stdcall * CreateGroupInGroup)(LPUNKNOWN This, DPID dpid,LPDPID lpdpid,LPDPNAME lpdpname,LPVOID pv,DWORD dw1,DWORD dw2);
	HRESULT (__stdcall * DeleteGroupFromGroup)(LPUNKNOWN This, DPID dpid1,DPID dpid2);	
	HRESULT (__stdcall * EnumConnections)	(LPUNKNOWN This, LPCGUID lpguid,LPDPENUMCONNECTIONSCALLBACK lpdpenumconnectionscallback,LPVOID pv,DWORD dw);
	HRESULT (__stdcall * EnumGroupsInGroup)	(LPUNKNOWN This, DPID dpid,LPGUID lpguid,LPDPENUMPLAYERSCALLBACK2 lpdpenumconnectionscallback,LPVOID pv,DWORD dw);
	HRESULT (__stdcall * GetGroupConnectionSettings)(LPUNKNOWN This, DWORD dw, DPID dpid, LPVOID pv, LPDWORD lpdw);
	HRESULT (__stdcall * InitializeConnection)(LPUNKNOWN This, LPVOID pv,DWORD dw);
	HRESULT (__stdcall * SecureOpen)		(LPUNKNOWN This, LPDPSESSIONDESC2 lpdpsessiondesc,DWORD dw,LPDPSECURITYDESC lpdpsecuritydesc,LPDPCREDENTIALS lpdpcredentials);
	HRESULT (__stdcall * SendChatMessage)	(LPUNKNOWN This, DPID dpid1,DPID dpid2,DWORD dw,LPDPCHAT lpdpchat);
	HRESULT (__stdcall * SetGroupConnectionSettings)(LPUNKNOWN This, DWORD dw,DPID dpid,LPDPLCONNECTION lpdplconnection);
	HRESULT (__stdcall * StartSession)		(LPUNKNOWN This, DWORD dw,DPID dpid);
	HRESULT (__stdcall * GetGroupFlags)		(LPUNKNOWN This, DPID dpid,LPDWORD lpdw);
	HRESULT (__stdcall * GetGroupParent)	(LPUNKNOWN This, DPID dpid1,LPDPID dpid2);
	HRESULT (__stdcall * GetPlayerAccount)	(LPUNKNOWN This, DPID dpid, DWORD dw, LPVOID pv, LPDWORD lpdw);
	HRESULT (__stdcall * GetPlayerFlags)	(LPUNKNOWN This, DPID dpid,LPDWORD lpdw);
	/*** IDirectPlay4 methods ***/
	HRESULT (__stdcall * GetGroupOwner)		(LPUNKNOWN This, DPID dpid, LPDPID lpdpid);
	HRESULT (__stdcall * SetGroupOwner)		(LPUNKNOWN This, DPID dpid1, DPID dpid2);
	HRESULT (__stdcall * SendEx)			(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw1, LPVOID pv1, DWORD dw2, DWORD dw3, DWORD dw4, LPVOID pv2, DWORD_PTR * lplpdw);
	HRESULT (__stdcall * GetMessageQueue)	(LPUNKNOWN This, DPID dpid1, DPID dpid2, DWORD dw, LPDWORD lpdw1, LPDWORD lpdw2);
	HRESULT (__stdcall * CancelMessage)		(LPUNKNOWN This, DWORD dw1, DWORD dw2);
	HRESULT (__stdcall * CancelPriority)	(LPUNKNOWN This, DWORD dw1, DWORD dw2, DWORD dw3);
};
typedef IDirectPlay4 FAR *LPDIRECTPLAY2;
typedef IDirectPlay4 FAR *LPDIRECTPLAY2A;
typedef IDirectPlay4 FAR *LPDIRECTPLAY3;
typedef IDirectPlay4 FAR *LPDIRECTPLAY3A;
typedef IDirectPlay4 FAR *LPDIRECTPLAY4;
typedef IDirectPlay4 FAR *LPDIRECTPLAY4A;
/*</IDirectPlay4>*/

/*</IDirectPlay interfaces themselves>*/

/*<IDirectPlayLobby structures, callbacks etc>*/
typedef struct DPLAPPINFO  // Used to hold information about a registered DirectPlay application
{
    DWORD       dwSize;             // Size of this structure
    GUID        guidApplication;    // GUID of the Application
    union
    {
        LPSTR   lpszAppNameA;       // Pointer to the Application Name
        LPWSTR  lpszAppName;
    };

} DPLAPPINFO, FAR *LPDPLAPPINFO;
typedef const DPLAPPINFO FAR *LPCDPLAPPINFO;

typedef struct DPCOMPOUNDADDRESSELEMENT  // An array of these is passed to CreateCompoundAddresses()
{
    GUID                guidDataType;
    DWORD               dwDataSize;
	LPVOID				lpData;
} DPCOMPOUNDADDRESSELEMENT, FAR *LPDPCOMPOUNDADDRESSELEMENT;
typedef const DPCOMPOUNDADDRESSELEMENT FAR *LPCDPCOMPOUNDADDRESSELEMENT;

typedef struct DPAPPLICATIONDESC  // Used to register a DirectPlay application
{
    DWORD       dwSize;
    DWORD       dwFlags;
    union
    {
        LPSTR       lpszApplicationNameA;
        LPWSTR      lpszApplicationName;
    };
    GUID        guidApplication;
    union
    {
        LPSTR       lpszFilenameA;
        LPWSTR      lpszFilename;
    };
    union
    {
        LPSTR       lpszCommandLineA;
        LPWSTR      lpszCommandLine;
    };
    union
    {
        LPSTR       lpszPathA;
        LPWSTR      lpszPath;
    };
    union
    {
        LPSTR       lpszCurrentDirectoryA;
        LPWSTR      lpszCurrentDirectory;
    };
    LPSTR       lpszDescriptionA;
    LPWSTR      lpszDescriptionW;
} DPAPPLICATIONDESC, *LPDPAPPLICATIONDESC;

typedef struct DPAPPLICATIONDESC2  // Used to register a DirectPlay application
{
    DWORD       dwSize;
    DWORD       dwFlags;
    union
    {
        LPSTR       lpszApplicationNameA;
        LPWSTR      lpszApplicationName;
    };
    GUID        guidApplication;
    union
    {
        LPSTR       lpszFilenameA;
        LPWSTR      lpszFilename;
    };
    union
    {
        LPSTR       lpszCommandLineA;
        LPWSTR      lpszCommandLine;
    };
    union
    {
        LPSTR       lpszPathA;
        LPWSTR      lpszPath;
    };
    union
    {
        LPSTR       lpszCurrentDirectoryA;
        LPWSTR      lpszCurrentDirectory;
    };
    LPSTR       lpszDescriptionA;
    LPWSTR      lpszDescriptionW;
    union
    {
    	LPSTR		lpszAppLauncherNameA;
    	LPWSTR      lpszAppLauncherName;
    };
} DPAPPLICATIONDESC2, *LPDPAPPLICATIONDESC2;
typedef BOOL (FAR PASCAL *LPDPENUMADDRESSCALLBACK)(GUID* guidDataType, DWORD dwDataSize, LPCVOID lpData, LPVOID lpContext);
typedef BOOL (FAR PASCAL *LPDPLENUMADDRESSTYPESCALLBACK)(GUID* guidDataType, LPVOID lpContext, DWORD dwFlags);
typedef BOOL (FAR PASCAL * LPDPLENUMLOCALAPPLICATIONSCALLBACK)(LPCDPLAPPINFO lpAppInfo, LPVOID lpContext, DWORD dwFlags);
/*</IDirectPlayLobby structures, callbacks etc>*/

/*<IDirectPlayLobby interfaces>*/

// IDirectPlayLobby : dplobby.h:233 - compatible subset of IDirectPlayLobby3

// IDirectPlayLobby2 : dplobby.h:262 - compatible subset of IDirectPlayLobby3

/*<IDirectPlayLobby3>  dplobby.h:293*/
typedef struct IDirectPlayLobby3
{
	const struct IDirectPlayLobby3Vtbl FAR*lpVtbl;
	DWORD dwRefCount;
	//BOOL UseUnicode; ? unnecessary
} IDirectPlayLobby3;
typedef struct IDirectPlayLobby3Vtbl IDirectPlayLobby3Vtbl;
struct IDirectPlayLobby3Vtbl
{
	/*  IUnknown Methods	*/
	HRESULT (__stdcall * QueryInterface)		(LPUNKNOWN This, IID* piid, LPVOID * ppvObj);
	ULONG (__stdcall * AddRef)					(LPUNKNOWN This);
	ULONG (__stdcall * Release)					(LPUNKNOWN This);
	/*  IDirectPlayLobby Methods	*/
	HRESULT (__stdcall * Connect)				(LPUNKNOWN This, DWORD, LPDIRECTPLAY2 *, IUnknown FAR *);
	HRESULT (__stdcall * CreateAddress)			(LPUNKNOWN This, GUID*, GUID*, LPVOID, DWORD, LPVOID, LPDWORD);
	HRESULT (__stdcall * EnumAddress)			(LPUNKNOWN This, LPDPENUMADDRESSCALLBACK, LPVOID, DWORD, LPVOID);
	HRESULT (__stdcall * EnumAddressTypes)		(LPUNKNOWN This, LPDPLENUMADDRESSTYPESCALLBACK, GUID*, LPVOID, DWORD);
	HRESULT (__stdcall * EnumLocalApplications)	(LPUNKNOWN This, LPDPLENUMLOCALAPPLICATIONSCALLBACK, LPVOID, DWORD);
	HRESULT (__stdcall * GetConnectionSettings)	(LPUNKNOWN This, DWORD, LPVOID, LPDWORD);
	HRESULT (__stdcall * ReceiveLobbyMessage)	(LPUNKNOWN This, DWORD, DWORD, LPDWORD, LPVOID, LPDWORD);
	HRESULT (__stdcall * RunApplication)		(LPUNKNOWN This, DWORD, LPDWORD, LPDPLCONNECTION, HANDLE);
	HRESULT (__stdcall * SendLobbyMessage)		(LPUNKNOWN This, DWORD, DWORD, LPVOID, DWORD);
	HRESULT (__stdcall * SetConnectionSettings)	(LPUNKNOWN This, DWORD, DWORD, LPDPLCONNECTION);
	HRESULT (__stdcall * SetLobbyMessageEvent)	(LPUNKNOWN This, DWORD, DWORD, HANDLE);
	/*  IDirectPlayLobby2 Methods	*/
	HRESULT (__stdcall * CreateCompoundAddress)	(LPUNKNOWN This, LPDPCOMPOUNDADDRESSELEMENT,DWORD,LPVOID,LPDWORD);
	/*  IDirectPlayLobby3 Methods	*/
	HRESULT (__stdcall * ConnectEx)				(LPUNKNOWN This, DWORD, IID* riid, LPVOID *, IUnknown FAR *);
	HRESULT (__stdcall * RegisterApplication)	(LPUNKNOWN This, DWORD, LPVOID);
	HRESULT (__stdcall * UnregisterApplication)	(LPUNKNOWN This, DWORD, GUID*);
	HRESULT (__stdcall * WaitForConnectionSettings)(LPUNKNOWN This, DWORD);
};
/*</IDirectPlayLobby3>*/

/*</IDirectPlayLobby interfaces>*/