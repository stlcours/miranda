/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "commonheaders.h"
#include "clc.h"

#define TOOLTIP_TOLERANCE 5

extern HIMAGELIST hCListImages;
extern BOOL(WINAPI * MySetProcessWorkingSetSize) (HANDLE, SIZE_T, SIZE_T);

int GetAverageMode();

static UINT WM_TASKBARCREATED;
static BOOL mToolTipTrayTips = FALSE;
static int cycleTimerId = 0, cycleStep = 0;
static int RefreshTimerId=0;   /////by FYR
static CRITICAL_SECTION trayLockCS;

// don't move to win2k.h, need new and old versions to work on 9x/2000/XP
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010

#define lock cli.pfnLockTray()
#define ulock cli.pfnUnlockTray()

#define initcheck if(!fTrayInited) return

static BOOL fTrayInited=FALSE;

static TCHAR* sttGetXStatus( const char* szProto )
{
	TCHAR* result = NULL;

	if ( CallProtoService( szProto, PS_GETSTATUS, 0, 0 ) > ID_STATUS_OFFLINE ) {
		char str[MAXMODULELABELLENGTH];
		mir_snprintf( str, sizeof(str), "%s/GetXStatus", szProto );
		if ( ServiceExists( str )) {
			char* dbTitle = "XStatusName";
			char* dbTitle2 = NULL;
			int xstatus = CallProtoService( szProto, "/GetXStatus", ( WPARAM )&dbTitle, ( LPARAM )&dbTitle2 );
			if ( dbTitle && xstatus ) {
				DBVARIANT dbv={0};
				if ( !DBGetContactSettingTString(NULL, szProto, dbTitle, &dbv )) {
					if ( dbv.ptszVal[0] != 0 )
						result = mir_tstrdup(dbv.ptszVal);
					DBFreeVariant(&dbv);
	}	}	}	}

	return result;
}

