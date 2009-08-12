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
 * Implements the skinning engine and most parts of the aero support in
 * tabSRMM 3.x+
 *
 */

#include "commonheaders.h"
#pragma hdrstop

/*
 * writes the current "theme" to .ini file
 * a theme contains all the fonts, colors and message log formatting
 * options.
 */

CSkin* Skin = 0;


static void __inline gradientVertical(UCHAR *ubRedFinal, UCHAR *ubGreenFinal, UCHAR *ubBlueFinal,
									  ULONG ulBitmapHeight, UCHAR ubRed, UCHAR ubGreen, UCHAR ubBlue, UCHAR ubRed2,
									  UCHAR ubGreen2, UCHAR ubBlue2, DWORD FLG_GRADIENT, BOOL transparent, UINT32 y, UCHAR *ubAlpha);

static void __inline gradientHorizontal(UCHAR *ubRedFinal, UCHAR *ubGreenFinal, UCHAR *ubBlueFinal,
										ULONG ulBitmapWidth, UCHAR ubRed, UCHAR ubGreen, UCHAR ubBlue,  UCHAR ubRed2,
										UCHAR ubGreen2, UCHAR ubBlue2, DWORD FLG_GRADIENT, BOOL transparent, UINT32 x, UCHAR *ubAlpha);


int  		SIDEBARWIDTH = DEFAULT_SIDEBARWIDTH;
UINT 		nextButtonID;
ButtonSet 	g_ButtonSet = {0};

#define   	NR_MAXSKINICONS 100

/*
 * initialize static class data
*/

int CSkin::m_bAvatarBorderType = 0;

BLENDFUNCTION CSkin::m_default_bf = {0};

bool CSkin::m_bClipBorder = false, CSkin::m_DisableScrollbars = false,
	 CSkin::m_skinEnabled = false, CSkin::m_frameSkins = false;

char CSkin::m_SkinnedFrame_left = 0, CSkin::m_SkinnedFrame_right = 0,
	 CSkin::m_SkinnedFrame_bottom = 0, CSkin::m_SkinnedFrame_caption = 0;

char CSkin::m_realSkinnedFrame_left = 0;
char CSkin::m_realSkinnedFrame_right = 0;
char CSkin::m_realSkinnedFrame_bottom = 0;
char CSkin::m_realSkinnedFrame_caption = 0;

int CSkin::m_titleBarLeftOff = 0, CSkin::m_titleButtonTopOff = 0, CSkin::m_captionOffset = 0, CSkin::m_captionPadding = 0,
	CSkin::m_titleBarRightOff = 0, CSkin::m_sidebarTopOffset = 0, CSkin::m_sidebarBottomOffset = 0, CSkin::m_bRoundedCorner = 0;

SIZE CSkin::m_titleBarButtonSize = {0};

COLORREF CSkin::m_ContainerColorKey = 0;
HBRUSH 	 CSkin::m_ContainerColorKeyBrush = 0, CSkin::m_MenuBGBrush = 0;

HPEN 	 CSkin::m_SkinLightShadowPen = 0, CSkin::m_SkinDarkShadowPen = 0;

HICON	 CSkin::m_closeIcon = 0, CSkin::m_maxIcon = 0, CSkin::m_minIcon = 0;

/*
 * definition of the availbale skin items
 */

CSkinItem SkinItems[] = {
	{_T("Container"), "TSKIN_Container", ID_EXTBKCONTAINER,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Toolbar"), "TSKIN_Container", ID_EXTBKBUTTONBAR,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("{-}Buttonpressed"), "TSKIN_BUTTONSPRESSED", ID_EXTBKBUTTONSPRESSED,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Buttonnotpressed"), "TSKIN_BUTTONSNPRESSED", ID_EXTBKBUTTONSNPRESSED,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Buttonmouseover"), "TSKIN_BUTTONSMOUSEOVER", ID_EXTBKBUTTONSMOUSEOVER,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Infopanelfield"), "TSKIN_INFOPANELFIELD", ID_EXTBKINFOPANEL,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Titlebutton"), "TSKIN_TITLEBUTTON", ID_EXTBKTITLEBUTTON,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Titlebuttonmouseover"), "TSKIN_TITLEBUTTONHOVER", ID_EXTBKTITLEBUTTONMOUSEOVER,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Titlebuttonpressed"), "TSKIN_TITLEBUTTONPRESSED", ID_EXTBKTITLEBUTTONPRESSED,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Tabpage"), "TSKIN_TABPAGE", ID_EXTBKTABPAGE,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Tabitem"), "TSKIN_TABITEM", ID_EXTBKTABITEM,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Tabitem_active"), "TSKIN_TABITEMACTIVE", ID_EXTBKTABITEMACTIVE,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Tabitem_bottom"), "TSKIN_TABITEMBOTTOM", ID_EXTBKTABITEMBOTTOM,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Tabitem_active_bottom"), "TSKIN_TABITEMACTIVEBOTTOM", ID_EXTBKTABITEMACTIVEBOTTOM,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Frame"), "TSKIN_FRAME", ID_EXTBKFRAME,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("MessageLog"), "TSKIN_MLOG", ID_EXTBKHISTORY,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("InputArea"), "TSKIN_INPUT", ID_EXTBKINPUTAREA,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("FrameInactive"), "TSKIN_FRAMEINACTIVE", ID_EXTBKFRAMEINACTIVE,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Tabitem_hottrack"), "TSKIN_TABITEMHOTTRACK", ID_EXTBKTABITEMHOTTRACK,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Tabitem_hottrack_bottom"), "TSKIN_TABITEMHOTTRACKBOTTOM", ID_EXTBKTABITEMHOTTRACKBOTTOM,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Statusbarpanel"), "TSKIN_STATUSBARPANEL", ID_EXTBKSTATUSBARPANEL,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Statusbar"), "TSKIN_STATUSBAR", ID_EXTBKSTATUSBAR,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("Userlist"), "TSKIN_USERLIST", ID_EXTBKUSERLIST,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {_T("InfoPanelBackground"), "TSKIN_INFOPANELBG", ID_EXTBKINFOPANELBG,
		8, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, 0xf0f0f0, 1, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}
};

static BYTE __forceinline percent_to_byte(UINT32 percent)
{
	return(BYTE)((FLOAT)(((FLOAT) percent) / 100) * 255);
}

static COLORREF __forceinline revcolref(COLORREF colref)
{
	return RGB(GetBValue(colref), GetGValue(colref), GetRValue(colref));
}

static DWORD __forceinline argb_from_cola(COLORREF col, UINT32 alpha)
{
	return((BYTE) percent_to_byte(alpha) << 24 | col);
}


void DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, int alpha, DWORD basecolor2, BYTE transparent, BYTE FLG_GRADIENT, BYTE FLG_CORNER, DWORD BORDERSTYLE, CImageItem *imageItem)
{
	HBRUSH BrMask;
	HBRUSH holdbrush;
	HDC hdc;
	BLENDFUNCTION bf;
	HBITMAP hbitmap;
	HBITMAP holdbitmap;
	BITMAPINFO bmi;
	VOID *pvBits;
	UINT32 x, y;
	ULONG ulBitmapWidth, ulBitmapHeight;
	UCHAR ubAlpha = 0xFF;
	UCHAR ubRedFinal = 0xFF;
	UCHAR ubGreenFinal = 0xFF;
	UCHAR ubBlueFinal = 0xFF;
	UCHAR ubRed;
	UCHAR ubGreen;
	UCHAR ubBlue;
	UCHAR ubRed2;
	UCHAR ubGreen2;
	UCHAR ubBlue2;

	int realx;

	FLOAT fAlphaFactor;
	LONG realHeight = (rc->bottom - rc->top);
	LONG realWidth = (rc->right - rc->left);
	LONG realHeightHalf = realHeight >> 1;

	if (rc == NULL || CMimAPI::m_MyGradientFill == 0 || CMimAPI::m_MyAlphaBlend == 0)
		return;

	if (imageItem) {
		imageItem->Render(hdcwnd, rc, false);
		//IMG_RenderImageItem(hdcwnd, imageItem, rc);
		return;
	}

	if (rc->right < rc->left || rc->bottom < rc->top || (realHeight <= 0) || (realWidth <= 0))
		return;

	/*
	 * use GDI fast gradient drawing when no corner radi exist
	 */

	if (FLG_CORNER == 0) {
		GRADIENT_RECT grect;
		TRIVERTEX tvtx[2];
		int orig = 1, dest = 0;

		if (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_TB) {
			orig = 0;
			dest = 1;
		}

		tvtx[0].x = rc->left;
		tvtx[0].y = rc->top;
		tvtx[1].x = rc->right;
		tvtx[1].y = rc->bottom;

		tvtx[orig].Red = (COLOR16)GetRValue(basecolor) << 8;
		tvtx[orig].Blue = (COLOR16)GetBValue(basecolor) << 8;
		tvtx[orig].Green = (COLOR16)GetGValue(basecolor) << 8;
		tvtx[orig].Alpha = (COLOR16) alpha << 8;

		tvtx[dest].Red = (COLOR16)GetRValue(basecolor2) << 8;
		tvtx[dest].Blue = (COLOR16)GetBValue(basecolor2) << 8;
		tvtx[dest].Green = (COLOR16)GetGValue(basecolor2) << 8;
		tvtx[dest].Alpha = (COLOR16) alpha << 8;

		grect.UpperLeft = 0;
		grect.LowerRight = 1;

		CMimAPI::m_MyGradientFill(hdcwnd, tvtx, 2, &grect, 1, (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT) ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
		return;
	}

	hdc = CreateCompatibleDC(hdcwnd);
	if (!hdc)
		return;

	ZeroMemory(&bmi, sizeof(BITMAPINFO));

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	if (FLG_GRADIENT & GRADIENT_ACTIVE && (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_RL)) {
		bmi.bmiHeader.biWidth = ulBitmapWidth = realWidth;
		bmi.bmiHeader.biHeight = ulBitmapHeight = 1;
	} else if (FLG_GRADIENT & GRADIENT_ACTIVE && (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT)) {
		bmi.bmiHeader.biWidth = ulBitmapWidth = 1;
		bmi.bmiHeader.biHeight = ulBitmapHeight = realHeight;
	} else {
		bmi.bmiHeader.biWidth = ulBitmapWidth = 1;
		bmi.bmiHeader.biHeight = ulBitmapHeight = 1;
	}

	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = ulBitmapWidth * ulBitmapHeight * 4;

	if (ulBitmapWidth <= 0 || ulBitmapHeight <= 0)
		return;

	hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	if (hbitmap == NULL || pvBits == NULL) {
		DeleteDC(hdc);
		return;
	}

	holdbitmap = (HBITMAP)SelectObject(hdc, hbitmap);

	// convert basecolor to RGB and then merge alpha so its ARGB
	basecolor = argb_from_cola(revcolref(basecolor), alpha);
	basecolor2 = argb_from_cola(revcolref(basecolor2), alpha);

	ubRed = (UCHAR)(basecolor >> 16);
	ubGreen = (UCHAR)(basecolor >> 8);
	ubBlue = (UCHAR) basecolor;

	ubRed2 = (UCHAR)(basecolor2 >> 16);
	ubGreen2 = (UCHAR)(basecolor2 >> 8);
	ubBlue2 = (UCHAR) basecolor2;

	//DRAW BASE - make corner space 100% transparent
	for (y = 0; y < ulBitmapHeight; y++) {
		for (x = 0 ; x < ulBitmapWidth ; x++) {
			if (FLG_GRADIENT & GRADIENT_ACTIVE) {
				if (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_RL) {
					realx = x + realHeightHalf;
					realx = (ULONG) realx > ulBitmapWidth ? ulBitmapWidth : realx;
					gradientHorizontal(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, ulBitmapWidth, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, realx, &ubAlpha);
				} else if (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT)
					gradientVertical(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, ulBitmapHeight, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, y, &ubAlpha);

				fAlphaFactor = (float) ubAlpha / (float) 0xff;
				((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
			} else {
				ubAlpha = percent_to_byte(alpha);
				ubRedFinal = ubRed;
				ubGreenFinal = ubGreen;
				ubBlueFinal = ubBlue;
				fAlphaFactor = (float) ubAlpha / (float) 0xff;

				((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
			}
		}
	}
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = (UCHAR)(basecolor >> 24);
	bf.AlphaFormat = AC_SRC_ALPHA; // so it will use our specified alpha value

	CMimAPI::m_MyAlphaBlend(hdcwnd, rc->left + realHeightHalf, rc->top, (realWidth - realHeightHalf * 2), realHeight, hdc, 0, 0, ulBitmapWidth, ulBitmapHeight, bf);

	SelectObject(hdc, holdbitmap);
	DeleteObject(hbitmap);

	// corners
	BrMask = CreateSolidBrush(RGB(0xFF, 0x00, 0xFF));
	{
		bmi.bmiHeader.biWidth = ulBitmapWidth = realHeightHalf;
		bmi.bmiHeader.biHeight = ulBitmapHeight = realHeight;
		bmi.bmiHeader.biSizeImage = ulBitmapWidth * ulBitmapHeight * 4;

		if (ulBitmapWidth <= 0 || ulBitmapHeight <= 0) {
			DeleteDC(hdc);
			DeleteObject(BrMask);
			return;
		}

		// TL+BL CORNER
		hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);

		if (hbitmap == 0 || pvBits == NULL)  {
			DeleteObject(BrMask);
			DeleteDC(hdc);
			return;
		}

		holdbrush = (HBRUSH)SelectObject(hdc, BrMask);
		holdbitmap = (HBITMAP)SelectObject(hdc, hbitmap);
		RoundRect(hdc, -1, -1, ulBitmapWidth * 2 + 1, (realHeight + 1), BORDERSTYLE, BORDERSTYLE);

		for (y = 0; y < ulBitmapHeight; y++) {
			for (x = 0; x < ulBitmapWidth; x++) {
				if (((((UINT32 *) pvBits)[x + y * ulBitmapWidth]) << 8) == 0xFF00FF00 || (y < ulBitmapHeight >> 1 && !(FLG_CORNER & CORNER_BL && FLG_CORNER & CORNER_ACTIVE)) || (y > ulBitmapHeight >> 2 && !(FLG_CORNER & CORNER_TL && FLG_CORNER & CORNER_ACTIVE))) {
					if (FLG_GRADIENT & GRADIENT_ACTIVE) {
						if (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_RL)
							gradientHorizontal(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, realWidth, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, x, &ubAlpha);
						else if (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT)
							gradientVertical(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, ulBitmapHeight, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, y, &ubAlpha);

						fAlphaFactor = (float) ubAlpha / (float) 0xff;
						((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
					} else {
						ubAlpha = percent_to_byte(alpha);
						ubRedFinal = ubRed;
						ubGreenFinal = ubGreen;
						ubBlueFinal = ubBlue;
						fAlphaFactor = (float) ubAlpha / (float) 0xff;

						((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
					}
				}
			}
		}
		CMimAPI::m_MyAlphaBlend(hdcwnd, rc->left, rc->top, ulBitmapWidth, ulBitmapHeight, hdc, 0, 0, ulBitmapWidth, ulBitmapHeight, bf);
		SelectObject(hdc, holdbitmap);
		DeleteObject(hbitmap);

		// TR+BR CORNER
		hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);

		//SelectObject(hdc, BrMask); // already BrMask?
		holdbitmap = (HBITMAP)SelectObject(hdc, hbitmap);
		RoundRect(hdc, -1 - ulBitmapWidth, -1, ulBitmapWidth + 1, (realHeight + 1), BORDERSTYLE, BORDERSTYLE);

		for (y = 0; y < ulBitmapHeight; y++) {
			for (x = 0; x < ulBitmapWidth; x++) {
				if (((((UINT32 *) pvBits)[x + y * ulBitmapWidth]) << 8) == 0xFF00FF00 || (y < ulBitmapHeight >> 1 && !(FLG_CORNER & CORNER_BR && FLG_CORNER & CORNER_ACTIVE)) || (y > ulBitmapHeight >> 1 && !(FLG_CORNER & CORNER_TR && FLG_CORNER & CORNER_ACTIVE))) {
					if (FLG_GRADIENT & GRADIENT_ACTIVE) {
						if (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_RL) {
							realx = x + realWidth;
							realx = realx > realWidth ? realWidth : realx;
							gradientHorizontal(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, realWidth, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, realx, &ubAlpha);
						} else if (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT)
							gradientVertical(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, ulBitmapHeight, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, y, &ubAlpha);

						fAlphaFactor = (float) ubAlpha / (float) 0xff;
						((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
					} else {
						ubAlpha = percent_to_byte(alpha);
						ubRedFinal = ubRed;
						ubGreenFinal = ubGreen;
						ubBlueFinal = ubBlue;
						fAlphaFactor = (float) ubAlpha / (float) 0xff;

						((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
					}
				}
			}
		}
		CMimAPI::m_MyAlphaBlend(hdcwnd, rc->right - realHeightHalf, rc->top, ulBitmapWidth, ulBitmapHeight, hdc, 0, 0, ulBitmapWidth, ulBitmapHeight, bf);
	}
	SelectObject(hdc, holdbitmap);
	DeleteObject(hbitmap);
	SelectObject(hdc, holdbrush);
	DeleteObject(BrMask);
	DeleteDC(hdc);
}

void __forceinline gradientHorizontal(UCHAR *ubRedFinal, UCHAR *ubGreenFinal, UCHAR *ubBlueFinal, ULONG ulBitmapWidth, UCHAR ubRed, UCHAR ubGreen, UCHAR ubBlue, UCHAR ubRed2, UCHAR ubGreen2, UCHAR ubBlue2, DWORD FLG_GRADIENT, BOOL transparent, UINT32 x, UCHAR *ubAlpha)
{
	FLOAT fSolidMulti, fInvSolidMulti;

	// solid to transparent
	if (transparent) {
		*ubAlpha = (UCHAR)((float) x / (float) ulBitmapWidth * 255);
		*ubAlpha = FLG_GRADIENT & GRADIENT_LR ? 0xFF - (*ubAlpha) : (*ubAlpha);
		*ubRedFinal = ubRed;
		*ubGreenFinal = ubGreen;
		*ubBlueFinal = ubBlue;
	} else { // solid to solid2
		if (FLG_GRADIENT & GRADIENT_LR) {
			fSolidMulti = ((float) x / (float) ulBitmapWidth);
			fInvSolidMulti = 1 - fSolidMulti;
		} else {
			fInvSolidMulti = ((float) x / (float) ulBitmapWidth);
			fSolidMulti = 1 - fInvSolidMulti;
		}

		*ubRedFinal = (UCHAR)((((float) ubRed * (float) fInvSolidMulti)) + (((float) ubRed2 * (float) fSolidMulti)));
		*ubGreenFinal = (UCHAR)((UCHAR)(((float) ubGreen * (float) fInvSolidMulti)) + (((float) ubGreen2 * (float) fSolidMulti)));
		*ubBlueFinal = (UCHAR)((((float) ubBlue * (float) fInvSolidMulti)) + (UCHAR)(((float) ubBlue2 * (float) fSolidMulti)));

		*ubAlpha = 0xFF;
	}
}

void __forceinline gradientVertical(UCHAR *ubRedFinal, UCHAR *ubGreenFinal, UCHAR *ubBlueFinal, ULONG ulBitmapHeight, UCHAR ubRed, UCHAR ubGreen, UCHAR ubBlue, UCHAR ubRed2, UCHAR ubGreen2, UCHAR ubBlue2, DWORD FLG_GRADIENT, BOOL transparent, UINT32 y, UCHAR *ubAlpha)
{
	FLOAT fSolidMulti, fInvSolidMulti;

	// solid to transparent
	if (transparent) {
		*ubAlpha = (UCHAR)((float) y / (float) ulBitmapHeight * 255);
		*ubAlpha = FLG_GRADIENT & GRADIENT_BT ? 0xFF - *ubAlpha : *ubAlpha;
		*ubRedFinal = ubRed;
		*ubGreenFinal = ubGreen;
		*ubBlueFinal = ubBlue;
	} else { // solid to solid2
		if (FLG_GRADIENT & GRADIENT_BT) {
			fSolidMulti = ((float) y / (float) ulBitmapHeight);
			fInvSolidMulti = 1 - fSolidMulti;
		} else {
			fInvSolidMulti = ((float) y / (float) ulBitmapHeight);
			fSolidMulti = 1 - fInvSolidMulti;
		}

		*ubRedFinal = (UCHAR)((((float) ubRed * (float) fInvSolidMulti)) + (((float) ubRed2 * (float) fSolidMulti)));
		*ubGreenFinal = (UCHAR)((((float) ubGreen * (float) fInvSolidMulti)) + (((float) ubGreen2 * (float) fSolidMulti)));
		*ubBlueFinal = (UCHAR)(((float) ubBlue * (float) fInvSolidMulti)) + (UCHAR)(((float) ubBlue2 * (float) fSolidMulti));

		*ubAlpha = 0xFF;
	}
}

/**
 * Renders the image item to the given target device context and rectangle
 *
 * @param hdc    HDC: target device context
 * @param rc     RECT *: client rectangle inside the target DC.
 */
void __fastcall CImageItem::Render(const HDC hdc, const RECT *rc, bool fIgnoreGlyph) const
{
	BYTE l = m_bLeft, r = m_bRight, t = m_bTop, b = m_bBottom;
	LONG width = rc->right - rc->left;
	LONG height = rc->bottom - rc->top;
	BOOL isGlyph = ((m_dwFlags & IMAGE_GLYPH) && Skin->haveGlyphItem());
	BOOL fCleanUp = TRUE;
	HDC hdcSrc = 0;
	HBITMAP hbmOld;
	LONG srcOrigX = isGlyph ? m_glyphMetrics[0] : 0;
	LONG srcOrigY = isGlyph ? m_glyphMetrics[1] : 0;

	if (CMimAPI::m_MyAlphaBlend == 0)
		return;

	if (m_hdc == 0) {
		hdcSrc = CreateCompatibleDC(hdc);
		hbmOld = (HBITMAP)SelectObject(hdcSrc, isGlyph ? Skin->getGlyphItem()->getHbm() : m_hbm);
	} else {
		if(fIgnoreGlyph)
			hdcSrc = m_hdc;
		else
			hdcSrc = isGlyph ? Skin->getGlyphItem()->getDC() : m_hdc;
		fCleanUp = FALSE;
	}

	if (m_dwFlags & IMAGE_FLAG_DIVIDED) {
		// top 3 items

		CMimAPI::m_MyAlphaBlend(hdc, rc->left, rc->top, l, t, hdcSrc, srcOrigX, srcOrigY, l, t, m_bf);
		CMimAPI::m_MyAlphaBlend(hdc, rc->left + l, rc->top, width - l - r, t, hdcSrc, srcOrigX + l, srcOrigY, m_inner_width, t, m_bf);
		CMimAPI::m_MyAlphaBlend(hdc, rc->right - r, rc->top, r, t, hdcSrc, srcOrigX + (m_width - r), srcOrigY, r, t, m_bf);

		// middle 3 items

		CMimAPI::m_MyAlphaBlend(hdc, rc->left, rc->top + t, l, height - t - b, hdcSrc, srcOrigX, srcOrigY + t, l, m_inner_height, m_bf);

		if ((m_dwFlags & IMAGE_FILLSOLID) && m_fillBrush) {
			RECT rcFill;
			rcFill.left = rc->left + l;
			rcFill.top = rc->top + t;
			rcFill.right = rc->right - r;
			rcFill.bottom = rc->bottom - b;
			FillRect(hdc, &rcFill, m_fillBrush);
		} else
			CMimAPI::m_MyAlphaBlend(hdc, rc->left + l, rc->top + t, width - l - r, height - t - b, hdcSrc, srcOrigX + l, srcOrigY + t, m_inner_width, m_inner_height, m_bf);

		CMimAPI::m_MyAlphaBlend(hdc, rc->right - r, rc->top + t, r, height - t - b, hdcSrc, srcOrigX + (m_width - r), srcOrigY + t, r, m_inner_height, m_bf);

		// bottom 3 items

		CMimAPI::m_MyAlphaBlend(hdc, rc->left, rc->bottom - b, l, b, hdcSrc, srcOrigX, srcOrigY + (m_height - b), l, b, m_bf);
		CMimAPI::m_MyAlphaBlend(hdc, rc->left + l, rc->bottom - b, width - l - r, b, hdcSrc, srcOrigX + l, srcOrigY + (m_height - b), m_inner_width, b, m_bf);
		CMimAPI::m_MyAlphaBlend(hdc, rc->right - r, rc->bottom - b, r, b, hdcSrc, srcOrigX + (m_width - r), srcOrigY + (m_height - b), r, b, m_bf);
	} else {
		switch (m_bStretch) {
			case IMAGE_STRETCH_H:
				// tile image vertically, stretch to width
			{
				LONG top = rc->top;

				do {
					if (top + m_height <= rc->bottom) {
						CMimAPI::m_MyAlphaBlend(hdc, rc->left, top, width, m_height, hdcSrc, srcOrigX, srcOrigY, m_width, m_height, m_bf);
						top += m_height;
					} else {
						CMimAPI::m_MyAlphaBlend(hdc, rc->left, top, width, rc->bottom - top, hdcSrc, srcOrigX, srcOrigY, m_width, rc->bottom - top, m_bf);
						break;
					}
				} while (TRUE);
				break;
			}
			case IMAGE_STRETCH_V:
				// tile horizontally, stretch to height
			{
				LONG left = rc->left;

				do {
					if (left + m_width <= rc->right) {
						CMimAPI::m_MyAlphaBlend(hdc, left, rc->top, m_width, height, hdcSrc, srcOrigX, srcOrigY, m_width, m_height, m_bf);
						left += m_width;
					} else {
						CMimAPI::m_MyAlphaBlend(hdc, left, rc->top, rc->right - left, height, hdcSrc, srcOrigX, srcOrigY, rc->right - left, m_height, m_bf);
						break;
					}
				} while (TRUE);
				break;
			}
			case IMAGE_STRETCH_B:
				// stretch the image in both directions...
				CMimAPI::m_MyAlphaBlend(hdc, rc->left, rc->top, width, height, hdcSrc, srcOrigX, srcOrigY, m_width, m_height, m_bf);
				break;
				/*
				case IMAGE_STRETCH_V:
				    // stretch vertically, draw 3 horizontal tiles...
				    AlphaBlend(hdc, rc->left, rc->top, l, height, item->hdc, 0, 0, l, item->height, item->bf);
				    AlphaBlend(hdc, rc->left + l, rc->top, width - l - r, height, item->hdc, l, 0, item->inner_width, item->height, item->bf);
				    AlphaBlend(hdc, rc->right - r, rc->top, r, height, item->hdc, item->width - r, 0, r, item->height, item->bf);
				    break;
				case IMAGE_STRETCH_H:
				    // stretch horizontally, draw 3 vertical tiles...
				    AlphaBlend(hdc, rc->left, rc->top, width, t, item->hdc, 0, 0, item->width, t, item->bf);
				    AlphaBlend(hdc, rc->left, rc->top + t, width, height - t - b, item->hdc, 0, t, item->width, item->inner_height, item->bf);
				    AlphaBlend(hdc, rc->left, rc->bottom - b, width, b, item->hdc, 0, item->height - b, item->width, b, item->bf);
				    break;
				*/
			default:
				break;
		}
	}
	if (fCleanUp) {
		SelectObject(hdcSrc, hbmOld);
		DeleteDC(hdcSrc);
	}
}

static CSkinItem StatusItem_Default = {
	_T("Container"), "EXBK_Offline", ID_EXTBKCONTAINER,
	CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
};

#if defined(_UNICODE)
	#define IMGL_LOAD 2			// IMGL_WCHAR, assume filename is wchar_t *
#else
	#define IMGL_LOAD 0
#endif

static HBITMAP LoadPNG(const TCHAR *szFilename)
{
	HBITMAP hBitmap = 0;
	hBitmap = (HBITMAP)CallService("IMG/Load", (WPARAM)szFilename,  IMGL_LOAD);
	CImageItem::PreMultiply(hBitmap, 1);
	return hBitmap;
}

static struct {
	TCHAR *szIniKey, *szIniName;
	char *szSetting;
	unsigned int size;
	int defaultval;
} _tagSettings[] = {
	_T("Global"), _T("SbarHeight"), "S_sbarheight", 1, 22,
	_T("ClientArea"), _T("Left"), "S_tborder_outer_left", 1, 0,
	_T("ClientArea"), _T("Right"), "S_tborder_outer_right", 1, 0,
	_T("ClientArea"), _T("Top"), "S_tborder_outer_top", 1, 0,
	_T("ClientArea"), _T("Bottom"), "S_tborder_outer_bottom", 1, 0,
	_T("ClientArea"), _T("Inner"), "S_tborder", 1, 0,
	_T("Global"), _T("TabTextNormal"), "S_tab_txt_normal", 5, 0,
	_T("Global"), _T("TabTextActive"), "S_tab_txt_active", 5, 0,
	_T("Global"), _T("TabTextUnread"), "S_tab_txt_unread", 5, 0,
	_T("Global"), _T("TabTextHottrack"), "S_tab_txt_hottrack", 5, 0,
	NULL, NULL, NULL, 0, 0
};

HBITMAP IMG_LoadLogo(const char *szFilename)
{
	HBITMAP hbm = (HBITMAP)CallService("IMG/Load", (LPARAM)szFilename, 0);
	CImageItem::PreMultiply(hbm, 1);
	return(hbm);
}

void CImageItem::Create(const TCHAR *szImageFile)
{
	HBITMAP hbm = ::LoadPNG(szImageFile);
	BITMAP bm;

	m_hdc = 0;
	m_hbmOld = 0;
	m_hbm = 0;

	if (hbm) {
		m_hbm = hbm;
		m_bf.BlendFlags = 0;
		m_bf.BlendOp = AC_SRC_OVER;
		m_bf.AlphaFormat = 0;

		GetObject(hbm, sizeof(bm), &bm);
		if (bm.bmBitsPixel == 32 && m_dwFlags & IMAGE_PERPIXEL_ALPHA) {
			CImageItem::PreMultiply(m_hbm, 1);
			m_bf.AlphaFormat = AC_SRC_ALPHA;
		}
		m_width = bm.bmWidth;
		m_height = bm.bmHeight;
		m_inner_height = m_height - m_bTop - m_bBottom;
		m_inner_width = m_width - m_bLeft - m_bRight;
		if (m_bTop && m_bBottom && m_bLeft && m_bRight) {
			m_dwFlags |= IMAGE_FLAG_DIVIDED;
			if (m_inner_height <= 0 || m_inner_width <= 0) {
				DeleteObject(hbm);
				m_hbm = 0;
				return;
			}
		}
	}
}

/**
 * Reads the definitions for an image item from the given .tsk file
 * It does _not_ create the image itself, a call to CImageItem::Create() must be done
 * to read the image in memory and prepare
 *
 * @param szFilename char*: full path and filename to the .TSK file
 *
 * @return char*: full path and filename to the .png image which represents this image item.
 *         caller MUST delete it.
 */
TCHAR* CImageItem::Read(const TCHAR *szFilename)
{
	TCHAR 	buffer[501];
	TCHAR 	szDrive[MAX_PATH], szPath[MAX_PATH];
	TCHAR	*szFinalName = 0;

	GetPrivateProfileString(m_szName, _T("Glyph"), _T("None"), buffer, 500, szFilename);
	if (_tcscmp(buffer, _T("None"))) {
		_stscanf(buffer, _T("%d,%d,%d,%d"), &m_glyphMetrics[0], &m_glyphMetrics[1],
			   &m_glyphMetrics[2], &m_glyphMetrics[3]);
		if (m_glyphMetrics[2] > m_glyphMetrics[0] && m_glyphMetrics[3] > m_glyphMetrics[1]) {
			m_dwFlags |= IMAGE_GLYPH;
			m_glyphMetrics[2] = (m_glyphMetrics[2] - m_glyphMetrics[0]) + 1;
			m_glyphMetrics[3] = (m_glyphMetrics[3] - m_glyphMetrics[1]) + 1;
		}
	}
	GetPrivateProfileString(m_szName, _T("Image"), _T("None"), buffer, 500, szFilename);
	if (_tcscmp(buffer, _T("None")) || m_dwFlags & IMAGE_GLYPH) {
		szFinalName = new TCHAR[MAX_PATH];
		//strncpy(m_szName, &m_szName[1], sizeof(m_szName));
		//m_szName[sizeof(m_szName) - 1] = 0;
		_tsplitpath(szFilename, szDrive, szPath, NULL, NULL);
		mir_sntprintf(szFinalName, MAX_PATH, _T("%s\\%s%s"), szDrive, szPath, buffer);
		if(!PathFileExists(szFinalName)) {
			delete szFinalName;
			szFinalName = 0;
		}
		m_alpha = GetPrivateProfileInt(m_szName, _T("Alpha"), 100, szFilename);
		m_alpha = min(m_alpha, 100);
		m_alpha = (BYTE)((FLOAT)(((FLOAT) m_alpha) / 100) * 255);
		m_bf.SourceConstantAlpha = m_alpha;
		m_bLeft = GetPrivateProfileInt(m_szName, _T("Left"), 0, szFilename);
		m_bRight = GetPrivateProfileInt(m_szName, _T("Right"), 0, szFilename);
		m_bTop = GetPrivateProfileInt(m_szName, _T("Top"), 0, szFilename);
		m_bBottom = GetPrivateProfileInt(m_szName, _T("Bottom"), 0, szFilename);
		if (m_dwFlags & IMAGE_GLYPH) {
			m_width = m_glyphMetrics[2];
			m_height = m_glyphMetrics[3];
			m_inner_height = m_glyphMetrics[3] - m_bTop - m_bBottom;
			m_inner_width = m_glyphMetrics[2] - m_bRight - m_bLeft;

			if (m_bTop && m_bBottom && m_bLeft && m_bRight)
				m_dwFlags |= IMAGE_FLAG_DIVIDED;
			m_bf.BlendFlags = 0;
			m_bf.BlendOp = AC_SRC_OVER;
			m_bf.AlphaFormat = 0;
			m_dwFlags |= IMAGE_PERPIXEL_ALPHA;
			m_bf.AlphaFormat = AC_SRC_ALPHA;
			if (m_inner_height <= 0 || m_inner_width <= 0) {
				if(szFinalName) {
					delete szFinalName;
					szFinalName = 0;
				}
				return(szFinalName);
			}
		}
		GetPrivateProfileString(m_szName, _T("Fillcolor"), _T("None"), buffer, 500, szFilename);
		if (_tcscmp(buffer, _T("None"))) {
			COLORREF fillColor = CSkin::HexStringToLong(buffer);
			m_fillBrush = CreateSolidBrush(fillColor);
			m_dwFlags |= IMAGE_FILLSOLID;
		} else
			m_fillBrush = 0;
		GetPrivateProfileString(m_szName, _T("Colorkey"), _T("None"), buffer, 500, szFilename);
		if (_tcscmp(buffer, _T("None"))) {
			CSkin::m_ContainerColorKey = CSkin::HexStringToLong(buffer);
			if (CSkin::m_ContainerColorKeyBrush)
				DeleteObject(CSkin::m_ContainerColorKeyBrush);
			CSkin::m_ContainerColorKeyBrush = CreateSolidBrush(CSkin::m_ContainerColorKey);
		}
		GetPrivateProfileString(m_szName, _T("Stretch"), _T("None"), buffer, 500, szFilename);
		if (buffer[0] == 'B' || buffer[0] == 'b')
			m_bStretch = IMAGE_STRETCH_B;
		else if (buffer[0] == 'h' || buffer[0] == 'H')
			m_bStretch = IMAGE_STRETCH_V;
		else if (buffer[0] == 'w' || buffer[0] == 'W')
			m_bStretch = IMAGE_STRETCH_H;
		m_hbm = 0;
		if (GetPrivateProfileInt(m_szName, _T("Perpixel"), 0, szFilename))
			m_dwFlags |= IMAGE_PERPIXEL_ALPHA;

		return(szFinalName);
	}
	return 0;
}

/**
 * Free all resources allocated by an image item
 */
void CImageItem::Free()
{
	if(m_hdc ) {
		::SelectObject(m_hdc, m_hbmOld);
		::DeleteDC(m_hdc);
	}
	if(m_hbm)
		::DeleteObject(m_hbm);

	if(m_fillBrush)
		::DeleteObject(m_fillBrush);

	ZeroMemory(this, sizeof(CImageItem));
}

void CImageItem::CorrectBitmap32Alpha(HBITMAP hBitmap)
{
	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;
	int x, y;
	BOOL fixIt = TRUE;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	if (bmp.bmBitsPixel != 32)
		return;

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	for (y = 0; y < bmp.bmHeight; ++y) {
		BYTE *px = p + bmp.bmWidth * 4 * y;

		for (x = 0; x < bmp.bmWidth; ++x) {
			if (px[3] != 0)
				fixIt = FALSE;
			else
				px[3] = 255;
			px += 4;
		}
	}

	if (fixIt)
		SetBitmapBits(hBitmap, bmp.bmWidth * bmp.bmHeight * 4, p);

	free(p);
}

void CImageItem::PreMultiply(HBITMAP hBitmap, int mode)
{
	BYTE *p = NULL;
	DWORD dwLen;
	int width, height, x, y;
	BITMAP bmp;
	BYTE alpha;

	GetObject(hBitmap, sizeof(bmp), &bmp);
	width = bmp.bmWidth;
	height = bmp.bmHeight;
	dwLen = width * height * 4;
	p = (BYTE *)malloc(dwLen);
	if (p) {
		GetBitmapBits(hBitmap, dwLen, p);
		for (y = 0; y < height; ++y) {
			BYTE *px = p + width * 4 * y;

			for (x = 0; x < width; ++x) {
				if (mode) {
					alpha = px[3];
					px[0] = px[0] * alpha / 255;
					px[1] = px[1] * alpha / 255;
					px[2] = px[2] * alpha / 255;
				} else
					px[3] = 255;
				px += 4;
			}
		}
		dwLen = SetBitmapBits(hBitmap, dwLen, p);
		free(p);
	}
}

/**
 * set filename and load parameters from the database
 * called on:
 * ) init
 * ) manual loading on user's request
 */
void CSkin::setFileName()
{
	DBVARIANT dbv;
	if(0 == M->GetTString(0, SRMSGMOD_T, "ContainerSkin", &dbv)) {
		M->pathToAbsolute(dbv.ptszVal,  m_tszFileName);
		m_tszFileName[MAX_PATH - 1] = 0;
		DBFreeVariant(&dbv);
	}
	else
		m_tszFileName[0] = 0;

	/*
	 * ANSI filename is kept for compatibility reasons. will go away at some time
	 */

#if defined(_UNICODE)
	if(m_tszFileName[0])
		WideCharToMultiByte(CP_ACP, 0, m_tszFileName, MAX_PATH, m_tszFileNameA, MAX_PATH, 0, 0);
#else
	lstrcpyn(m_tszFileNameA, m_tszFileName, MAX_PATH);
	m_tszFileNameA[MAX_PATH - 1] = 0;
#endif

	m_fLoadOnStartup = M->GetByte("useskin", 0) ? true : false;
}
/**
 * initialize the skin object
 */

void CSkin::Init()
{
	m_ImageItems = 0;
	ZeroMemory(this, sizeof(CSkin));
	m_SkinItems = ::SkinItems;
	m_fLoadOnStartup = false;
	m_skinEnabled = m_frameSkins = false;
	m_bAvatarBorderType = (BYTE)M->GetByte("avbordertype", 0);

	/*
	 * read current skin name from db
	 */

	setFileName();
}

bool CSkin::warnToClose() const
{
	if (::pFirstContainer) {
		if (MessageBox(0, TranslateT("All message containers need to close before the skin can be changed\nProceed?"), TranslateT("Change skin"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
			ContainerWindowData *pContainer = ::pFirstContainer;
			while (pFirstContainer)
				SendMessage(pFirstContainer->hwnd, WM_CLOSE, 0, 1);
			return true;
		} else
			return false;
	}
	return true;
}

/**
 * Unload the skin. Free everything allocated.
 * Called when:
 * * user unloads the skin from the dialog box
 * * a new skin is loaded by user's request.
 */
void CSkin::Unload()
{
	int i;
	CImageItem *tmp = m_ImageItems, *nextItem = 0;

	/*
	 * delete all image items
	 */

	if(warnToClose() == false)
		return;				  						// do nothing when user decides to not close any window

	m_skinEnabled = m_frameSkins = false;

	while(tmp) {
		nextItem = tmp->getNextItem();
		delete tmp;
		tmp = nextItem;
	}

	m_ImageItems = 0;
	m_glyphItem.Free();

	if(m_ContainerColorKeyBrush)
		::DeleteObject(m_ContainerColorKeyBrush);
	if(m_MenuBGBrush)
		::DeleteObject(m_MenuBGBrush);

	m_ContainerColorKeyBrush = m_MenuBGBrush = 0;

	if(m_SkinLightShadowPen)
		::DeleteObject(m_SkinLightShadowPen);
	m_SkinLightShadowPen = 0;

	if(m_SkinDarkShadowPen)
		::DeleteObject(m_SkinDarkShadowPen);
	m_SkinDarkShadowPen = 0;

	for(i = 0; i < ID_EXTBK_LAST; i++) {
		m_SkinItems[i].IGNORED = 1;
		m_SkinItems[i].imageItem = 0;
	}
	ZeroMemory(this, sizeof(CSkin));
	m_SkinItems = ::SkinItems;
	setFileName();

	m_bClipBorder = m_DisableScrollbars = false;
	m_SkinnedFrame_left = m_SkinnedFrame_right = m_SkinnedFrame_bottom = m_SkinnedFrame_caption = 0;
	m_realSkinnedFrame_left = m_realSkinnedFrame_right = m_realSkinnedFrame_bottom = m_realSkinnedFrame_caption = 0;

	m_titleBarLeftOff = m_titleButtonTopOff = m_captionOffset = m_captionPadding =
		m_titleBarRightOff = m_sidebarTopOffset = m_sidebarBottomOffset = m_bRoundedCorner = 0;

	m_titleBarButtonSize.cx = m_titleBarButtonSize.cy = 0;
	m_ContainerColorKey = 0;
	m_ContainerColorKeyBrush = m_MenuBGBrush = 0;
	m_skinEnabled = m_frameSkins = false;

	if(m_closeIcon)
		::DestroyIcon(m_closeIcon);
	if(m_maxIcon)
		::DestroyIcon(m_maxIcon);
	if(m_minIcon)
		::DestroyIcon(m_minIcon);

	m_closeIcon = m_maxIcon = m_minIcon = 0;

	for(i = 0; i < m_nrSkinIcons; i++) {
		if(m_skinIcons[i].phIcon )
			::DestroyIcon(*(m_skinIcons[i].phIcon));
	}
	M->getAeroState();				// refresh after unload
}

void CSkin::LoadIcon(const TCHAR *szSection, const TCHAR *name, HICON *hIcon)
{
	TCHAR buffer[512];
	if (*hIcon != 0)
		DestroyIcon(*hIcon);
	GetPrivateProfileString(szSection, name, _T("none"), buffer, 250, m_tszFileName);
	buffer[500] = 0;

	if (_tcsicmp(buffer, _T("none"))) {
		TCHAR szDrive[MAX_PATH], szDir[MAX_PATH], szImagePath[MAX_PATH];

		_tsplitpath(m_tszFileName, szDrive, szDir, NULL, NULL);
		mir_sntprintf(szImagePath, MAX_PATH, _T("%s\\%s\\%s"), szDrive, szDir, buffer);
		if (hIcon)
			*hIcon = (HICON)LoadImage(0, szImagePath, IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
	}
}


/**
 * Read a skin item (not a image item - these are handled elsewhere)
 * Reads all values from the .ini style .tsk file and fills the
 * structure.
 *
 * @param id     int: zero-based index into the table of predefined skin items
 * @param szItem char *: the section name in the ini file which holds the definition for this
 *               item.
 */
void CSkin::ReadItem(const int id, const TCHAR *szItem)
{
	TCHAR buffer[512];
	TCHAR def_color[20];
	COLORREF clr;
	CSkinItem *defaults = &StatusItem_Default;

	CSkinItem	*this_item = &m_SkinItems[id];

	this_item->ALPHA = (int)GetPrivateProfileInt(szItem, _T("Alpha"), defaults->ALPHA, m_tszFileName);
	this_item->ALPHA = min(this_item->ALPHA, 100);

	clr = RGB(GetBValue(defaults->COLOR), GetGValue(defaults->COLOR), GetRValue(defaults->COLOR));
	_sntprintf(def_color, 15, _T("%6.6x"), clr);
	GetPrivateProfileString(szItem, _T("Color1"), def_color, buffer, 400, m_tszFileName);
	this_item->COLOR = HexStringToLong(buffer);

	clr = RGB(GetBValue(defaults->COLOR2), GetGValue(defaults->COLOR2), GetRValue(defaults->COLOR2));
	_sntprintf(def_color, 15, _T("%6.6x"), clr);
	GetPrivateProfileString(szItem, _T("Color2"), def_color, buffer, 400, m_tszFileName);
	this_item->COLOR2 = HexStringToLong(buffer);

	this_item->COLOR2_TRANSPARENT = (BYTE)GetPrivateProfileInt(szItem, _T("COLOR2_TRANSPARENT"), defaults->COLOR2_TRANSPARENT, m_tszFileName);

	this_item->CORNER = defaults->CORNER & CORNER_ACTIVE ? defaults->CORNER : 0;
	GetPrivateProfileString(szItem, _T("Corner"), _T("None"), buffer, 400, m_tszFileName);
	if (_tcsstr(buffer, _T("tl")))
		this_item->CORNER |= CORNER_TL;
	if (_tcsstr(buffer, _T("tr")))
		this_item->CORNER |= CORNER_TR;
	if (_tcsstr(buffer, _T("bl")))
		this_item->CORNER |= CORNER_BL;
	if (_tcsstr(buffer, _T("br")))
		this_item->CORNER |= CORNER_BR;
	if (this_item->CORNER)
		this_item->CORNER |= CORNER_ACTIVE;

	this_item->GRADIENT = defaults->GRADIENT & GRADIENT_ACTIVE ?  defaults->GRADIENT : 0;
	GetPrivateProfileString(szItem, _T("Gradient"), _T("None"), buffer, 400, m_tszFileName);
	if (_tcsstr(buffer, _T("left")))
		this_item->GRADIENT = GRADIENT_RL;
	else if (_tcsstr(buffer, _T("right")))
		this_item->GRADIENT = GRADIENT_LR;
	else if (_tcsstr(buffer, _T("up")))
		this_item->GRADIENT = GRADIENT_BT;
	else if (_tcsstr(buffer, _T("down")))
		this_item->GRADIENT = GRADIENT_TB;
	if (this_item->GRADIENT)
		this_item->GRADIENT |= GRADIENT_ACTIVE;

	this_item->MARGIN_LEFT = GetPrivateProfileInt(szItem, _T("Left"), defaults->MARGIN_LEFT, m_tszFileName);
	this_item->MARGIN_RIGHT = GetPrivateProfileInt(szItem, _T("Right"), defaults->MARGIN_RIGHT, m_tszFileName);
	this_item->MARGIN_TOP = GetPrivateProfileInt(szItem, _T("Top"), defaults->MARGIN_TOP, m_tszFileName);
	this_item->MARGIN_BOTTOM = GetPrivateProfileInt(szItem, _T("Bottom"), defaults->MARGIN_BOTTOM, m_tszFileName);
	this_item->BORDERSTYLE = GetPrivateProfileInt(szItem, _T("Radius"), defaults->BORDERSTYLE, m_tszFileName);

	GetPrivateProfileString(szItem, _T("Textcolor"), _T("ffffffff"), buffer, 400, m_tszFileName);
	this_item->TEXTCOLOR = HexStringToLong(buffer);
	this_item->IGNORED = 0;
}

/**
 * stub to read a single item. Called from CSkin::LoadItems()
 * The real work is done by the CImageItem::Read().
 *
 * @param itemname char *: image item name, also section name in the .tsk file
 */
void CSkin::ReadImageItem(const TCHAR *itemname)
{
	TCHAR buffer[512], szItemNr[30];

	CImageItem tmpItem(itemname);

	TCHAR *szImageFileName = tmpItem.Read(m_tszFileName);

	if (!_tcsicmp(itemname, _T("$glyphs")) && szImageFileName != 0) {		// the glyph item MUST have a valid image
		tmpItem.Create(szImageFileName);
		if (tmpItem.getHbm()) {
			m_glyphItem = tmpItem;
			m_fHaveGlyph = true;
		}
		tmpItem.Clear();
		delete szImageFileName;
		return;
	}

	/*
	 * handle the assignments of image items to skin items
	 */

	for (int n = 0;;n++) {
		mir_sntprintf(szItemNr, 30, _T("Item%d"), n);
		GetPrivateProfileString(itemname, szItemNr, _T("None"), buffer, 500, m_tszFileName);
		if (!_tcscmp(buffer, _T("None")))
			break;
		for (int i = 0; i <= ID_EXTBK_LAST; i++) {
			if (!_tcsicmp(SkinItems[i].szName[0] == '{' ? &SkinItems[i].szName[3] : SkinItems[i].szName, buffer)) {
				if (!(tmpItem.getFlags() & IMAGE_GLYPH)) {
					if(szImageFileName)
						tmpItem.Create(szImageFileName);
					else {
						tmpItem.Clear();
						return;							// no reference to the glpyh image and no valid image file name -> invalid item
					}
				}
				if (tmpItem.getHbm() || (tmpItem.getFlags() & IMAGE_GLYPH)) {
					CImageItem *newItem = new CImageItem(tmpItem);
					SkinItems[i].imageItem = newItem;
					if (m_ImageItems == NULL)
						m_ImageItems = newItem;
					else {
						CImageItem *pItem = m_ImageItems;

						while (pItem->getNextItem() != 0)
							pItem = pItem->getNextItem();
						pItem->setNextItem(newItem);
					}
				}
			}
		}
	}
	tmpItem.Clear();
	if(szImageFileName)
		delete szImageFileName;
}

void CSkin::ReadButtonItem(const TCHAR *itemName) const
{
	ButtonItem tmpItem, *newItem;
	TCHAR szBuffer[1024];
	char  szBufferA[1024];
	CImageItem *imgItem = m_ImageItems;
	HICON *phIcon;

#if defined(_UNICODE)
	const char *szItemNameA = mir_u2a(itemName);
	const char *szFileNameA = m_tszFileNameA;
#else
	const char *szItemNameA = itemName;
	const char *szFileNameA = m_tszFileName;
#endif
	ZeroMemory(&tmpItem, sizeof(tmpItem));
	mir_snprintf(tmpItem.szName, safe_sizeof(tmpItem.szName), "%s", &szItemNameA[1]);
	tmpItem.width = GetPrivateProfileInt(itemName, _T("Width"), 16, m_tszFileName);
	tmpItem.height = GetPrivateProfileInt(itemName,_T( "Height"), 16, m_tszFileName);
	tmpItem.xOff = GetPrivateProfileInt(itemName, _T("xoff"), 0, m_tszFileName);
	tmpItem.yOff = GetPrivateProfileInt(itemName, _T("yoff"), 0, m_tszFileName);

	tmpItem.dwFlags |= GetPrivateProfileInt(itemName, _T("toggle"), 0, m_tszFileName) ? BUTTON_ISTOGGLE : 0;

	GetPrivateProfileString(itemName, _T("Pressed"), _T("None"), szBuffer, 1000, m_tszFileName);
	if (!_tcsicmp(szBuffer, _T("default")))
		tmpItem.imgPressed = SkinItems[ID_EXTBKBUTTONSPRESSED].imageItem;
	else {
		while (imgItem) {
			if (!_tcsicmp(imgItem->getName(), szBuffer)) {
				tmpItem.imgPressed = imgItem;
				break;
			}
			imgItem = imgItem->getNextItem();
		}
	}

	imgItem = m_ImageItems;
	GetPrivateProfileString(itemName, _T("Normal"), _T("None"), szBuffer, 1000, m_tszFileName);
	if (!_tcsicmp(szBuffer, _T("default")))
		tmpItem.imgNormal = SkinItems[ID_EXTBKBUTTONSNPRESSED].imageItem;
	else {
		while (imgItem) {
			if (!_tcsicmp(imgItem->getName(), szBuffer)) {
				tmpItem.imgNormal = imgItem;
				break;
			}
			imgItem = imgItem->getNextItem();
		}
	}

	imgItem = m_ImageItems;
	GetPrivateProfileString(itemName, _T("Hover"), _T("None"), szBuffer, 1000, m_tszFileName);
	if (!_tcsicmp(szBuffer, _T("default")))
		tmpItem.imgHover = SkinItems[ID_EXTBKBUTTONSMOUSEOVER].imageItem;
	else {
		while (imgItem) {
			if (!_tcsicmp(imgItem->getName(), szBuffer)) {
				tmpItem.imgHover = imgItem;
				break;
			}
			imgItem = imgItem->getNextItem();
		}
	}

	tmpItem.uId = IDC_TBFIRSTUID - 1;
	tmpItem.pfnAction = tmpItem.pfnCallback = NULL;

	if (GetPrivateProfileInt(itemName, _T("Sidebar"), 0, m_tszFileName)) {
		tmpItem.dwFlags |= BUTTON_ISSIDEBAR;
		PluginConfig.m_SideBarEnabled = TRUE;
		SIDEBARWIDTH = max(tmpItem.width + 2, SIDEBARWIDTH);
	}

	GetPrivateProfileString(itemName, _T("Action"), _T("Custom"), szBuffer, 1000, m_tszFileName);
	if (!_tcsicmp(szBuffer, _T("service"))) {
		tmpItem.szService[0] = 0;
		GetPrivateProfileStringA(szItemNameA, "Service", "None", szBufferA, 1000, szFileNameA);
		if (_stricmp(szBufferA, "None")) {
			mir_snprintf(tmpItem.szService, 256, "%s", szBufferA);
			tmpItem.dwFlags |= BUTTON_ISSERVICE;
			tmpItem.uId = nextButtonID++;
		}
	} else if (!_tcsicmp(szBuffer, _T("protoservice"))) {
		tmpItem.szService[0] = 0;
		GetPrivateProfileStringA(szItemNameA, "Service", "None", szBufferA, 1000, szFileNameA);
		if (_stricmp(szBufferA, "None")) {
			mir_snprintf(tmpItem.szService, 256, "%s", szBufferA);
			tmpItem.dwFlags |= BUTTON_ISPROTOSERVICE;
			tmpItem.uId = nextButtonID++;
		}
	} else if (!_tcsicmp(szBuffer, _T("database"))) {
		int n;

		GetPrivateProfileStringA(szItemNameA, "Module", "None", szBufferA, 1000, szFileNameA);
		if (_stricmp(szBufferA, "None"))
			mir_snprintf(tmpItem.szModule, 256, "%s", szBufferA);
		GetPrivateProfileStringA(szItemNameA, "Setting", "None", szBufferA, 1000, szFileNameA);
		if (_stricmp(szBufferA, "None"))
			mir_snprintf(tmpItem.szSetting, 256, "%s", szBufferA);
		if (GetPrivateProfileIntA(szItemNameA, "contact", 0, szFileNameA) != 0)
			tmpItem.dwFlags |= BUTTON_DBACTIONONCONTACT;

		for (n = 0; n <= 1; n++) {
			char szKey[20];
			BYTE *pValue;

			strcpy(szKey, n == 0 ? "dbonpush" : "dbonrelease");
			pValue = (n == 0 ? tmpItem.bValuePush : tmpItem.bValueRelease);

			GetPrivateProfileStringA(szItemNameA, szKey, "None", szBufferA, 1000, szFileNameA);
			switch (szBufferA[0]) {
				case 'b': {
					BYTE value = (BYTE)atol(&szBufferA[1]);
					pValue[0] = value;
					tmpItem.type = DBVT_BYTE;
					break;
				}
				case 'w': {
					WORD value = (WORD)atol(&szBufferA[1]);
					*((WORD *)&pValue[0]) = value;
					tmpItem.type = DBVT_WORD;
					break;
				}
				case 'd': {
					DWORD value = (DWORD)atol(&szBufferA[1]);
					*((DWORD *)&pValue[0]) = value;
					tmpItem.type = DBVT_DWORD;
					break;
				}
				case 's': {
					mir_snprintf((char *)pValue, 256, &szBufferA[1]);
					tmpItem.type = DBVT_ASCIIZ;
					break;
				}
			}
		}
		if (tmpItem.szModule[0] && tmpItem.szSetting[0]) {
			tmpItem.dwFlags |= BUTTON_ISDBACTION;
			if (tmpItem.szModule[0] == '$' && (tmpItem.szModule[1] == 'c' || tmpItem.szModule[1] == 'C'))
				tmpItem.dwFlags |= BUTTON_ISCONTACTDBACTION;
			tmpItem.uId = nextButtonID++;
		}
	} else if (_tcsicmp(szBuffer, _T("Custom"))) {
		if (BTN_GetStockItem(&tmpItem, szBuffer))
			goto create_it;
	}
	GetPrivateProfileString(itemName, _T("PassContact"), _T("None"), szBuffer, 1000, m_tszFileName);
	if (_tcsicmp(szBuffer, _T("None"))) {
		if (szBuffer[0] == 'w' || szBuffer[0] == 'W')
			tmpItem.dwFlags |= BUTTON_PASSHCONTACTW;
		else if (szBuffer[0] == 'l' || szBuffer[0] == 'L')
			tmpItem.dwFlags |= BUTTON_PASSHCONTACTL;
	}

	GetPrivateProfileString(itemName, _T("Tip"), _T("None"), szBuffer, 1000, m_tszFileName);
	if (_tcsicmp(szBuffer, _T("None"))) {
		mir_sntprintf(tmpItem.szTip, 256, _T("%s"), szBuffer);
	} else
		tmpItem.szTip[0] = 0;

create_it:

	GetPrivateProfileString(itemName, _T("Label"), _T("None"), szBuffer, 40, m_tszFileName);
	if (_tcsicmp(szBuffer, _T("None"))) {
		mir_sntprintf(tmpItem.tszLabel, 40, _T("%s"), szBuffer);
		tmpItem.dwFlags |= BUTTON_HASLABEL;
	} else
		tmpItem.tszLabel[0] = 0;

	GetPrivateProfileString(itemName, _T("NormalGlyph"), _T("0, 0, 0, 0"), szBuffer, 1000, m_tszFileName);
	if (_tcsicmp(szBuffer, _T("default"))) {
		tmpItem.dwFlags &= ~BUTTON_NORMALGLYPHISICON;
		if ((phIcon = BTN_GetIcon(szBuffer)) != 0) {
			tmpItem.dwFlags |= BUTTON_NORMALGLYPHISICON;
			tmpItem.normalGlyphMetrics[0] = (LONG_PTR)phIcon;
		} else {
			_tscanf(szBuffer, _T("%d,%d,%d,%d"), &tmpItem.normalGlyphMetrics[0], &tmpItem.normalGlyphMetrics[1],
				   &tmpItem.normalGlyphMetrics[2], &tmpItem.normalGlyphMetrics[3]);
			tmpItem.normalGlyphMetrics[2] = (tmpItem.normalGlyphMetrics[2] - tmpItem.normalGlyphMetrics[0]) + 1;
			tmpItem.normalGlyphMetrics[3] = (tmpItem.normalGlyphMetrics[3] - tmpItem.normalGlyphMetrics[1]) + 1;
		}
	}

	GetPrivateProfileString(itemName, _T("PressedGlyph"), _T("0, 0, 0, 0"), szBuffer, 1000, m_tszFileName);
	if (_tcsicmp(szBuffer, _T("default"))) {
		tmpItem.dwFlags &= ~BUTTON_PRESSEDGLYPHISICON;
		if ((phIcon = BTN_GetIcon(szBuffer)) != 0) {
			tmpItem.pressedGlyphMetrics[0] = (LONG_PTR)phIcon;
			tmpItem.dwFlags |= BUTTON_PRESSEDGLYPHISICON;
		} else {
			_tscanf(szBuffer, _T("%d,%d,%d,%d"), &tmpItem.pressedGlyphMetrics[0], &tmpItem.pressedGlyphMetrics[1],
				   &tmpItem.pressedGlyphMetrics[2], &tmpItem.pressedGlyphMetrics[3]);
			tmpItem.pressedGlyphMetrics[2] = (tmpItem.pressedGlyphMetrics[2] - tmpItem.pressedGlyphMetrics[0]) + 1;
			tmpItem.pressedGlyphMetrics[3] = (tmpItem.pressedGlyphMetrics[3] - tmpItem.pressedGlyphMetrics[1]) + 1;
		}
	}

	GetPrivateProfileString(itemName, _T("HoverGlyph"), _T("0, 0, 0, 0"), szBuffer, 1000, m_tszFileName);
	if (_tcsicmp(szBuffer, _T("default"))) {
		tmpItem.dwFlags &= ~BUTTON_HOVERGLYPHISICON;
		if ((phIcon = BTN_GetIcon(szBuffer)) != 0) {
			tmpItem.hoverGlyphMetrics[0] = (LONG_PTR)phIcon;
			tmpItem.dwFlags |= BUTTON_HOVERGLYPHISICON;
		} else {
			_tscanf(szBuffer, _T("%d,%d,%d,%d"), &tmpItem.hoverGlyphMetrics[0], &tmpItem.hoverGlyphMetrics[1],
				   &tmpItem.hoverGlyphMetrics[2], &tmpItem.hoverGlyphMetrics[3]);
			tmpItem.hoverGlyphMetrics[2] = (tmpItem.hoverGlyphMetrics[2] - tmpItem.hoverGlyphMetrics[0]) + 1;
			tmpItem.hoverGlyphMetrics[3] = (tmpItem.hoverGlyphMetrics[3] - tmpItem.hoverGlyphMetrics[1]) + 1;
		}
	}

	newItem = (ButtonItem *)malloc(sizeof(ButtonItem));
	ZeroMemory(newItem, sizeof(ButtonItem));
	if (g_ButtonSet.items == NULL) {
		g_ButtonSet.items = newItem;
		*newItem = tmpItem;
		newItem->nextItem = 0;
	} else {
		ButtonItem *curItem = g_ButtonSet.items;
		while (curItem->nextItem)
			curItem = curItem->nextItem;
		*newItem = tmpItem;
		newItem->nextItem = 0;
		curItem->nextItem = newItem;
	}
#ifdef _UNICODE
	mir_free((void *)szItemNameA);
#endif
	return;
}

/**
 * Load the skin from the .tsk file
 * It reads and initializes all static values for the skin. Afterwards
 * it calls ReadItems() to read additional skin information like image items,
 * buttons and icons.
 */
void CSkin::Load()
{
	if(warnToClose() == false)
		return;

	if(m_skinEnabled) {
		Unload();
		m_skinEnabled = false;
	}

	m_bAvatarBorderType = (BYTE)M->GetByte("avbordertype", 0);

	m_fHaveGlyph = false;

	if(m_tszFileName[0]) {
		if(::PathFileExists(m_tszFileName)) {
			TCHAR *p;
			TCHAR *szSections = (TCHAR *)malloc(6004);
			int i = 1, j = 0;
			UINT  data;
			TCHAR buffer[500];

			if (!(GetPrivateProfileInt(_T("Global"), _T("Version"), 0, m_tszFileName) >= 1 &&
				  GetPrivateProfileInt(_T("Global"), _T("Signature"), 0, m_tszFileName) == 101))
				return;

			i = 0;
			while (_tagSettings[i].szIniKey != NULL) {
				data = 0;
				data = GetPrivateProfileInt(_tagSettings[i].szIniKey, _tagSettings[i].szIniName,
											_tagSettings[i].defaultval, m_tszFileName);
				switch (_tagSettings[i].size) {
					case 1:
						M->WriteByte(SRMSGMOD_T, _tagSettings[i].szSetting, (BYTE)data);
						break;
					case 4:
						M->WriteDword(SRMSGMOD_T, _tagSettings[i].szSetting, data);
						break;
					case 2:
						DBWriteContactSettingWord(NULL, SRMSGMOD_T, _tagSettings[i].szSetting, (WORD)data);
						break;
					case 5:
						GetPrivateProfileString(_tagSettings[i].szIniKey, _tagSettings[i].szIniName, _T("000000"),
											    buffer, 10, m_tszFileName);
						M->WriteDword(SRMSGMOD_T, _tagSettings[i].szSetting, HexStringToLong(buffer));
						break;
				}
				i++;
			}

			m_DisableScrollbars = false;

			ZeroMemory(szSections, 3000);
			p = szSections;
			GetPrivateProfileSectionNames(szSections, 3000, m_tszFileName);
			szSections[3001] = szSections[3000] = 0;
			p = szSections;
			while (lstrlen(p) > 1) {
				if (p[0] != '%') {
					p += (lstrlen(p) + 1);
					continue;
				}
				for (i = 0; i <= ID_EXTBK_LAST; i++) {
					if (!_tcsicmp(&p[1], SkinItems[i].szName[0] == '{' ? &SkinItems[i].szName[3] : SkinItems[i].szName)) {
						ReadItem(i, p);
						break;
					}
				}
				p += (lstrlen(p) + 1);
				j++;
			}

			if (j > 0) {
				m_skinEnabled = true;
				M->getAeroState();		// refresh aero state (set to false when a skin is successfully loaded and active)
			}

			GetPrivateProfileString(_T("Avatars"), _T("BorderColor"), _T("000000"), buffer, 20, m_tszFileName);
			M->WriteDword(SRMSGMOD_T, "avborderclr", HexStringToLong(buffer));

			LoadIcon(_T("Global"), _T("CloseGlyph"), &CSkin::m_closeIcon);
			LoadIcon(_T("Global"), _T("MaximizeGlyph"), &CSkin::m_maxIcon);
			LoadIcon(_T("Global"), _T("MinimizeGlyph"), &CSkin::m_minIcon);

			m_frameSkins = GetPrivateProfileInt(_T("Global"), _T("framelessmode"), 0, m_tszFileName) ? true : false;
			m_DisableScrollbars = GetPrivateProfileInt(_T("Global"), _T("NoScrollbars"), 0, m_tszFileName) ? true : false;

			data = GetPrivateProfileInt(_T("Global"), _T("SkinnedTabs"), 1, m_tszFileName);
			PluginConfig.m_TabAppearance = data ? PluginConfig.m_TabAppearance | TCF_NOSKINNING : PluginConfig.m_TabAppearance & ~TCF_NOSKINNING;
			M->WriteDword(SRMSGMOD_T, "tabconfig", PluginConfig.m_TabAppearance);

			m_SkinnedFrame_left = GetPrivateProfileInt(_T("WindowFrame"), _T("left"), 4, m_tszFileName);
			m_SkinnedFrame_right = GetPrivateProfileInt(_T("WindowFrame"), _T("right"), 4, m_tszFileName);
			m_SkinnedFrame_caption = GetPrivateProfileInt(_T("WindowFrame"), _T("Caption"), 24, m_tszFileName);
			m_SkinnedFrame_bottom = GetPrivateProfileInt(_T("WindowFrame"), _T("bottom"), 4, m_tszFileName);

			m_titleBarButtonSize.cx = GetPrivateProfileInt(_T("WindowFrame"), _T("TitleButtonWidth"), 24, m_tszFileName);
			m_titleBarButtonSize.cy = GetPrivateProfileInt(_T("WindowFrame"), _T("TitleButtonHeight"), 12, m_tszFileName);
			m_titleButtonTopOff = GetPrivateProfileInt(_T("WindowFrame"), _T("TitleButtonTopOffset"), 0, m_tszFileName);

			m_titleBarRightOff = GetPrivateProfileInt(_T("WindowFrame"), _T("TitleBarRightOffset"), 0, m_tszFileName);
			m_titleBarLeftOff = GetPrivateProfileInt(_T("WindowFrame"), _T("TitleBarLeftOffset"), 0, m_tszFileName);

			m_captionOffset = GetPrivateProfileInt(_T("WindowFrame"), _T("CaptionOffset"), 3, m_tszFileName);
			m_captionPadding = GetPrivateProfileInt(_T("WindowFrame"), _T("CaptionPadding"), 0, m_tszFileName);
			m_sidebarTopOffset = GetPrivateProfileInt(_T("ClientArea"), _T("SidebarTop"), -1, m_tszFileName);
			m_sidebarBottomOffset = GetPrivateProfileInt(_T("ClientArea"), _T("SidebarBottom"), -1, m_tszFileName);

			m_bClipBorder = GetPrivateProfileInt(_T("WindowFrame"), _T("ClipFrame"), 0, m_tszFileName) ? true : false;;

			BYTE radius_tl, radius_tr, radius_bl, radius_br;
			char szFinalName[MAX_PATH];
			char szDrive[MAX_PATH], szPath[MAX_PATH];
			char bufferA[MAX_PATH];

			radius_tl = GetPrivateProfileInt(_T("WindowFrame"), _T("RadiusTL"), 0, m_tszFileName);
			radius_tr = GetPrivateProfileInt(_T("WindowFrame"), _T("RadiusTR"), 0, m_tszFileName);
			radius_bl = GetPrivateProfileInt(_T("WindowFrame"), _T("RadiusBL"), 0, m_tszFileName);
			radius_br = GetPrivateProfileInt(_T("WindowFrame"), _T("RadiusBR"), 0, m_tszFileName);

			CSkin::m_bRoundedCorner = radius_tl;

			GetPrivateProfileStringA("Theme", "File", "None", bufferA, MAX_PATH, m_tszFileNameA);

			_splitpath(m_tszFileNameA, szDrive, szPath, NULL, NULL);
			mir_snprintf(szFinalName, MAX_PATH, "%s\\%s\\%s", szDrive, szPath, bufferA);
			if (PathFileExistsA(szFinalName)) {
				ReadThemeFromINI(szFinalName, 0, FALSE, m_fLoadOnStartup ? 0 : M->GetByte("skin_loadmode", 0));
				CacheLogFonts();
				CacheMsgLogIcons();
			}

			GetPrivateProfileString(_T("Global"), _T("MenuBarBG"), _T("None"), buffer, 20, m_tszFileName);
			data = HexStringToLong(buffer);
			if (m_MenuBGBrush) {
				DeleteObject(m_MenuBGBrush);
				m_MenuBGBrush = 0;
			}
			if (_tcscmp(buffer, _T("None")))
				m_MenuBGBrush = CreateSolidBrush(data);

			GetPrivateProfileString(_T("Global"), _T("LightShadow"), _T("000000"), buffer, 20, m_tszFileName);
			data = HexStringToLong(buffer);
			CSkin::m_SkinLightShadowPen = CreatePen(PS_SOLID, 1, RGB(GetRValue(data), GetGValue(data), GetBValue(data)));
			GetPrivateProfileString(_T("Global"), _T("DarkShadow"), _T("000000"), buffer, 20, m_tszFileName);
			data = HexStringToLong(buffer);
			CSkin::m_SkinDarkShadowPen = CreatePen(PS_SOLID, 1, RGB(GetRValue(data), GetGValue(data), GetBValue(data)));

			SkinCalcFrameWidth();

			GetPrivateProfileString(_T("Global"), _T("FontColor"), _T("None"), buffer, 20, m_tszFileName);
			if (_tcscmp(buffer, _T("None")))
				PluginConfig.skinDefaultFontColor = HexStringToLong(buffer);
			else
				PluginConfig.skinDefaultFontColor = GetSysColor(COLOR_BTNTEXT);
			buffer[499] = 0;
			free(szSections);

			LoadItems();
		}
	}
}

/**
 * Load additional skin items (like image items, buttons, icons etc.)
 * This is called AFTER ReadItems() has read the basic skin items
 */
void CSkin::LoadItems()
{
	TCHAR *szSections = NULL;
	TCHAR *p, *p1;
	ICONDESC tmpIconDesc = {0};

	CImageItem *pItem = m_ImageItems;

	if (m_skinIcons == NULL)
		m_skinIcons = (ICONDESCW *)malloc(sizeof(ICONDESCW) * NR_MAXSKINICONS);

	ZeroMemory(m_skinIcons, sizeof(ICONDESC) * NR_MAXSKINICONS);
	m_nrSkinIcons = 0;

	szSections = (TCHAR *)malloc(5004);
	ZeroMemory(szSections, 5004);

	GetPrivateProfileSection(_T("Icons"), szSections, 2500, m_tszFileName);
	p = szSections;
	while (lstrlen(p) > 1) {
		p1 = _tcschr(p, (int)'=');
		if (p1)
			*p1 = 0;
		if (m_nrSkinIcons < NR_MAXSKINICONS && p1) {
			LoadIcon(_T("Icons"), p, (HICON *)&tmpIconDesc.uId);
			if (tmpIconDesc.uId) {
				ZeroMemory(&m_skinIcons[m_nrSkinIcons], sizeof(ICONDESC));
				m_skinIcons[m_nrSkinIcons].uId = tmpIconDesc.uId;
				m_skinIcons[m_nrSkinIcons].phIcon = (HICON *)(&m_skinIcons[m_nrSkinIcons].uId);
				m_skinIcons[m_nrSkinIcons].szName = (TCHAR *)malloc(sizeof(TCHAR) * (lstrlen(p) + 1));
				lstrcpy(m_skinIcons[m_nrSkinIcons].szName, p);
				m_nrSkinIcons++;
			}
		}
		if (p1)
			*p1 = '=';
		p += (lstrlen(p) + 1);
	}

	ZeroMemory(szSections, 5004);
	p = szSections;
	GetPrivateProfileSectionNames(szSections, 2500, m_tszFileName);

	szSections[2500] = szSections[2501] = 0;
	p = szSections;
	while (lstrlen(p) > 1) {
		if (p[0] == '$')
			ReadImageItem(p);
		p += (lstrlen(p) + 1);
	}
	nextButtonID = IDC_TBFIRSTUID;
	SIDEBARWIDTH = DEFAULT_SIDEBARWIDTH;

	p = szSections;
	while (lstrlen(p) > 1) {
		if (p[0] == '!')
		 	ReadButtonItem(p);
		p += (lstrlen(p) + 1);
	}
	free(szSections);
	g_ButtonSet.top = GetPrivateProfileInt(_T("ButtonArea"), _T("top"), 0, m_tszFileName);
	g_ButtonSet.bottom = GetPrivateProfileInt(_T("ButtonArea"), _T("bottom"), 0, m_tszFileName);
	g_ButtonSet.left = GetPrivateProfileInt(_T("ButtonArea"), _T("left"), 0, m_tszFileName);
	g_ButtonSet.right = GetPrivateProfileInt(_T("ButtonArea"), _T("right"), 0, m_tszFileName);
}

/**
 * Calculate window frame borders for a skin with the ability to paint the window frame.
 * Uses system metrics to determine predefined window borders and caption bar size.
 */
void CSkin::SkinCalcFrameWidth()
{
	int xBorder, yBorder, yCaption;

	xBorder = GetSystemMetrics(SM_CXSIZEFRAME);
	yBorder = GetSystemMetrics(SM_CYSIZEFRAME);
	yCaption = GetSystemMetrics(SM_CYCAPTION);

	m_realSkinnedFrame_left = m_SkinnedFrame_left - xBorder;
	m_realSkinnedFrame_right = m_SkinnedFrame_right - xBorder;
	m_realSkinnedFrame_bottom = m_SkinnedFrame_bottom - yBorder;
	m_realSkinnedFrame_caption = m_SkinnedFrame_caption - yCaption;
}


/**
 * Draws part of the background to the foreground control
 *
 * @param hwndClient HWND: target window
 * @param hwnd       HWND: source window (usually the parent, needed for transforming client coordinates
 * @param pContainer ContainerWindowData *: needed to access the cached DC of the container window
 * @param rcClient   RECT *: client rectangle (target area)
 * @param hdcTarget  HDC: device context of the target window
 */
void CSkin::SkinDrawBG(HWND hwndClient, HWND hwnd, struct ContainerWindowData *pContainer, RECT *rcClient, HDC hdcTarget)
{
	RECT rcWindow;
	POINT pt;
	LONG width = rcClient->right - rcClient->left;
	LONG height = rcClient->bottom - rcClient->top;
	HDC dc;

	::GetWindowRect(hwndClient, &rcWindow);
	pt.x = rcWindow.left + rcClient->left;
	pt.y = rcWindow.top + rcClient->top;
	::ScreenToClient(hwnd, &pt);
	if (pContainer)
		dc = pContainer->cachedDC;
	else
		dc = ::GetDC(hwnd);
	::BitBlt(hdcTarget, rcClient->left, rcClient->top, width, height, dc, pt.x, pt.y, SRCCOPY);
	if (!pContainer)
		::ReleaseDC(hwnd, dc);
}

/**
 * Draws part of the background to the foreground control
 * same as above, but can use any source DC, not just the
 * container
 *
 * @param hwndClient HWND: target window
 * @param hwnd       HWND: source window (usually the parent, needed for transforming client coordinates
 * @param pContainer ContainerWindowData *: needed to access the cached DC of the container window
 * @param rcClient   RECT *: client rectangle (target area)
 * @param hdcTarget  HDC: device context of the target window
 */

void CSkin::SkinDrawBGFromDC(HWND hwndClient, HWND hwnd, HDC hdcSrc, RECT *rcClient, HDC hdcTarget)
{
	RECT rcWindow;
	POINT pt;
	LONG width = rcClient->right - rcClient->left;
	LONG height = rcClient->bottom - rcClient->top;

	::GetWindowRect(hwndClient, &rcWindow);
	pt.x = rcWindow.left + rcClient->left;
	pt.y = rcWindow.top + rcClient->top;
	::ScreenToClient(hwnd, &pt);
	::BitBlt(hdcTarget, rcClient->left, rcClient->top, width, height, hdcSrc, pt.x, pt.y, SRCCOPY);
}

/*
 * draw transparent avatar image. Get around crappy image rescaling quality of the
 * AlphaBlend() API.
 *
 * hdcMem contains the bitmap to draw (must be premultiplied for proper per-pixel alpha
 * rendering in AlphaBlend().
 */

void CSkin::MY_AlphaBlend(HDC hdcDraw, DWORD left, DWORD top,  int width, int height, int bmWidth, int bmHeight, HDC hdcMem)
{
	HDC hdcTemp = CreateCompatibleDC(hdcDraw);
	HBITMAP hbmTemp = CreateCompatibleBitmap(hdcMem, bmWidth, bmHeight);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcTemp, hbmTemp);

	SetStretchBltMode(hdcTemp, HALFTONE);
	StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, hdcDraw, left, top, width, height, SRCCOPY);
	if (CMimAPI::m_MyAlphaBlend)
		CMimAPI::m_MyAlphaBlend(hdcTemp, 0, 0, bmWidth, bmHeight, hdcMem, 0, 0, bmWidth, bmHeight, CSkin::m_default_bf);
	else {
		SetStretchBltMode(hdcTemp, HALFTONE);
		StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, hdcMem, 0, 0, bmWidth, bmHeight, SRCCOPY);
	}
	StretchBlt(hdcDraw, left, top, width, height, hdcTemp, 0, 0, bmWidth, bmHeight, SRCCOPY);
	SelectObject(hdcTemp, hbmOld);
	DeleteObject(hbmTemp);
	DeleteDC(hdcTemp);
}

/*
 * draw an icon "dimmed" (small amount of transparency applied)
*/

static BLENDFUNCTION bf_t = {0};

void CSkin::DrawDimmedIcon(HDC hdc, LONG left, LONG top, LONG dx, LONG dy, HICON hIcon, BYTE alpha)
{
	HDC dcMem = CreateCompatibleDC(hdc);
	HBITMAP hbm = CreateCompatibleBitmap(hdc, dx, dy), hbmOld = 0;

	hbmOld = (HBITMAP)SelectObject(dcMem, hbm);
	BitBlt(dcMem, 0, 0, dx, dx, hdc, left, top, SRCCOPY);
	DrawIconEx(dcMem, 0, 0, hIcon, dx, dy, 0, 0, DI_NORMAL);
	bf_t.SourceConstantAlpha = alpha;
	if (CMimAPI::m_MyAlphaBlend)
		CMimAPI::m_MyAlphaBlend(hdc, left, top, dx, dy, dcMem, 0, 0, dx, dy, bf_t);
	else {
		SetStretchBltMode(hdc, HALFTONE);
		StretchBlt(hdc, left, top, dx, dy, dcMem, 0, 0, dx, dy, SRCCOPY);
	}
	SelectObject(dcMem, hbmOld);
	DeleteObject(hbm);
	DeleteDC(dcMem);
}

UINT CSkin::NcCalcRichEditFrame(HWND hwnd, const _MessageWindowData *mwdat, UINT skinID, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC OldWndProc)
{
	LRESULT orig;
	NCCALCSIZE_PARAMS *nccp = (NCCALCSIZE_PARAMS *)lParam;
	BOOL bReturn = FALSE;

	if (CSkin::m_DisableScrollbars) {
		SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) & ~WS_VSCROLL);
		EnableScrollBar(hwnd, SB_VERT, ESB_DISABLE_BOTH);
		ShowScrollBar(hwnd, SB_VERT, FALSE);
	}
	orig = CallWindowProc(OldWndProc, hwnd, msg, wParam, lParam);

	if (mwdat && mwdat->pContainer->bSkinned && !mwdat->bFlatMsgLog) {
		CSkinItem *item = &SkinItems[skinID];
		if (!item->IGNORED) {
			/*
				nccp->rgrc[0].left += item->MARGIN_LEFT;
				nccp->rgrc[0].right -= item->MARGIN_RIGHT;
				nccp->rgrc[0].bottom -= item->MARGIN_BOTTOM;
				nccp->rgrc[0].top += item->MARGIN_TOP;
			*/
			return WVR_REDRAW;
		}
	}
	if (mwdat && mwdat->hTheme && wParam && CMimAPI::m_pfnGetThemeBackgroundContentRect) {
		RECT rcClient;
		HDC hdc = GetDC(GetParent(hwnd));

		if (CMimAPI::m_pfnGetThemeBackgroundContentRect(mwdat->hTheme, hdc, 1, 1, &nccp->rgrc[0], &rcClient) == S_OK) {
			if (EqualRect(&rcClient, &nccp->rgrc[0]))
				InflateRect(&rcClient, -1, -1);
			CopyRect(&nccp->rgrc[0], &rcClient);
			bReturn = TRUE;
		}
		ReleaseDC(GetParent(hwnd), hdc);
		if (bReturn)
			return WVR_REDRAW;
		else
			return orig;
	}
	if (mwdat && (mwdat->sendMode & SMODE_MULTIPLE || mwdat->sendMode & SMODE_CONTAINER) && skinID == ID_EXTBKINPUTAREA) {
		InflateRect(&nccp->rgrc[0], -1, -1);
		return WVR_REDRAW;
	}
	return orig;
}

/*
 * process WM_NCPAINT for the rich edit control. Draw a visual style border and avoid classic static edge / client edge
 * may also draw a skin item around the rich edit control.
 */

UINT CSkin::DrawRichEditFrame(HWND hwnd, const _MessageWindowData *mwdat, UINT skinID, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC OldWndProc)
{
	CSkinItem *item = &SkinItems[skinID];
	LRESULT result = 0;
	BOOL isMultipleReason;

	result = CallWindowProc(OldWndProc, hwnd, msg, wParam, lParam);			// do default processing (otherwise, NO scrollbar as it is painted in NC_PAINT)
	if (!mwdat)
		return result;

	isMultipleReason = ((skinID == ID_EXTBKINPUTAREA) && (mwdat->sendMode & SMODE_MULTIPLE || mwdat->sendMode & SMODE_CONTAINER));

	if (isMultipleReason || ((mwdat && mwdat->hTheme) || (mwdat && mwdat->pContainer->bSkinned && !item->IGNORED && !mwdat->bFlatMsgLog))) {
		HDC hdc = GetWindowDC(hwnd);
		RECT rcWindow;
		POINT pt;
		LONG left_off, top_off, right_off, bottom_off;
		LONG dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		LONG dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

		GetWindowRect(hwnd, &rcWindow);
		pt.x = pt.y = 0;
		ClientToScreen(hwnd, &pt);
		left_off = pt.x - rcWindow.left;
		if (dwStyle & WS_VSCROLL && dwExStyle & WS_EX_RTLREADING)
			left_off -= PluginConfig.ncm.iScrollWidth;
		top_off = pt.y - rcWindow.top;

		if (mwdat->pContainer->bSkinned && !item->IGNORED) {
			right_off = item->MARGIN_RIGHT;
			bottom_off = item->MARGIN_BOTTOM;
		} else {
			right_off = left_off;
			bottom_off = top_off;
		}

		rcWindow.right -= rcWindow.left;
		rcWindow.bottom -= rcWindow.top;
		rcWindow.left = rcWindow.top = 0;

		ExcludeClipRect(hdc, left_off, top_off, rcWindow.right - right_off, rcWindow.bottom - bottom_off);
		if (mwdat->pContainer->bSkinned && !item->IGNORED) {
			ReleaseDC(hwnd, hdc);
			return result;
		} else if (CMimAPI::m_pfnDrawThemeBackground) {
			if (isMultipleReason) {
				HBRUSH br = CreateSolidBrush(RGB(255, 130, 130));
				FillRect(hdc, &rcWindow, br);
				DeleteObject(br);
			} else
				CMimAPI::m_pfnDrawThemeBackground(mwdat->hTheme, hdc, 1, 1, &rcWindow, &rcWindow);
		}
		ReleaseDC(hwnd, hdc);
		return result;
	}
	return result;
}

/**
 * convert a html-style color string (without the #) to a 32bit COLORREF value
 *
 * @param szSource TCHAR*: the color value as string. format:
 *  			   html-style without the leading #. e.g.
 *  			   "f3e355"
 *
 * @return COLORREF representation of the string value.
 */
DWORD __fastcall CSkin::HexStringToLong(const TCHAR *szSource)
{
	TCHAR *stopped;
	COLORREF clr = _tcstol(szSource, &stopped, 16);
	if (clr == -1)
		return clr;
	return(RGB(GetBValue(clr), GetGValue(clr), GetRValue(clr)));
}

/**
 * Render text to the given HDC. This function is aero aware and will use uxtheme DrawThemeTextEx() when needed.
 * Paramaters are pretty much comparable to GDI DrawText() API
 *
 * @return
 */
#if defined(_UNICODE)
int CSkin::RenderText(HDC hdc, HANDLE hTheme, const TCHAR *szText, RECT *rc, DWORD dtFlags)
{
	if(M->isAero()) {
		DTTOPTS dto = {0};
		dto.dwSize = sizeof(dto);
		dto.dwFlags = DTT_COMPOSITED|DTT_GLOWSIZE;
		dto.iGlowSize = 10;
		return(CMimAPI::m_pfnDrawThemeTextEx(hTheme, hdc, BP_PUSHBUTTON, PBS_NORMAL, szText, -1, dtFlags, rc, &dto));
	}
	else {
		return(::DrawText(hdc, szText, -1, rc, dtFlags));
	}
}
#else
int CSkin::RenderText(HDC hdc, HANDLE hTheme, const TCHAR *szText, RECT *rc, DWORD dtFlags)
{
	return(::DrawText(hdc, szText, -1, rc, dtFlags));
}
#endif

/**
 * Resize a bitmap using image service. The function does not care about the image aspect ratio.
 * The caller is responsible to submit proper values for the desired height and width.
 *
 * @param hBmpSrc  HBITMAP: the source bitmap
 * @param width    LONG: width of the destination bitmap
 * @param height   LONG: height of the new bitmap
 * @param mustFree bool: indicates that the new bitmap had been
 *                 resized and either the source or destination
 *                 bitmap should be freed.
 *
 * @return HBTIAMP: handle to a bitmap with the desired size.
 */
HBITMAP CSkin::ResizeBitmap(HBITMAP hBmpSrc, LONG width, LONG height, bool &mustFree)
{
	BITMAP	bm;

	GetObject(hBmpSrc, sizeof(bm), &bm);
	if(bm.bmHeight != height || bm.bmWidth != width) {
		::ResizeBitmap rb;
		rb.size = sizeof(rb);
		rb.fit = RESIZEBITMAP_STRETCH;
		rb.max_height = height;
		rb.max_width = width;
		rb.hBmp = hBmpSrc;

		HBITMAP hbmNew = (HBITMAP)CallService("IMG/ResizeBitmap", (WPARAM)&rb, 0);
		if(hbmNew != hBmpSrc)
			mustFree = true;
		return(hbmNew);
	}
	else {
		mustFree = false;
		return(hBmpSrc);
	}
}

/**
 * Create a 32bit RGBA bitmap, compatible for rendering with alpha channel.
 * Required by anything which would render on a transparent aero surface.
 *
 * @param rc     RECT &: the rectangle describing the target area.
 * @param dc     The device context for which the bitmap should be created.
 *
 * @return HBITMAP: handle to the bitmap created.
 */
HBITMAP CSkin::CreateAeroCompatibleBitmap(const RECT &rc, HDC dc)
{
	BITMAPINFO dib = {0};

	dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dib.bmiHeader.biWidth = rc.right - rc.left;
    dib.bmiHeader.biHeight = -(rc.bottom - rc.top);
    dib.bmiHeader.biPlanes = 1;
    dib.bmiHeader.biBitCount = 32;
    dib.bmiHeader.biCompression = BI_RGB;

    return(CreateDIBSection(dc, &dib, DIB_RGB_COLORS, NULL, NULL, 0 ));
}

/**
 * Map a given rectangle within the window, specified by hwndClient
 * to the client area of another window.
 *
 * @param hwndClient HWND: Client window
 * @param hwndParent HWND: The window to which the coordinates should be mapped
 * @param rc         RECT &: Rectangular area within the client area of hwndClient.
 *
 *                   It will receive the transformed coordinates, relative to the client area of hwndParent
 */
void CSkin::MapClientToParent(HWND hwndClient, HWND hwndParent, RECT &rc)
{
	POINT pt;
	GetWindowRect(hwndClient, &rc);

	LONG  cx = rc.right - rc.left;
	LONG  cy = rc.bottom - rc.top;

	pt.x = rc.left; pt.y = rc.top;

	ScreenToClient(hwndParent, &pt);

	rc.top = pt.y;
	rc.left = pt.x;
	rc.right = rc.left + cx;
	rc.bottom = rc.top + cy;
}

/**
 * Render the nickname in the info panel.
 * This will also show the status message (if one is available)
 * The field will dynamically adjust itself to the available info panel space. If
 * the info panel is too small to show both nick and UIN fields, this field will show
 * the UIN _instead_ of the nickname (most people have the nickname in the title
 * bar anyway).
 *
 * @param hdc    HDC: target DC for drawing
 *
 * @param rcItem RECT &: target rectangle
 * @param dat    _MessageWindowData*: message window information structure
 */
void CSkin::RenderIPNickname(HDC hdc, RECT &rcItem, _MessageWindowData *dat)
{
	RECT 	rc = rcItem;
	TCHAR 	*szStatusMsg = NULL;
	CSkinItem *item = &SkinItems[ID_EXTBKINFOPANEL];
	TCHAR	*szTextToShow = 0;
	bool	fShowUin = false;

	if(dat->panelHeight < 51) {
		szTextToShow = dat->uin;
		fShowUin = true;
	} else
		szTextToShow = dat->szNickname;

	szStatusMsg = dat->statusMsg;

	dat->szLabel.cx = 0;
	rc.right = rc.left;
	SetBkMode(hdc, TRANSPARENT);

	if (dat->pContainer->bSkinned) {
		RECT rc = rcItem;
		rc.left += item->MARGIN_LEFT;
		rc.right -= item->MARGIN_RIGHT;
		rc.top += item->MARGIN_TOP;
		rc.bottom -= item->MARGIN_BOTTOM;
		if (!item->IGNORED)
			DrawAlpha(hdc, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
					  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
	}
	rcItem.left += 2;
	if (szTextToShow[0]) {
		HFONT hOldFont = 0;
		HICON xIcon = 0;

		xIcon = GetXStatusIcon(dat);

		if (xIcon) {
			DrawIconEx(hdc, rcItem.left, (rcItem.bottom + rcItem.top - PluginConfig.m_smcyicon) / 2, xIcon, PluginConfig.m_smcxicon, PluginConfig.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
			DestroyIcon(xIcon);
			rcItem.left += 21;
		}

		if (PluginConfig.ipConfig.isValid) {
			if(fShowUin) {
				hOldFont = (HFONT)SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_UIN]);
				SetTextColor(hdc, PluginConfig.ipConfig.clrs[IPFONTID_UIN]);
			}
			else {
				hOldFont = (HFONT)SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_NICK]);
				SetTextColor(hdc, PluginConfig.ipConfig.clrs[IPFONTID_NICK]);
			}
		}
		if (szStatusMsg && szStatusMsg[0]) {
			SIZE szNick, sStatusMsg, sMask;
			DWORD dtFlags, dtFlagsNick;

			GetTextExtentPoint32(hdc, szTextToShow, lstrlen(szTextToShow), &szNick);
			GetTextExtentPoint32(hdc, _T("A"), 1, &sMask);
			GetTextExtentPoint32(hdc, szStatusMsg, lstrlen(szStatusMsg), &sStatusMsg);
			dtFlagsNick = DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX;
			if ((szNick.cx + sStatusMsg.cx + 6) < (rcItem.right - rcItem.left) || (rcItem.bottom - rcItem.top) < (2 * sMask.cy))
				dtFlagsNick |= DT_VCENTER;
			CSkin::RenderText(hdc, dat->hThemeIP, szTextToShow, &rcItem, dtFlagsNick);
			if (PluginConfig.ipConfig.isValid) {
				SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_STATUS]);
				SetTextColor(hdc, PluginConfig.ipConfig.clrs[IPFONTID_STATUS]);
			}
			rcItem.left += (szNick.cx + 10);

			if (!(dtFlagsNick & DT_VCENTER))
				//if(dis->rcItem.bottom - dis->rcItem.top >= 2 * sStatusMsg.cy)
				dtFlags = DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX;
			else
				dtFlags = DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER;


			rcItem.right -= 3;
			if (rcItem.left + 30 < rcItem.right)
				CSkin::RenderText(hdc, dat->hThemeIP, szStatusMsg, &rcItem, dtFlags);
		} else
			CSkin::RenderText(hdc, dat->hThemeIP, szTextToShow, &rcItem, DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_NOPREFIX);

		if (hOldFont)
			SelectObject(hdc, hOldFont);
	}
}

/**
 * Draws the UIN field for the info panel.
 *
 * @param hdc    HDC: device context for drawing.
 * @param rcItem RECT &: target rectangle for drawing
 * @param dat    _MessageWindowData *: the message session structure
 */
void CSkin::RenderIPUIN(HDC hdc, RECT &rcItem, _MessageWindowData *dat)
{
	TCHAR	szBuf[256];
	BOOL 	config = PluginConfig.ipConfig.isValid;
	HFONT 	hOldFont = 0;
	CSkinItem *item = &SkinItems[ID_EXTBKINFOPANEL];
	TCHAR	*tszUin = dat->uin;

	SetBkMode(hdc, TRANSPARENT);
	if (dat->pContainer->bSkinned) {
		RECT rc = rcItem;
		rc.left += item->MARGIN_LEFT;
		rc.right -= item->MARGIN_RIGHT;
		rc.top += item->MARGIN_TOP;
		rc.bottom -= item->MARGIN_BOTTOM;
		if (!item->IGNORED)
			DrawAlpha(hdc, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
					  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
	}

	rcItem.left += 2;
	if (config) {
		hOldFont = (HFONT)SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_UIN]);
		SetTextColor(hdc, PluginConfig.ipConfig.clrs[IPFONTID_UIN]);
	}
	if (dat->uin[0]) {
		SIZE sUIN;
		if (dat->idle) {
			time_t diff = time(NULL) - dat->idle;
			int i_hrs = diff / 3600;
			int i_mins = (diff - i_hrs * 3600) / 60;
			mir_sntprintf(szBuf, safe_sizeof(szBuf), _T("%s    Idle: %dh,%02dm"), tszUin, i_hrs, i_mins);
			GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &sUIN);
			CSkin::RenderText(hdc, dat->hThemeIP, szBuf, &rcItem, DT_SINGLELINE | DT_VCENTER);
		} else {
			GetTextExtentPoint32(hdc, tszUin, lstrlen(tszUin), &sUIN);
			CSkin::RenderText(hdc, dat->hThemeIP, tszUin, &rcItem, DT_SINGLELINE | DT_VCENTER);
		}
	}
	if (hOldFont)
		SelectObject(hdc, hOldFont);
}

void CSkin::RenderIPStatus(HDC hdc, RECT &rcItem, _MessageWindowData *dat)
{
	char		*szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
	SIZE		sProto = {0}, sStatus = {0}, sTime = {0};
	DWORD		oldPanelStatusCX = dat->panelStatusCX;
	RECT		rc;
	HFONT		hOldFont = 0;
	BOOL		config = PluginConfig.ipConfig.isValid;
	CSkinItem 	*item = &SkinItems[ID_EXTBKINFOPANEL];
	TCHAR   	*szFinalProto = NULL;
	TCHAR 		szResult[80];
	int 		base_hour;
	TCHAR 		symbolic_time[3];

	szResult[0] = 0;

	if (dat->szStatus[0])
		GetTextExtentPoint32(hdc, dat->szStatus, lstrlen(dat->szStatus), &sStatus);

	/*
	 * figure out final account name
	 */
	if(dat->bIsMeta) {
		PROTOACCOUNT *acc = (PROTOACCOUNT *)CallService(MS_PROTO_GETACCOUNT, (WPARAM)0, (LPARAM)(dat->bIsMeta ? dat->szMetaProto : dat->szProto));
		if(acc && acc->tszAccountName)
			szFinalProto = acc->tszAccountName;
	}
	else
		szFinalProto = dat->szAccount;

	if (szFinalProto) {
		if (config)
			SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_PROTO]);
		GetTextExtentPoint32(hdc, szFinalProto, lstrlen(szFinalProto), &sProto);
	}

	if (dat->timezone != -1) {
		DBTIMETOSTRINGT dbtts;
		time_t 			final_time;
		time_t 			now = time(NULL);

		final_time = now - dat->timediff;
		dbtts.szDest = szResult;
		dbtts.cbDest = 70;
		dbtts.szFormat = _T("t");
		CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, final_time, (LPARAM) &dbtts);
		GetTextExtentPoint32(hdc, szResult, lstrlen(szResult), &sTime);
	}

	dat->panelStatusCX = 3 + sStatus.cx + sProto.cx + 14 + (dat->hClientIcon ? 20 : 0) + sTime.cx + 13;;

	if (dat->panelStatusCX != oldPanelStatusCX) {
		SendMessage(dat->hwnd, WM_SIZE, 0, 0);
		CSkin::MapClientToParent(GetDlgItem(dat->hwnd, IDC_PANELSTATUS), dat->hwnd, rcItem);
	}

	SetBkMode(hdc, TRANSPARENT);
	rc = rcItem;
	if (dat->pContainer->bSkinned) {
		RECT rc = rcItem;
		rc.left += item->MARGIN_LEFT;
		rc.right -= item->MARGIN_RIGHT;
		rc.top += item->MARGIN_TOP;
		rc.bottom -= item->MARGIN_BOTTOM;
		if (!item->IGNORED)
			DrawAlpha(hdc, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
					  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
	}
	rc.left += 2;
	rc.right -=3;

	if(szResult[0]) {
		HFONT oldFont = (HFONT)SelectObject(hdc, PluginConfig.m_hFontWebdings);
		base_hour = _ttoi(szResult);
		base_hour = base_hour > 11 ? base_hour - 12 : base_hour;
		symbolic_time[0] = (TCHAR)(0xB7 + base_hour);
		symbolic_time[1] = 0;
		CSkin::RenderText(hdc, dat->hThemeIP, symbolic_time, &rcItem, DT_SINGLELINE | DT_VCENTER);
		if (config) {
			oldFont = (HFONT)SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_TIME]);
			SetTextColor(hdc, PluginConfig.ipConfig.clrs[IPFONTID_TIME]);
		}
		rcItem.left += 16;
		CSkin::RenderText(hdc, dat->hThemeIP, szResult, &rcItem, DT_SINGLELINE | DT_VCENTER);
		SelectObject(hdc, oldFont);
		rc.left += (sTime.cx + 20);
	}

	if (config)
		hOldFont = (HFONT)SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_STATUS]);

	if (dat->szStatus[0]) {
		if (config) {
			SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_STATUS]);
			SetTextColor(hdc, PluginConfig.ipConfig.clrs[IPFONTID_STATUS]);
		}
		CSkin::RenderText(hdc, dat->hThemeIP, dat->szStatus, &rc, DT_SINGLELINE | DT_VCENTER);
	}
	if (szFinalProto) {
		rc.left = rc.right - sProto.cx - 3 - (dat->hClientIcon ? 20 : 0);
		if (config) {
			SelectObject(hdc, PluginConfig.ipConfig.hFonts[IPFONTID_PROTO]);
			SetTextColor(hdc, PluginConfig.ipConfig.clrs[IPFONTID_PROTO]);
		} else
			SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
		CSkin::RenderText(hdc, dat->hThemeIP, szFinalProto, &rc, DT_SINGLELINE | DT_VCENTER);
	}

	if (dat->hClientIcon)
		DrawIconEx(hdc, rc.right - 19, (rc.bottom + rc.top - 16) / 2, dat->hClientIcon, 16, 16, 0, 0, DI_NORMAL);

	if (config && hOldFont)
		SelectObject(hdc, hOldFont);
}

