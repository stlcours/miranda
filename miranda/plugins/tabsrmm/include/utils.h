/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * utility functions
 *
 */

#ifndef __UTILS_H
#define __UTILS_H

#define RTF_CTABLE_DEFSIZE 8

struct TRTFColorTable {
    TCHAR 		szName[10];
    COLORREF 	clr;
    int 		index;
    int 		menuid;
};

static TRTFColorTable _rtf_ctable[] = {
	_T("red"), RGB(255, 0, 0), 0, ID_FONT_RED,
	_T("blue"), RGB(0, 0, 255), 0, ID_FONT_BLUE,
	_T("green"), RGB(0, 255, 0), 0, ID_FONT_GREEN,
	_T("magenta"), RGB(255, 0, 255), 0, ID_FONT_MAGENTA,
	_T("yellow"), RGB(255, 255, 0), 0, ID_FONT_YELLOW,
	_T("cyan"), RGB(0, 255, 255), 0, ID_FONT_CYAN,
	_T("black"), 0, 0, ID_FONT_BLACK,
	_T("white"), RGB(255, 255, 255), 0, ID_FONT_WHITE,
	_T(""), 0, 0, 0
};

class Utils {

public:
	static	int					TSAPI FindRTLLocale					(_MessageWindowData *dat);
	static  TCHAR* 				TSAPI GetPreviewWithEllipsis		(TCHAR *szText, size_t iMaxLen);
	static	TCHAR* 				TSAPI FilterEventMarkers			(TCHAR *wszText);
	static  const TCHAR* 		TSAPI FormatRaw						(_MessageWindowData *dat, const TCHAR *msg, int flags, BOOL isSent);
	static	const TCHAR* 		TSAPI FormatTitleBar				(const _MessageWindowData *dat, const TCHAR *szFormat);
#if defined(_UNICODE)
	static	char* 				TSAPI FilterEventMarkers			(char *szText);
#endif
	static	const TCHAR* 		TSAPI DoubleAmpersands				(TCHAR *pszText);
	static	void 				TSAPI RTF_CTableInit				();
	static	void 				TSAPI RTF_ColorAdd					(const TCHAR *tszColname, size_t length);
	static	void 				TSAPI CreateColorMap				(TCHAR *Text);
	static	int 				TSAPI RTFColorToIndex				(int iCol);

public:
	static	TRTFColorTable*		rtf_ctable;
	static	int					rtf_ctable_size;
};

#endif /* __UTILS_H */