TCHAR* fnTrayIconMakeTooltip( const TCHAR *szPrefix, const char *szProto )
{
	char szProtoName[32];
	TCHAR *szStatus, *szSeparator, *sztProto;
	TCHAR *ProtoXStatus=NULL;
	int t,cn;
	initcheck NULL;
	lock;
	if ( !mToolTipTrayTips )
		szSeparator = (IsWinVerMEPlus()) ? szSeparator = _T("\n") : _T(" | ");
	else
		szSeparator = _T("\n");

	if (szProto == NULL) {
		PROTOCOLDESCRIPTOR **protos;
		int count, netProtoCount, i;
		CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
		for (i = 0,netProtoCount = 0; i < count; i++) {
			if (protos[i]->type == PROTOTYPE_PROTOCOL)
				netProtoCount++;
		}
		if (netProtoCount == 1) {
			for (i = 0; i < count; i++) {
				if ( protos[i]->type == PROTOTYPE_PROTOCOL )
				{ ulock; return cli.pfnTrayIconMakeTooltip(szPrefix, protos[i]->szName); }
		}	}

		if (szPrefix && szPrefix[0]) {
			lstrcpyn(cli.szTip, szPrefix, MAX_TIP_SIZE);
			if (!DBGetContactSettingByte(NULL, "CList", "AlwaysStatus", SETTING_ALWAYSSTATUS_DEFAULT))
			{ ulock; return cli.szTip; }
		} 
		else cli.szTip[0] = '\0';
		cli.szTip[ MAX_TIP_SIZE-1 ] = '\0';

		t=0;
		cn=DBGetContactSettingDword(NULL,"Protocols","ProtoCount",-1);
		if ( cn == -1 )
		{ ulock; return NULL; }

		for ( t=0; t < cn; t++ ) {
			i = cli.pfnGetProtoIndexByPos(protos, count,t);
			if ( i == -1 )
			{ ulock; return _T("???"); }

			if ( protos[i]->type == PROTOTYPE_PROTOCOL && cli.pfnGetProtocolVisibility( protos[i]->szName ))
				ProtoXStatus = sttGetXStatus( protos[i]->szName );

			CallProtoService( protos[i]->szName, PS_GETNAME, sizeof(szProtoName), (LPARAM) szProtoName );
			szStatus = cli.pfnGetStatusModeDescription( CallProtoService( protos[i]->szName, PS_GETSTATUS, 0, 0), 0);
			if ( szStatus ) {
				if ( mToolTipTrayTips ) {
					TCHAR tipline[256];
					#if defined( _UNICODE )
						mir_sntprintf(tipline, SIZEOF(tipline), _T("<b>%-12.12S</b>\t%s"), szProtoName, szStatus);
					#else
						mir_sntprintf(tipline, SIZEOF(tipline), _T("<b>%-12.12s</b>\t%s"), szProtoName, szStatus);
					#endif
					if ( cli.szTip[0] )
						_tcsncat(cli.szTip, szSeparator, MAX_TIP_SIZE - _tcslen(cli.szTip));
					_tcsncat(cli.szTip, tipline, MAX_TIP_SIZE - _tcslen(cli.szTip));
					if (ProtoXStatus) {
						mir_sntprintf(tipline, SIZEOF(tipline), _T("%-24.24s\n"), ProtoXStatus);
						if ( cli.szTip[0] )
							_tcsncat(cli.szTip, szSeparator, MAX_TIP_SIZE - _tcslen(cli.szTip));
						_tcsncat(cli.szTip, tipline, MAX_TIP_SIZE - _tcslen(cli.szTip));
						mir_free( ProtoXStatus );
					}
				}
				else {
					if (cli.szTip[0])
						_tcsncat(cli.szTip, szSeparator, MAX_TIP_SIZE - _tcslen(cli.szTip));
					#if defined( _UNICODE )
					{
						TCHAR* p = a2u( szProtoName );
						_tcsncat(cli.szTip, p, MAX_TIP_SIZE - _tcslen(cli.szTip));
						mir_free( p );
					}
					#else
						_tcsncat(cli.szTip, szProtoName, MAX_TIP_SIZE - _tcslen(cli.szTip));
					#endif
					_tcsncat(cli.szTip, _T(" "), MAX_TIP_SIZE - _tcslen(cli.szTip));
					_tcsncat(cli.szTip, szStatus, MAX_TIP_SIZE - _tcslen(cli.szTip));
	}	}	}	}
	else {
		CallProtoService(szProto, PS_GETNAME, sizeof(szProtoName), (LPARAM) szProtoName);
		#if defined( _UNICODE )
			sztProto = a2u( szProtoName );
		#else
			sztProto = szProtoName;
		#endif
		ProtoXStatus = sttGetXStatus( szProto );
		szStatus = cli.pfnGetStatusModeDescription(CallProtoService(szProto, PS_GETSTATUS, 0, 0), 0);
		if ( szPrefix && szPrefix[0] ) {
			if ( DBGetContactSettingByte( NULL, "CList", "AlwaysStatus", SETTING_ALWAYSSTATUS_DEFAULT )) {
				if ( mToolTipTrayTips ) {
					if ( ProtoXStatus )
						mir_sntprintf(cli.szTip, MAX_TIP_SIZE, _T("%s%s<b>%-12.12s</b>\t%s%s%-24.24s"), szPrefix, szSeparator, sztProto, szStatus,szSeparator,ProtoXStatus);
					else
						mir_sntprintf(cli.szTip, MAX_TIP_SIZE, _T("%s%s<b>%-12.12s</b>\t%s"), szPrefix, szSeparator, sztProto, szStatus);
				}
				else mir_sntprintf(cli.szTip, MAX_TIP_SIZE, _T("%s%s%s %s"), szPrefix, szSeparator, sztProto, szStatus);
			}
			else lstrcpyn(cli.szTip, szPrefix, MAX_TIP_SIZE);
		}
		else {
			if ( mToolTipTrayTips ) {
				if ( ProtoXStatus )
					mir_sntprintf( cli.szTip, MAX_TIP_SIZE, _T("<b>%-12.12s</b>\t%s\n%-24.24s"), sztProto, szStatus,ProtoXStatus);
				else
					mir_sntprintf( cli.szTip, MAX_TIP_SIZE, _T("<b>%-12.12s</b>\t%s"), sztProto, szStatus);
			}
			else mir_sntprintf(cli.szTip, MAX_TIP_SIZE, _T("%s %s"), sztProto, szStatus);
		}
		if (ProtoXStatus) mir_free(ProtoXStatus);
		#if defined( _UNICODE )
			mir_free(sztProto);
		#endif
	}
	{ ulock;  return cli.szTip; }
}

