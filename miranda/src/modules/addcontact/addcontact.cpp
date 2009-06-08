/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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

INT_PTR CALLBACK AddContactDlgProc(HWND hdlg,UINT msg,WPARAM wparam,LPARAM lparam)
{
	ADDCONTACTSTRUCT *acs;

	switch(msg) {
	case WM_INITDIALOG:
		{
			char szUin[10];
			acs=(ADDCONTACTSTRUCT *)lparam;
			SetWindowLongPtr(hdlg,GWLP_USERDATA,(LONG)acs);

			TranslateDialogDefault(hdlg);
			Window_SetIcon_IcoLib(hdlg,SKINICON_OTHER_ADDCONTACT);
			if ( acs->handleType == HANDLE_EVENT ) {
				DWORD dwUin;
				DBEVENTINFO dbei = { 0 };
				dbei.cbSize=sizeof(dbei);
				dbei.cbBlob=sizeof(DWORD);
				dbei.pBlob=(PBYTE)&dwUin;
				CallService(MS_DB_EVENT_GET,(WPARAM)acs->handle,(LPARAM)&dbei);
				_ltoa(dwUin,szUin,10);
				acs->szProto = dbei.szModule;
			}
			{
				TCHAR* szName = NULL;
				if ( acs->handleType == HANDLE_CONTACT )
					szName = cli.pfnGetContactDisplayName( acs->handle, GCDNF_TCHAR );
				else {
					char *p;
					int isSet = 0;

					if (acs->handleType == HANDLE_EVENT) {
						DBEVENTINFO dbei;
						HANDLE hcontact;

						ZeroMemory(&dbei,sizeof(dbei));
						dbei.cbSize=sizeof(dbei);
						dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)acs->handle,0);
						dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob);
						CallService(MS_DB_EVENT_GET,(WPARAM)acs->handle,(LPARAM)&dbei);
						hcontact=*((PHANDLE)(dbei.pBlob+sizeof(DWORD)));
						mir_free(dbei.pBlob);
						if (hcontact!=INVALID_HANDLE_VALUE) {
							szName = cli.pfnGetContactDisplayName( hcontact, 0 );
							isSet = 1;
						}
					}
					if (!isSet) {
						p = (acs->handleType == HANDLE_EVENT) ? szUin : acs->psr->nick;
						#if defined( _UNICODE )
							szName =( TCHAR* )alloca( 128*sizeof( TCHAR ));
							MultiByteToWideChar( CP_ACP, 0, p, -1, szName, 128 );
						#else
							szName = p;
						#endif
				}	}

				if ( lstrlen( szName )) {
					TCHAR  szTitle[128];
					mir_sntprintf( szTitle, SIZEOF(szTitle), TranslateT("Add %s"), szName );
					SetWindowText( hdlg, szTitle );
				}
				else SetWindowText( hdlg, TranslateT("Add Contact"));
		}	}

		if ( acs->handleType == HANDLE_CONTACT && acs->handle )
			if ( acs->szProto == NULL || (acs->szProto != NULL && *acs->szProto == 0 ))
				acs->szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)acs->handle,0);
		
		{
			int groupId;
			for ( groupId = 0; groupId < 999; groupId++ ) {
				DBVARIANT dbv;
				char idstr[4];
				int id;
				_itoa(groupId,idstr,10);
				if(DBGetContactSettingTString(NULL,"CListGroups",idstr,&dbv)) break;
				id = SendDlgItemMessage(hdlg,IDC_GROUP,CB_ADDSTRING,0,(LPARAM)(dbv.ptszVal+1));
				SendDlgItemMessage(hdlg,IDC_GROUP,CB_SETITEMDATA ,id,groupId+1);
				DBFreeVariant(&dbv);
		}	}

		SendDlgItemMessage(hdlg,IDC_GROUP,CB_INSERTSTRING,0,(LPARAM)TranslateT("None"));
		SendDlgItemMessage(hdlg,IDC_GROUP,CB_SETCURSEL,0,0);
		/* acs->szProto may be NULL don't expect it */
		{
			DWORD flags = (acs->szProto) ? CallProtoService(acs->szProto,PS_GETCAPS,PFLAGNUM_4,0) : 0;
			if (flags&PF4_FORCEADDED) { // force you were added requests for this protocol
				CheckDlgButton(hdlg,IDC_ADDED,BST_CHECKED);
				EnableWindow(GetDlgItem(hdlg,IDC_ADDED),FALSE);
			}
			if (flags&PF4_FORCEAUTH) { // force auth requests for this protocol
				CheckDlgButton(hdlg,IDC_AUTH,BST_CHECKED);
				EnableWindow(GetDlgItem(hdlg,IDC_AUTH),FALSE);
			}
			if (flags&PF4_NOCUSTOMAUTH) {
				EnableWindow(GetDlgItem(hdlg,IDC_AUTHREQ),FALSE);
				EnableWindow(GetDlgItem(hdlg,IDC_AUTHGB),FALSE);
		}	}
		SetDlgItemText(hdlg,IDC_AUTHREQ,TranslateT("Please authorize my request and add me to your contact list."));
		EnableWindow(GetDlgItem(hdlg,IDC_AUTHREQ),IsDlgButtonChecked(hdlg,IDC_AUTH));
		EnableWindow(GetDlgItem(hdlg,IDC_AUTHGB),IsDlgButtonChecked(hdlg,IDC_AUTH));
		break;

	case WM_COMMAND:
		acs=(ADDCONTACTSTRUCT *)GetWindowLongPtr(hdlg,GWLP_USERDATA);

		switch(LOWORD(wparam)) {
		case IDC_AUTH:
			{
				DWORD flags = CallProtoService(acs->szProto,PS_GETCAPS,PFLAGNUM_4,0);
				if (flags & PF4_NOCUSTOMAUTH) {
					EnableWindow(GetDlgItem(hdlg,IDC_AUTHREQ),FALSE);
					EnableWindow(GetDlgItem(hdlg,IDC_AUTHGB),FALSE);
				}
				else {
					EnableWindow(GetDlgItem(hdlg,IDC_AUTHREQ),IsDlgButtonChecked(hdlg,IDC_AUTH));
					EnableWindow(GetDlgItem(hdlg,IDC_AUTHGB),IsDlgButtonChecked(hdlg,IDC_AUTH));
				}
			}
			break;
		case IDOK:
			{
				HANDLE hcontact = INVALID_HANDLE_VALUE;
				switch ( acs->handleType ) {
				case HANDLE_EVENT:
					{
						DBEVENTINFO dbei = { 0 };
						dbei.cbSize = sizeof(dbei);
						CallService(MS_DB_EVENT_GET,(WPARAM)acs->handle,(LPARAM)&dbei);
						hcontact = (HANDLE)CallProtoService(dbei.szModule,PS_ADDTOLISTBYEVENT,0,(LPARAM)acs->handle);
               }
					break;
				case HANDLE_SEARCHRESULT:
					hcontact = (HANDLE)CallProtoService(acs->szProto,PS_ADDTOLIST,0,(LPARAM)acs->psr);
					break;
				case HANDLE_CONTACT:
					hcontact = acs->handle;
					break;
				}

				if ( hcontact == NULL )
					break;

				{	TCHAR szHandle[256]; int item;
					if ( GetDlgItemText( hdlg, IDC_MYHANDLE, szHandle, SIZEOF(szHandle)))
						DBWriteContactSettingTString( hcontact, "CList", "MyHandle", szHandle );

					item = SendDlgItemMessage(hdlg, IDC_GROUP, CB_GETCURSEL, 0, 0);
					if (item > 0) {
						item = SendDlgItemMessage(hdlg, IDC_GROUP, CB_GETITEMDATA, item, 0);
						CallService(MS_CLIST_CONTACTCHANGEGROUP, (WPARAM)hcontact, item);
					}
				}

				if ( IsDlgButtonChecked( hdlg, IDC_ADDED ))
					CallContactService( hcontact, PSS_ADDED, 0, 0 );

				if ( IsDlgButtonChecked( hdlg, IDC_AUTH )) {
					DWORD flags = CallProtoService( acs->szProto, PS_GETCAPS, PFLAGNUM_4, 0 );
					if ( flags & PF4_NOCUSTOMAUTH )
						CallContactService( hcontact, PSS_AUTHREQUEST, 0, (LPARAM)"" );
					else {
						char szReason[256];
						GetDlgItemTextA(hdlg,IDC_AUTHREQ,szReason,256);
						CallContactService(hcontact,PSS_AUTHREQUEST,0,(LPARAM)szReason);
				}	}

				DBDeleteContactSetting(hcontact,"CList","NotOnList");
			}
			// fall through
		case IDCANCEL:
			if ( GetParent( hdlg ) == NULL)
				DestroyWindow( hdlg );
			else
				EndDialog( hdlg, 0 );
			break;
		}
		break;

	case WM_CLOSE:
		/* if there is no parent for the dialog, its a modeless dialog and can't be killed using EndDialog() */
		if ( GetParent( hdlg ) == NULL )
			DestroyWindow(hdlg);
		else
			EndDialog( hdlg, 0 );
		break;

	case WM_DESTROY:
		Window_FreeIcon_IcoLib(hdlg);
		acs = ( ADDCONTACTSTRUCT* )GetWindowLongPtr(hdlg,GWLP_USERDATA);
		if (acs) {
			if (acs->psr) {
				mir_free(acs->psr->nick);
				mir_free(acs->psr->firstName);
				mir_free(acs->psr->lastName);
				mir_free(acs->psr->email);
				mir_free(acs->psr);
			}
			mir_free(acs);
		}
		break;
	}

	return FALSE;
}