/**
 * Draw the background for the message window tool bar
 *
 * @param dat      _MessageWindowData *: structure describing the message session
 *
 * @param hdc      HDC: handle to the device context in which painting should occur.
 * @param rcWindow RECT &: The window rectangle of the message dialog window
 */
void CSkin::RenderToolbarBG(const _MessageWindowData *dat, HDC hdc, const RECT &rcWindow)
{
	if(dat) {
		if(dat->pContainer->dwFlags & CNT_HIDETOOLBAR)
			return;

		RECT 	rc, rcToolbar;;
		POINT	pt;
		if(!(dat->pContainer->dwFlags & CNT_BOTTOMTOOLBAR)) {
			GetWindowRect(GetDlgItem(dat->hwnd, dat->bType == SESSIONTYPE_CHAT ? IDC_CHAT_LOG : IDC_LOG), &rc);
			pt.y = rc.bottom + 0;
			ScreenToClient(dat->hwnd, &pt);
			rcToolbar.top = pt.y;
			rcToolbar.left = 0;
			rcToolbar.right = rcWindow.right;
			GetWindowRect(GetDlgItem(dat->hwnd, dat->bType == SESSIONTYPE_CHAT ? IDC_CHAT_MESSAGE : IDC_MESSAGE), &rc);
			pt.y = rc.top - 2;
			ScreenToClient(dat->hwnd, &pt);
			rcToolbar.bottom = pt.y;
		}
		else {
			GetWindowRect(GetDlgItem(dat->hwnd, dat->bType == SESSIONTYPE_CHAT ? IDC_CHAT_MESSAGE : IDC_MESSAGE), &rc);
			pt.y = rc.bottom - (dat->bType == SESSIONTYPE_IM ? 2 : 0);;
			ScreenToClient(dat->hwnd, &pt);
			rcToolbar.top = pt.y + 1;
			rcToolbar.left = 0;
			rcToolbar.right = rcWindow.right;
			rcToolbar.bottom = rcWindow.bottom;
		}
		LONG cx = rcToolbar.right - rcToolbar.left;
		LONG cy = rcToolbar.bottom - rcToolbar.top;

		if(dat->pContainer->cachedToolbarDC) {
			SelectObject(dat->pContainer->cachedToolbarDC, dat->pContainer->oldhbmToolbarBG);
			DeleteObject(dat->pContainer->hbmToolbarBG);
			DeleteDC(dat->pContainer->cachedToolbarDC);
		}
		dat->pContainer->cachedToolbarDC = CreateCompatibleDC(hdc);
		dat->pContainer->hbmToolbarBG = CreateCompatibleBitmap(hdc, cx, cy);
		dat->pContainer->oldhbmToolbarBG = (HBITMAP)SelectObject(dat->pContainer->cachedToolbarDC, dat->pContainer->hbmToolbarBG);

		CMimAPI::m_pfnDrawThemeBackground(dat->hThemeToolbar, hdc, 7, PBS_NORMAL, &rcToolbar, &rcToolbar);

		RECT rcCachedToolbar = {0};
		rcCachedToolbar.right = cx;
		rcCachedToolbar.bottom = cy;

		CMimAPI::m_pfnDrawThemeBackground(dat->hThemeToolbar, dat->pContainer->cachedToolbarDC, 7, PBS_NORMAL,
										  &rcCachedToolbar, &rcCachedToolbar);
	}
}