int fnTrayIconAdd(HWND hwnd, const char *szProto, const char *szIconProto, int status)
{
	NOTIFYICONDATA nid = { 0 };
	int i;
	initcheck 0;
	lock;
	for (i = 0; i < cli.trayIconCount; i++)
		if (cli.trayIcon[i].id == 0)
			break;

	cli.trayIcon[i].id = TRAYICON_ID_BASE + i;
	cli.trayIcon[i].szProto = (char *) szProto;
	cli.trayIcon[i].hBaseIcon = cli.pfnGetIconFromStatusMode( NULL, szIconProto ? szIconProto : cli.trayIcon[i].szProto, status );

	nid.cbSize = ( cli.shellVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = hwnd;
	nid.uID = cli.trayIcon[i].id;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = TIM_CALLBACK;
	nid.hIcon = cli.trayIcon[i].hBaseIcon;

	if (cli.shellVersion >= 5)
		nid.uFlags |= NIF_INFO;

	cli.pfnTrayIconMakeTooltip( NULL, cli.trayIcon[i].szProto );
	if ( !mToolTipTrayTips )
		lstrcpyn( nid.szTip, cli.szTip, SIZEOF( nid.szTip ));
	cli.trayIcon[i].ptszToolTip = mir_tstrdup( cli.szTip );

	Shell_NotifyIcon(NIM_ADD, &nid);
	cli.trayIcon[i].isBase = 1;
	{ ulock; return i; }
}

void fnTrayIconRemove(HWND hwnd, const char *szProto)
{
	int i;
	initcheck;
	lock;
	for ( i = 0; i < cli.trayIconCount; i++ ) {
		struct trayIconInfo_t* pii = &cli.trayIcon[i];
		if ( pii->id != 0 && !lstrcmpA( szProto, pii->szProto )) {
			NOTIFYICONDATA nid = { 0 };
			nid.cbSize = ( cli.shellVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
			nid.hWnd = hwnd;
			nid.uID = pii->id;
			Shell_NotifyIcon(NIM_DELETE, &nid);

			DestroyIcon(pii->hBaseIcon);
			mir_free(pii->ptszToolTip); pii->ptszToolTip = NULL;
			pii->id = 0;
			break;
	}	}	
	ulock;
}

int fnTrayIconInit(HWND hwnd)
{
	int count;
	int averageMode = GetAverageMode();
	PROTOCOLDESCRIPTOR **protos;
	initcheck 0;
	lock;
	mToolTipTrayTips = ServiceExists("mToolTip/ShowTip") ? TRUE : FALSE;

	if ( cli.cycleTimerId ) {
		KillTimer(NULL, cli.cycleTimerId);
		cli.cycleTimerId = 0;
	}
	CallService(MS_PROTO_ENUMPROTOCOLS, ( WPARAM )&count, ( LPARAM )&protos);
	
	if (DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI) {
		int netProtoCount, i;
		for (i = 0, netProtoCount = 0; i < count; i++)
			if (protos[i]->type == PROTOTYPE_PROTOCOL && cli.pfnGetProtocolVisibility( protos[i]->szName ))
				netProtoCount++;
		cli.trayIconCount = netProtoCount;
	}
	else cli.trayIconCount = 1;

	cli.trayIcon = (struct trayIconInfo_t *) mir_alloc(sizeof(struct trayIconInfo_t) * count);
	memset(cli.trayIcon, 0, sizeof(struct trayIconInfo_t) * count);
	if ( DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI 
	     && (averageMode <= 0 || DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT ))) {
		int i;
		for (i = count - 1; i >= 0; i--) {
			int j = cli.pfnGetProtoIndexByPos( protos, count, i);
			if ( j > -1 )
				if ( protos[j]->type == PROTOTYPE_PROTOCOL && cli.pfnGetProtocolVisibility( protos[j]->szName ))
					cli.pfnTrayIconAdd(hwnd, protos[j]->szName, NULL, CallProtoService(protos[j]->szName, PS_GETSTATUS, 0, 0));
	}	}
	else if (averageMode <= ID_STATUS_OFFLINE && DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_SINGLE) {
		DBVARIANT dbv = { DBVT_DELETED };
		char *szProto;
		if (!DBGetContactSetting(NULL, "CList", "PrimaryStatus", &dbv) 
			 && (averageMode <= 0 || DBGetContactSettingByte(NULL, "CList", "AlwaysPrimary", 0) ))
			szProto = dbv.pszVal;	
		else
			szProto = NULL;

		cli.pfnTrayIconAdd(hwnd, NULL, szProto, szProto ? CallProtoService(szProto, PS_GETSTATUS, 0, 0) : CallService(MS_CLIST_GETSTATUSMODE, 0, 0));
		DBFreeVariant(&dbv);
	}
	else
		cli.pfnTrayIconAdd(hwnd, NULL, NULL, averageMode);
	if (averageMode <= 0 && DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_CYCLE)
		cli.cycleTimerId = SetTimer(NULL, 0, DBGetContactSettingWord(NULL, "CList", "CycleTime", SETTING_CYCLETIME_DEFAULT) * 1000, cli.pfnTrayCycleTimerProc);
	{ ulock; return 0; }
}

int fnTrayIconDestroy(HWND hwnd)
{
	NOTIFYICONDATA nid = { 0 };
	int i;
	initcheck 0;
	lock;
	nid.cbSize = ( cli.shellVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = hwnd;
	for ( i = 0; i < cli.trayIconCount; i++ ) {
		if ( cli.trayIcon[i].id == 0 )
			continue;
		nid.uID = cli.trayIcon[i].id;
		Shell_NotifyIcon( NIM_DELETE, &nid );
		DestroyIcon( cli.trayIcon[i].hBaseIcon );
		mir_free( cli.trayIcon[i].ptszToolTip );
	}
	mir_free(cli.trayIcon);
	cli.trayIcon = NULL;
	cli.trayIconCount = 0;
	{ ulock; return 0; }
}

//called when Explorer crashes and the taskbar is remade
void fnTrayIconTaskbarCreated(HWND hwnd)
{
	initcheck;
	cli.pfnTrayIconDestroy(hwnd);
	cli.pfnTrayIconInit(hwnd);
}

////////////////////////////////////////////////////////////

static VOID CALLBACK RefreshTimerProc(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	int count,i;
	PROTOCOLDESCRIPTOR **protos;
	if(RefreshTimerId) {KillTimer(NULL,RefreshTimerId); RefreshTimerId=0;}
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	for (i=0; i<count; i++)
		if(protos[i]->type==PROTOTYPE_PROTOCOL &&
			(cli.pfnGetProtocolVisibility(protos[i]->szName)!=0))
			cli.pfnTrayIconUpdateBase(protos[i]->szName);

}
//////// End by FYR /////////

int fnTrayIconUpdate(HICON hNewIcon, const TCHAR *szNewTip, const char *szPreferredProto, int isBase)
{
	NOTIFYICONDATA nid = { 0 };
	int i;

	initcheck -1;
	lock;
	nid.cbSize = ( cli.shellVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = cli.hwndContactList;
	nid.uFlags = NIF_ICON | NIF_TIP;
	nid.hIcon = hNewIcon;
	if (!hNewIcon)
	{ ulock; return -1; }

	for (i = 0; i < cli.trayIconCount; i++) {
		if (cli.trayIcon[i].id == 0)
			continue;
		if (lstrcmpA(cli.trayIcon[i].szProto, szPreferredProto))
			continue;

		nid.uID = cli.trayIcon[i].id;
		cli.pfnTrayIconMakeTooltip(szNewTip, cli.trayIcon[i].szProto);
		mir_free( cli.trayIcon[i].ptszToolTip );
		cli.trayIcon[i].ptszToolTip = mir_tstrdup( cli.szTip );
		if(!mToolTipTrayTips)
			lstrcpyn(nid.szTip, cli.szTip, SIZEOF(nid.szTip));
		Shell_NotifyIcon(NIM_MODIFY, &nid);

		cli.trayIcon[i].isBase = isBase;
		{ ulock; return i; }
	}

	//if there wasn't a suitable icon, change all the icons
	{
		for (i = 0; i < cli.trayIconCount; i++) {
			if (cli.trayIcon[i].id == 0)
				continue;
			nid.uID = cli.trayIcon[i].id;
	
			cli.pfnTrayIconMakeTooltip(szNewTip, cli.trayIcon[i].szProto);
			mir_free( cli.trayIcon[i].ptszToolTip );
			cli.trayIcon[i].ptszToolTip = mir_tstrdup( cli.szTip );
			if(!mToolTipTrayTips)
				lstrcpyn(nid.szTip, cli.szTip, SIZEOF(nid.szTip));
			Shell_NotifyIcon(NIM_MODIFY, &nid);
	
			cli.trayIcon[i].isBase = isBase;
			if (DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI)
			{
				DWORD time1=DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT)*200;
				DWORD time2=DBGetContactSettingWord(NULL,"CList","IconFlashTime",550)+1000;
				DWORD time=max(max(2000,time1),time2);
				if(RefreshTimerId) {KillTimer(NULL,RefreshTimerId); RefreshTimerId=0;}
				RefreshTimerId=SetTimer(NULL,0,time,RefreshTimerProc);	// if unknown base was changed - than show preffered proto icon for 2 sec and reset it to original one after timeout				
			}
			{ ulock; return i; }
		}
	}
	{ ulock; return -1; }
}

int fnTrayIconSetBaseInfo(HICON hIcon, const char *szPreferredProto)
{
	int i;
	initcheck -1;
	lock;
	if (szPreferredProto)
	{
		for (i = 0; i < cli.trayIconCount; i++) {
			if (cli.trayIcon[i].id == 0)
				continue;
			if (lstrcmpA(cli.trayIcon[i].szProto, szPreferredProto))
				continue;
			DestroyIcon(cli.trayIcon[i].hBaseIcon);
			cli.trayIcon[i].hBaseIcon = hIcon;
			{ ulock; return i; }
		}
		if ((cli.pfnGetProtocolVisibility(szPreferredProto)) &&
			(GetAverageMode()==-1) &&
			(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI) &&
			!(DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT)))
		{ ulock; return -1; }
	}
	
	//if there wasn't a specific icon, there will only be one suitable
	for (i = 0; i < cli.trayIconCount; i++) {
		if (cli.trayIcon[i].id == 0)
			continue;
		DestroyIcon(cli.trayIcon[i].hBaseIcon);
		cli.trayIcon[i].hBaseIcon = hIcon;
		{ ulock; return i; }
	}
	DestroyIcon(hIcon);
	{ ulock; return -1; }
}

void fnTrayIconUpdateWithImageList(int iImage, const TCHAR *szNewTip, char *szPreferredProto)
{
	HICON hIcon = ImageList_GetIcon(hCListImages, iImage, ILD_NORMAL);
	int res=cli.pfnTrayIconUpdate(hIcon, szNewTip, szPreferredProto, 0);
	DestroyIcon(hIcon);
}

VOID CALLBACK fnTrayCycleTimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
	int count;
	PROTOCOLDESCRIPTOR **protos;
	initcheck;
	lock;
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) & count, (LPARAM) & protos);
	for (cycleStep++;; cycleStep++) {
		if (cycleStep >= count)
			cycleStep = 0;
		if (protos[cycleStep]->type == PROTOTYPE_PROTOCOL && (cli.pfnGetProtocolVisibility(protos[cycleStep]->szName)) )
			break;
	}
	DestroyIcon(cli.trayIcon[0].hBaseIcon);
	cli.trayIcon[0].hBaseIcon = cli.pfnGetIconFromStatusMode(NULL,protos[cycleStep]->szName,CallProtoService(protos[cycleStep]->szName,PS_GETSTATUS,0,0));
	if (cli.trayIcon[0].isBase)
		cli.pfnTrayIconUpdate(cli.trayIcon[0].hBaseIcon, NULL, NULL, 1);
	ulock;
}