int AddContactDialog(WPARAM wParam,LPARAM lParam)
{
	if (lParam) {
		ADDCONTACTSTRUCT* acs = ( ADDCONTACTSTRUCT* )mir_alloc(sizeof(ADDCONTACTSTRUCT));
		memmove( acs, ( ADDCONTACTSTRUCT* )lParam, sizeof( ADDCONTACTSTRUCT ));
		if ( acs->psr ) {
			PROTOSEARCHRESULT *psr;
			/* bad! structures that are bigger than psr will cause crashes if they define pointers within unreachable structural space */
			psr = (PROTOSEARCHRESULT *)mir_alloc(acs->psr->cbSize);
			memmove(psr,acs->psr,acs->psr->cbSize);
			psr->nick = mir_strdup(psr->nick);
			psr->firstName = mir_strdup(psr->firstName);
			psr->lastName = mir_strdup(psr->lastName);
			psr->email = mir_strdup(psr->email);
			acs->psr = psr;
			/* copied the passed acs structure, the psr structure with, the pointers within that  */
		}

		if ( wParam )
			DialogBoxParam(hMirandaInst,MAKEINTRESOURCE(IDD_ADDCONTACT),(HWND)wParam,AddContactDlgProc,(LPARAM)acs);
		else
			CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(IDD_ADDCONTACT),(HWND)wParam,AddContactDlgProc,(LPARAM)acs);
		return 0;
	}
	return 1;
}

int LoadAddContactModule(void)
{
	CreateServiceFunction(MS_ADDCONTACT_SHOW,AddContactDialog);
	return 0;
}