void fnTrayIconUpdateBase(const char *szChangedProto)
{
	int i, count, netProtoCount, changed = -1;
	PROTOCOLDESCRIPTOR **protos;
	int averageMode = 0;
	HWND hwnd = cli.hwndContactList;
	initcheck;
	lock;
	if (cli.cycleTimerId) {
		KillTimer(NULL, cli.cycleTimerId);
		cli.cycleTimerId = 0;
	}
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) & count, (LPARAM) & protos);
	for (i = 0, netProtoCount = 0; i < count; i++) {
		if (protos[i]->type != PROTOTYPE_PROTOCOL)
			continue;
		netProtoCount++;
		if (!lstrcmpA(szChangedProto, protos[i]->szName))
			cycleStep = i;
		if (averageMode == 0)
			averageMode = CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0);
		else if (averageMode != CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0)) {
			averageMode = -1;
			break;
		}
	}
	if (netProtoCount > 1) {
		if (averageMode > 0) {
			if (DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI) {
				if (DBGetContactSettingByte(NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT))
					//changed = cli.pfnTrayIconSetBaseInfo( cli.pfnGetIconFromStatusMode((char*)szChangedProto, NULL, averageMode), (char*)szChangedProto);
					changed = cli.pfnTrayIconSetBaseInfo( cli.pfnGetIconFromStatusMode( NULL, szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0)), (char*)szChangedProto );
				else if (cli.trayIcon && cli.trayIcon[0].szProto != NULL) {
					cli.pfnTrayIconDestroy(hwnd);
					cli.pfnTrayIconInit(hwnd);
				}
				else
					changed = cli.pfnTrayIconSetBaseInfo( cli.pfnGetIconFromStatusMode(NULL, NULL, averageMode), NULL );
			}
			else
				changed = cli.pfnTrayIconSetBaseInfo( cli.pfnGetIconFromStatusMode(NULL, NULL, averageMode), NULL);
		}
		else {
			switch (DBGetContactSettingByte(NULL, "CList", "TrayIcon", SETTING_TRAYICON_DEFAULT)) {
			case SETTING_TRAYICON_SINGLE:
				{
					DBVARIANT dbv = { DBVT_DELETED };
					char *szProto;
					if (DBGetContactSetting(NULL, "CList", "PrimaryStatus", &dbv))
						szProto = NULL;
					else
						szProto = dbv.pszVal;
					changed = cli.pfnTrayIconSetBaseInfo( cli.pfnGetIconFromStatusMode( NULL, szProto, szProto ? CallProtoService(szProto, PS_GETSTATUS, 0,0) : CallService(MS_CLIST_GETSTATUSMODE, 0, 0)), szProto );
					DBFreeVariant(&dbv);
					break;
				}
			case SETTING_TRAYICON_CYCLE:
				cli.cycleTimerId =
					SetTimer(NULL, 0, DBGetContactSettingWord(NULL, "CList", "CycleTime", SETTING_CYCLETIME_DEFAULT) * 1000, cli.pfnTrayCycleTimerProc);
				changed =
					cli.pfnTrayIconSetBaseInfo(ImageList_GetIcon
					(hCListImages, cli.pfnIconFromStatusMode(szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0), NULL),
					ILD_NORMAL), NULL);
				break;
			case SETTING_TRAYICON_MULTI:
				if (!cli.trayIcon) {
					cli.pfnTrayIconRemove(NULL, NULL);
				}
				else if (DBGetContactSettingByte( NULL, "CList", "AlwaysMulti", SETTING_ALWAYSMULTI_DEFAULT ))
					changed = cli.pfnTrayIconSetBaseInfo( cli.pfnGetIconFromStatusMode( NULL, szChangedProto, CallProtoService(szChangedProto, PS_GETSTATUS, 0, 0)), (char*)szChangedProto );
				else {
					cli.pfnTrayIconDestroy(hwnd);
					cli.pfnTrayIconInit(hwnd);
				}
				break;
			}
		}
	}
	else
		changed = cli.pfnTrayIconSetBaseInfo(ImageList_GetIcon(hCListImages, cli.pfnIconFromStatusMode(NULL, averageMode, NULL), ILD_NORMAL), NULL);
	if (changed != -1 && cli.trayIcon[changed].isBase)
		cli.pfnTrayIconUpdate(cli.trayIcon[changed].hBaseIcon, NULL, cli.trayIcon[changed].szProto, 1);
	ulock;
}

void fnTrayIconSetToBase(char *szPreferredProto)
{
	int i;
	initcheck;
	lock;
	for (i = 0; i < cli.trayIconCount; i++) {
		if ( cli.trayIcon[i].id == 0 )
			continue;
		if ( lstrcmpA( cli.trayIcon[i].szProto, szPreferredProto ))
			continue;
		cli.pfnTrayIconUpdate( cli.trayIcon[i].hBaseIcon, NULL, szPreferredProto, 1);
		{ ulock; return; }
	}

	//if there wasn't a specific icon, there will only be one suitable
	for ( i = 0; i < cli.trayIconCount; i++) {
		if ( cli.trayIcon[i].id == 0 )
			continue;
		cli.pfnTrayIconUpdate( cli.trayIcon[i].hBaseIcon, NULL, szPreferredProto, 1);
		{ ulock; return; }
	}
	{ ulock; return; }
}

void fnTrayIconIconsChanged(void)
{
	initcheck;
	lock;
	cli.pfnTrayIconDestroy(cli.hwndContactList);
	cli.pfnTrayIconInit(cli.hwndContactList);
	ulock;
}

static int autoHideTimerId;
static VOID CALLBACK TrayIconAutoHideTimer(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
	HWND hwndClui;
	initcheck;
	lock;
	KillTimer(hwnd, idEvent);
	hwndClui = cli.hwndContactList;
	if (GetActiveWindow() == hwndClui)
	{ ulock; return; }
	ShowWindow(hwndClui, SW_HIDE);
	if (MySetProcessWorkingSetSize != NULL)
		MySetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
	{ ulock; return; }
}

int fnTrayIconPauseAutoHide(WPARAM wParam, LPARAM lParam)
{
	initcheck 0;
	lock;
	if (DBGetContactSettingByte(NULL, "CList", "AutoHide", SETTING_AUTOHIDE_DEFAULT)) {
		if ( GetActiveWindow() != cli.hwndContactList ) {
			KillTimer(NULL, autoHideTimerId);
			autoHideTimerId = SetTimer(NULL, 0, 1000 * DBGetContactSettingWord(NULL, "CList", "HideTime", SETTING_HIDETIME_DEFAULT), TrayIconAutoHideTimer);
		}
	}
	{ ulock; return 0; }
}

/////////////////////////////////////////////////////////////////////////////////////////
// processes tray icon's messages

static BYTE s_LastHoverIconID = 0;
static BOOL g_trayTooltipActive = FALSE;
static POINT tray_hover_pos = {0};

static void CALLBACK TrayHideToolTipTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD elapsed)
{
	if ( g_trayTooltipActive ) {
		POINT pt;
		GetCursorPos(&pt);
		if ( abs(pt.x - tray_hover_pos.x) > TOOLTIP_TOLERANCE || abs(pt.y - tray_hover_pos.y) > TOOLTIP_TOLERANCE ) {
			CallService("mToolTip/HideTip", 0, 0);
			g_trayTooltipActive = FALSE;
			KillTimer( hwnd, TIMERID_TRAYHOVER_2 );
		}
	}
	else KillTimer( hwnd, TIMERID_TRAYHOVER_2 );
}

static void CALLBACK TrayToolTipTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD elapsed)
{
	if ( !g_trayTooltipActive && !cli.bTrayMenuOnScreen ) {
		CLCINFOTIP ti = {0};
		POINT pt;
		GetCursorPos( &pt );
		if ( abs(pt.x - tray_hover_pos.x) <= TOOLTIP_TOLERANCE && abs(pt.y - tray_hover_pos.y) <= TOOLTIP_TOLERANCE ) {
			TCHAR* szTipCur = cli.szTip;
			{
				int n = s_LastHoverIconID-100;
				if ( n >= 0 && n < cli.trayIconCount )
					szTipCur = cli.trayIcon[n].ptszToolTip;
			}
			ti.rcItem.left   = pt.x - 10;
			ti.rcItem.right  = pt.x + 10;
			ti.rcItem.top    = pt.y - 10;
			ti.rcItem.bottom = pt.y + 10;
			ti.cbSize = sizeof( ti );
			ti.isTreeFocused = GetFocus() == cli.hwndContactList ? 1 : 0;
			#if defined( _UNICODE )
			{	char* p = u2a( szTipCur );
	        	CallService( "mToolTip/ShowTip", (WPARAM)p, (LPARAM)&ti );
				mir_free( p );
			}
			#else
	        	CallService( "mToolTip/ShowTip", (WPARAM)szTipCur, (LPARAM)&ti );
			#endif
			GetCursorPos( &tray_hover_pos );
			SetTimer( cli.hwndContactList, TIMERID_TRAYHOVER_2, 600, TrayHideToolTipTimerProc );
			g_trayTooltipActive = TRUE;
	}	}

	KillTimer(hwnd, id);
}

int fnTrayIconProcessMessage(WPARAM wParam, LPARAM lParam)
{
	MSG *msg = (MSG *) wParam;
	switch (msg->message) {
	case WM_CREATE: {
		WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");
		PostMessage(msg->hwnd, TIM_CREATE, 0, 0);
		break;
	}
	case TIM_CREATE:
		cli.pfnTrayIconInit(msg->hwnd);
		break;

	case WM_ACTIVATE:
		if (DBGetContactSettingByte(NULL, "CList", "AutoHide", SETTING_AUTOHIDE_DEFAULT)) {
			if (LOWORD(msg->wParam) == WA_INACTIVE)
				autoHideTimerId = SetTimer(NULL, 0, 1000 * DBGetContactSettingWord(NULL, "CList", "HideTime", SETTING_HIDETIME_DEFAULT), TrayIconAutoHideTimer);
			else
				KillTimer(NULL, autoHideTimerId);
		}
		break;

	case WM_DESTROY:
		cli.pfnTrayIconDestroy(msg->hwnd);
		cli.pfnUninitTray();
		break;

	case TIM_CALLBACK:
		if ( msg->lParam == WM_RBUTTONDOWN || msg->lParam == WM_LBUTTONDOWN || msg->lParam == WM_RBUTTONDOWN && g_trayTooltipActive ) {
			CallService("mToolTip/HideTip", 0, 0);
			g_trayTooltipActive = FALSE;
		}

		if ( msg->lParam == WM_MBUTTONUP )
			cli.pfnShowHide(0, 0);
		else if (msg->lParam == (DBGetContactSettingByte(NULL, "CList", "Tray1Click", SETTING_TRAY1CLICK_DEFAULT) ? WM_LBUTTONUP : WM_LBUTTONDBLCLK)) {
			if ((GetAsyncKeyState(VK_CONTROL) & 0x8000))
				cli.pfnShowHide(0, 0);
			else if (cli.pfnEventsProcessTrayDoubleClick(msg->wParam))
				cli.pfnShowHide(0, 0);
		}
		else if (msg->lParam == WM_RBUTTONUP) {
			MENUITEMINFO mi;
			POINT pt;
			HMENU hMainMenu = LoadMenu(cli.hInst, MAKEINTRESOURCE(IDR_CONTEXT));
			HMENU hMenu = GetSubMenu(hMainMenu, 0);
			CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hMenu, 0);

			ZeroMemory(&mi, sizeof(mi));
			mi.cbSize = MENUITEMINFO_V4_SIZE;
			mi.fMask = MIIM_SUBMENU | MIIM_TYPE;
			mi.fType = MFT_STRING;
			mi.hSubMenu = (HMENU) CallService(MS_CLIST_MENUGETMAIN, 0, 0);
			mi.dwTypeData = TranslateT("&Main Menu");
			InsertMenuItem(hMenu, 1, TRUE, &mi);
			mi.hSubMenu = (HMENU) CallService(MS_CLIST_MENUGETSTATUS, 0, 0);
			mi.dwTypeData = TranslateT("&Status");
			InsertMenuItem(hMenu, 2, TRUE, &mi);
			SetMenuDefaultItem(hMenu, ID_TRAY_HIDE, FALSE);

			SetForegroundWindow(msg->hwnd);
			SetFocus(msg->hwnd);
			GetCursorPos(&pt);
			TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, msg->hwnd, NULL);

			RemoveMenu(hMenu, 1, MF_BYPOSITION);
			RemoveMenu(hMenu, 1, MF_BYPOSITION);
			DestroyMenu(hMainMenu);
		}
		else if ( msg->lParam == WM_MOUSEMOVE ) {
			s_LastHoverIconID = msg->wParam;
			if ( g_trayTooltipActive ) {
				POINT pt;
				GetCursorPos( &pt );
				if ( abs(pt.x - tray_hover_pos.x) > TOOLTIP_TOLERANCE || abs(pt.y - tray_hover_pos.y) > TOOLTIP_TOLERANCE ) {
					CallService("mToolTip/HideTip", 0, 0);
					g_trayTooltipActive = FALSE;
					ReleaseCapture();
				}
			}
			else {
				GetCursorPos(&tray_hover_pos);
				SetTimer(cli.hwndContactList, TIMERID_TRAYHOVER, 600, TrayToolTipTimerProc);
			}
			break;
		}

		*((LRESULT *) lParam) = 0;
		return TRUE;
	
	default:
		if ( msg->message == WM_TASKBARCREATED ) {
			cli.pfnTrayIconTaskbarCreated(msg->hwnd);
			*((LRESULT *) lParam) = 0;
			return TRUE;
	}	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// processes tray icon's notifications

int fnCListTrayNotify(MIRANDASYSTRAYNOTIFY *msn)
{
 #if defined(_UNICODE)
	if ( msn->dwInfoFlags & NIIF_INTERN_UNICODE ) {
		if (msn && msn->cbSize == sizeof(MIRANDASYSTRAYNOTIFY) && msn->szInfo && msn->szInfoTitle) {
			if (cli.trayIcon) {
				NOTIFYICONDATAW nid = {0};
				nid.cbSize = ( cli.shellVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATAW_V1_SIZE;
				nid.hWnd = cli.hwndContactList;
				if (msn->szProto) {
					int j;
					for (j = 0; j < cli.trayIconCount; j++) {
						if (cli.trayIcon[j].szProto != NULL) {
							if (strcmp(msn->szProto, cli.trayIcon[j].szProto) == 0) {
								nid.uID = cli.trayIcon[j].id;
								j = cli.trayIconCount;
								continue;
							}
						} else {
							if (cli.trayIcon[j].isBase) {
								nid.uID = cli.trayIcon[j].id;
								j = cli.trayIconCount;
								continue;
							}
						}
					} //for
				} else {
					nid.uID = cli.trayIcon[0].id;
				}
				nid.uFlags = NIF_INFO;
				lstrcpynW(nid.szInfo, (WCHAR *)msn->szInfo, SIZEOF(nid.szInfo));
				lstrcpynW(nid.szInfoTitle, (WCHAR *)msn->szInfoTitle, SIZEOF(nid.szInfoTitle));
				nid.szInfo[SIZEOF(nid.szInfo) - 1] = 0;
				nid.szInfoTitle[SIZEOF(nid.szInfoTitle) - 1] = 0;
				nid.uTimeout = msn->uTimeout;
				nid.dwInfoFlags = (msn->dwInfoFlags & ~NIIF_INTERN_UNICODE);
				return Shell_NotifyIconW(NIM_MODIFY, (void*) &nid) == 0;
			}
			return 2;
		}
	}
	else 
	#endif
		if (msn && msn->cbSize == sizeof(MIRANDASYSTRAYNOTIFY) && msn->szInfo && msn->szInfoTitle) {
			if (cli.trayIcon) {
				NOTIFYICONDATAA nid = { 0 };
				nid.cbSize = ( cli.shellVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATAA_V1_SIZE;
				nid.hWnd = cli.hwndContactList;
				if (msn->szProto) {
					int j;
					for (j = 0; j < cli.trayIconCount; j++) {
						if (cli.trayIcon[j].szProto != NULL) {
							if (strcmp(msn->szProto, cli.trayIcon[j].szProto) == 0) {
								nid.uID = cli.trayIcon[j].id;
								j = cli.trayIconCount;
								continue;
							}
						}
						else {
							if (cli.trayIcon[j].isBase) {
								nid.uID = cli.trayIcon[j].id;
								j = cli.trayIconCount;
								continue;
							}
						}
					}               //for
				}
				else {
					nid.uID = cli.trayIcon[0].id;
				}
				nid.uFlags = NIF_INFO;
				lstrcpynA(nid.szInfo, msn->szInfo, sizeof(nid.szInfo));
				lstrcpynA(nid.szInfoTitle, msn->szInfoTitle, sizeof(nid.szInfoTitle));
				nid.uTimeout = msn->uTimeout;
				nid.dwInfoFlags = msn->dwInfoFlags;
				return Shell_NotifyIconA(NIM_MODIFY, (void *) &nid) == 0;
			}
			return 2;
		}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////

typedef struct _DllVersionInfo
{
	DWORD cbSize;
	DWORD dwMajorVersion;       // Major version
	DWORD dwMinorVersion;       // Minor version
	DWORD dwBuildNumber;        // Build number
	DWORD dwPlatformID;         // DLLVER_PLATFORM_*
}
	DLLVERSIONINFO;

typedef HRESULT(CALLBACK * DLLGETVERSIONPROC) (DLLVERSIONINFO *);

static DLLVERSIONINFO dviShell;

static int pfnCListTrayNotifyStub(WPARAM wParam, LPARAM lParam )
{	return cli.pfnCListTrayNotify(( MIRANDASYSTRAYNOTIFY* )lParam );
}

void fnInitTray( void )
{
	HINSTANCE hLib = LoadLibraryA("shell32.dll");
	if ( hLib ) {
		DLLGETVERSIONPROC proc;
		dviShell.cbSize = sizeof(dviShell);
		proc = ( DLLGETVERSIONPROC )GetProcAddress( hLib, "DllGetVersion" );
		if (proc) {
			proc( &dviShell );
			cli.shellVersion = dviShell.dwMajorVersion;
		}
 		FreeLibrary(hLib);
	}
	InitializeCriticalSection(&trayLockCS);	
	if ( cli.shellVersion >= 5 )
		CreateServiceFunction(MS_CLIST_SYSTRAY_NOTIFY, pfnCListTrayNotifyStub );
	fTrayInited=TRUE;
}

void fnUninitTray( void )
{
	fTrayInited=FALSE;
	DeleteCriticalSection( &trayLockCS );
}
void   fnLockTray( void )
{
//	return;				//stub to be removed
	initcheck;
	EnterCriticalSection( &trayLockCS );
}

void   fnUnlockTray( void )
{
//	return;				//stub to be removed
	initcheck;
#ifdef _DEBUG
	if (trayLockCS.RecursionCount==0) DebugBreak();	   //try to unlock already 
#endif
	LeaveCriticalSection( &trayLockCS );
}

#undef lock
#undef ulock
#undef initcheck