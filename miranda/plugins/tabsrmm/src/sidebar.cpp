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
 * the contact switch bar on the left (or right) side
 *
 */

#include "commonheaders.h"

extern void GetIconSize(HICON hIcon, int* sizeX, int* sizeY);

HBITMAP CSideBar::m_hbmBackground = 0;
UINT	CSideBar::m_hbmRefCount = 0;

TSideBarLayout CSideBar::m_layouts[CSideBar::NR_LAYOUTS] = {
	{
		_T("Like tabs, vertical text orientation"),
		26, 30,
		SIDEBARLAYOUT_DYNHEIGHT | SIDEBARLAYOUT_VERTICALORIENTATION,
		CSideBar::m_DefaultContentRenderer,
		CSideBar::m_DefaultBackgroundRenderer,
		NULL,
		NULL,
		SIDEBARLAYOUT_VERTICAL
	},
	{
		_T("Compact layout, horizontal buttons"),
		100, 24,
		0,
		CSideBar::m_DefaultContentRenderer,
		CSideBar::m_DefaultBackgroundRenderer,
		NULL,
		NULL,
		SIDEBARLAYOUT_NORMAL
	}
};

CSideBarButton::CSideBarButton(const _MessageWindowData *dat, const CSideBar *sideBar)
{
	m_dat = dat;
	m_id = reinterpret_cast<UINT>(dat->hContact);								// set the control id
	m_sideBar = sideBar;
	_create();
}

CSideBarButton::CSideBarButton(const UINT id, const CSideBar *sideBar)
{
	m_dat = 0;
	m_id = id;
	m_sideBar = sideBar;
	_create();
}

/**
 * Internal method to create the button item and configure the associated button control
 */
void CSideBarButton::_create()
{
	m_hwnd = 0;
	m_isTopAligned = true;
	m_sz.cx = m_sz.cy = 0;

	m_hwnd = ::CreateWindowEx(0, _T("TSButtonClass"), _T(""), WS_CHILD | WS_TABSTOP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
							  0, 0, 40, 40, m_sideBar->getScrollWnd(), reinterpret_cast<HMENU>(m_id), g_hInst, NULL);

	if(m_hwnd) {
		::SendMessage(m_hwnd, BUTTONSETASSIDEBARBUTTON, 0, (LPARAM)this);
		::SendMessage(m_hwnd, BUTTONSETASFLATBTN, 1,  1);
		::SendMessage(m_hwnd, BUTTONSETASFLATBTN + 10, 1,  1);
		::SendMessage(m_hwnd, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_sideBar->getContainer());
		m_buttonControl = (MButtonCtrl *)::GetWindowLongPtr(m_hwnd, 0);
	}
	else
		delete this;

	if(m_id == IDC_SIDEBARUP || m_id == IDC_SIDEBARDOWN)
		::SetParent(m_hwnd, m_sideBar->getContainer()->hwnd);
}

CSideBarButton::~CSideBarButton()
{
	if(m_hwnd) {
		::SendMessage(m_hwnd, BUTTONSETASSIDEBARBUTTON, 0, 0);		// make sure, the button will no longer call us back
		::DestroyWindow(m_hwnd);
		//_DebugTraceA("button item %d destroyed", m_id);
	}
}

void CSideBarButton::Show(const int showCmd) const
{
	::ShowWindow(m_hwnd, showCmd);
}

/**
 * Measure the metrics for the current item. The side bar layouting will call this
 * whenever a layout with a dynamic height is active). For fixed dimension layouts,
 * m_elementWidth and m_elementHeight will be used.
 *
 * @return SIZE&: reference to the item's size member. The caller may use cx and cy values
 *         to determine further layouting actions.
 */
const SIZE& CSideBarButton::measureItem()
{
	SIZE	 sz;

	if(m_sideBarLayout->pfnMeasureItem)
		m_sideBarLayout->pfnMeasureItem(this);        // use the current layout's function, if available, else use default
	else {
		HDC dc = GetDC(m_hwnd);
		TCHAR	 tszLabel[255];

		HFONT oldFont = (HFONT)::SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));

		mir_sntprintf(tszLabel, 255, _T("%s"), m_dat->newtitle);
		::GetTextExtentPoint32(dc, tszLabel, lstrlen(tszLabel), &sz);

		sz.cx += 26;

		m_sz.cy = sz.cx;

		::SelectObject(dc, oldFont);
		ReleaseDC(m_hwnd, dc);
	}
	return(m_sz);
}
/**
 * Render the button item. Callback from the button window procedure
 *
 * @param ctl    MButtonCtrl *: pointer to the private button data structure
 * @param hdc    HDC: device context for painting
 */
void CSideBarButton::RenderThis(const HDC hdc) const
{
	RECT		rc;
	LONG		cx, cy;
	HDC			hdcMem = 0;
	bool		fVertical = (m_sideBarLayout->dwFlags & CSideBar::SIDEBARLAYOUT_VERTICALORIENTATION) ? true : false;
	HBITMAP		hbmMem, hbmOld;
	HANDLE		hbp = 0;

	::GetClientRect(m_hwnd, &rc);

	if(m_id == IDC_SIDEBARUP || m_id == IDC_SIDEBARDOWN)
		fVertical = false;

	if(fVertical) {
		cx = rc.bottom;
		cy = rc.right;
	}
	else {
		cx = rc.right;
		cy = rc.bottom;
	}

	hdcMem = ::CreateCompatibleDC(hdc);

	if(fVertical) {
		RECT	rcFlipped = {0,0,cx,cy};
		hbmMem = CSkin::CreateAeroCompatibleBitmap(rcFlipped, hdcMem);
		rc = rcFlipped;
	}
	else
		hbmMem = CSkin::CreateAeroCompatibleBitmap(rc, hdcMem);

	hbmOld = (HBITMAP)::SelectObject(hdcMem, hbmMem);

	HFONT hFontOld = (HFONT)::SelectObject(hdcMem, GetStockObject(DEFAULT_GUI_FONT));

	m_sideBarLayout->pfnBackgroundRenderer(hdcMem, &rc, this);
	m_sideBarLayout->pfnContentRenderer(hdcMem, &rc, this);

	::SelectObject(hdcMem, hFontOld);

	/*
	 * for vertical tabs, we did draw to a rotated rectangle, so we now must rotate the
	 * final bitmap back to it's original orientation
	 */

	if(fVertical) {
		::SelectObject(hdcMem, hbmOld);

		FIBITMAP *fib = FIF->FI_CreateDIBFromHBITMAP(hbmMem);
		FIBITMAP *fib_new = FIF->FI_RotateClassic(fib, 90.0f);
		FIF->FI_Unload(fib);
		::DeleteObject(hbmMem);
		hbmMem = FIF->FI_CreateHBITMAPFromDIB(fib_new);
		FIF->FI_Unload(fib_new);
		hbmOld =(HBITMAP)::SelectObject(hdcMem, hbmMem);
		::BitBlt(hdc, 0, 0, cy, cx, hdcMem, 0, 0, SRCCOPY);
		::SelectObject(hdcMem, hbmOld);
		::DeleteObject(hbmMem);
		::DeleteDC(hdcMem);
	}
	else {
		::BitBlt(hdc, 0, 0, cx, cy, hdcMem, 0, 0, SRCCOPY);
		::SelectObject(hdcMem, hbmOld);
		::DeleteObject(hbmMem);
		::DeleteDC(hdcMem);
	}
	return;
}

void CSideBarButton::renderIconAndNick(const HDC hdc, const RECT *rcItem) const
{
	HICON	hIcon;
	RECT	rc = *rcItem;
	DWORD	dwTextFlags = DT_SINGLELINE | DT_VCENTER;
	int		stateId = m_buttonControl->stateId;
	int		iSize = 16;
	const ContainerWindowData *pContainer = m_sideBar->getContainer();

	if(m_dat && pContainer) {
		if (m_dat->dwFlags & MWF_ERRORSTATE)
			hIcon = PluginConfig.g_iconErr;
		else if (m_dat->mayFlashTab)
			hIcon = m_dat->iFlashIcon;
		else {
			if (m_dat->si && m_dat->iFlashIcon) {
				int sizeX, sizeY;

				hIcon = m_dat->iFlashIcon;
				::GetIconSize(hIcon, &sizeX, &sizeY);
				iSize = sizeX;
			} else if (m_dat->hTabIcon == m_dat->hTabStatusIcon && m_dat->hXStatusIcon)
				hIcon = m_dat->hXStatusIcon;
			else
				hIcon = m_dat->hTabIcon;
		}

		if (m_dat->mayFlashTab == FALSE || (m_dat->mayFlashTab == TRUE && m_dat->bTabFlash != 0) || !(pContainer->dwFlagsEx & TCF_FLASHICON)) {
			DWORD ix = rc.left + 2;
			DWORD iy = (rc.bottom + rc.top - iSize) / 2;
			if (m_dat->dwFlagsEx & MWF_SHOW_ISIDLE && PluginConfig.m_IdleDetect)
				CSkin::DrawDimmedIcon(hdc, ix, iy, iSize, iSize, hIcon, 180);
			else
				::DrawIconEx(hdc, ix, iy, hIcon, iSize, iSize, 0, NULL, DI_NORMAL | DI_COMPAT);
		}

		rc.left += (iSize + 2);
		SetBkMode(hdc, TRANSPARENT);

		if (m_dat->mayFlashTab == FALSE || (m_dat->mayFlashTab == TRUE && m_dat->bTabFlash != 0) || !(pContainer->dwFlagsEx & TCF_FLASHLABEL)) {
			dwTextFlags |= DT_WORD_ELLIPSIS;
			if(M->isAero())
				CSkin::RenderText(hdc, m_buttonControl->hThemeButton, m_dat->newtitle, &rc, dwTextFlags, 7);
			else {
				::SetTextColor(hdc, (stateId == PBS_PRESSED || m_sideBar->getActiveItem() == this) ? ::GetSysColor(COLOR_BTNHIGHLIGHT) :
						   ::GetSysColor(COLOR_BTNTEXT));
				//if(M->isVSThemed())
					//M->m_pfnDrawThemeText(m_buttonControl->hThemeButton, hdc, 1, stateId == PBS_PRESSED ? 3 : (stateId == PBS_HOT ? 2 : 1),
					//					  m_dat->newtitle, lstrlen(m_dat->newtitle), dwTextFlags, 0, &rc);
				//else
				::DrawText(hdc, m_dat->newtitle, lstrlen(m_dat->newtitle), &rc, dwTextFlags);
			}
		}
	}
}

/**
 * call back from the button window procedure. Activate my session
 */
void CSideBarButton::activateSession() const
{
	if(m_dat)
		::SendMessage(m_dat->hwnd, DM_ACTIVATEME, 0, 0);
}

CSideBar::CSideBar(ContainerWindowData *pContainer)
{
	m_pContainer = pContainer;
	m_up = m_down = 0;
	m_hwndScrollWnd = 0;
	m_buttonlist.clear();
	m_activeItem = 0;

	Init(true);

	if(m_hbmBackground == 0) {
		HDC		hdcWnd = ::GetDC(m_pContainer->hwnd);
		HDC		hdcMem = ::CreateCompatibleDC(hdcWnd);
		RECT    rc = {0, 0, 350, 150};
		HBITMAP hbm = CSkin::CreateAeroCompatibleBitmap(rc, hdcWnd);
		HBITMAP hbmOld = (HBITMAP)::SelectObject(hdcMem, hbm);

		::DrawAlpha(hdcMem, &rc, 0x333333, 255, 0xf0f0f0, 0, GRADIENT_ACTIVE | GRADIENT_RL, 31, 7, 0);

		::SelectObject(hdcMem, hbmOld);
		::DeleteDC(hdcMem);
		::ReleaseDC(m_pContainer->hwnd, hdcWnd);
		FIBITMAP *fib = FIF->FI_CreateDIBFromHBITMAP(hbm);
		FIBITMAP *fib_new = FIF->FI_RotateClassic(fib, 90.0f);

		m_hbmBackground = FIF->FI_CreateHBITMAPFromDIB(fib_new);
		FIF->FI_Unload(fib);
		FIF->FI_Unload(fib_new);

		::DeleteObject(hbm);
	}
	m_hbmRefCount++;
}

CSideBar::~CSideBar()
{
	destroyScroller();
	m_buttonlist.clear();

	m_hbmRefCount--;
	if(m_hbmRefCount == 0) {
		::DeleteObject(m_hbmBackground);
		m_hbmBackground = 0;
	}
	if(m_hwndScrollWnd)
		::DestroyWindow(m_hwndScrollWnd);
}

void CSideBar::Init(const bool fForce)
{
	m_iTopButtons = m_iBottomButtons = 0;
	m_topHeight = m_bottomHeight = 0;
	m_firstVisibleOffset = 0;
	m_totalItemHeight = 0;

	m_uLayout = (m_pContainer->dwFlagsEx & 0xff000000) >> 24;
	m_uLayout = ((m_uLayout >= 0 && m_uLayout < NR_LAYOUTS) ? m_uLayout : 0);

	m_currentLayout = &m_layouts[m_uLayout];

	m_dwFlags = m_currentLayout->dwFlags;

	if(m_pContainer->dwFlags & CNT_SIDEBAR) {
		if(m_hwndScrollWnd == 0)
			m_hwndScrollWnd = ::CreateWindowEx(0, _T("SideBarClass"), _T(""), WS_CLIPCHILDREN | WS_CLIPSIBLINGS |  WS_VISIBLE | WS_CHILD,
											   0, 0, m_width, 40, m_pContainer->hwnd, reinterpret_cast<HMENU>(5000), g_hInst, this);

		m_isVisible = m_isActive ? m_isVisible : true;
		m_isActive = true;
		createScroller();
		m_elementHeight = m_currentLayout->height;
		m_elementWidth = m_currentLayout->width;
		m_width = m_elementWidth + 4;
		populateAll();
		if(m_activeItem)
			setActiveItem(m_activeItem);
	}
	else {
		destroyScroller();
		m_width = 0;
		m_isActive = m_isVisible = false;
		m_activeItem = 0;

		removeAll();
		if(m_hwndScrollWnd)
			::DestroyWindow(m_hwndScrollWnd);
		m_hwndScrollWnd = 0;
	}
}

void CSideBar::setVisible(bool fNewVisible)
{
	m_isVisible = fNewVisible;

	/*
	 * only needed on hiding. Layout() will do it when showing it
	 */

	if(!m_isVisible)
		showAll(SW_HIDE);
	else {
		m_up->Show(SW_SHOW);
		m_down->Show(SW_SHOW);
	}
}

/**
 * Create both scrollbar buttons which can be used to scroll the switchbar
 * up and down.
 */
void CSideBar::createScroller()
{
	if(m_up == 0)
		m_up = new CSideBarButton(IDC_SIDEBARUP, this);
	if(m_down == 0)
		m_down = new CSideBarButton(IDC_SIDEBARDOWN, this);

	m_up->setLayout(m_currentLayout);
	m_down->setLayout(m_currentLayout);
}

/**
 * Destroy the scroller buttons.
 */
void CSideBar::destroyScroller()
{
	if(m_up) {
		delete m_up;
		m_up = 0;
	}
	if(m_down) {
		delete m_down;
		m_down = 0;
	}
}

/**
 * remove all buttons from the current list
 * Does not remove the sessions. This is basically only used when switching
 * from a sidebar to a tabbed interface
 */
void CSideBar::removeAll()
{
	std::vector<CSideBarButton *>::iterator item = m_buttonlist.begin();

	while(item != m_buttonlist.end()) {
		delete *item;
		item++;
	}
	m_buttonlist.clear();
}

void CSideBar::populateAll()
{
	HWND	hwndTab = ::GetDlgItem(m_pContainer->hwnd, IDC_MSGTABS);

	if(hwndTab) {
		int			iItems = (int)TabCtrl_GetItemCount(hwndTab);
		TCITEM		item = {0};

		item.mask = TCIF_PARAM;
		std::vector<CSideBarButton *>::iterator b_item;

		m_iTopButtons = 0;

		for(int i = 0; i < iItems; i++) {
			TabCtrl_GetItem(hwndTab, i, &item);
			if(item.lParam && ::IsWindow((HWND)item.lParam)) {
				_MessageWindowData *dat = (_MessageWindowData *)::GetWindowLongPtr((HWND)item.lParam, GWLP_USERDATA);
				if(dat) {
			    	if((b_item = findSession(dat)) == m_buttonlist.end())
						addSession(dat, i);
					else {
						(*b_item)->setLayout(m_currentLayout);
						if(m_dwFlags & SIDEBARLAYOUT_VERTICALORIENTATION) {
							(*b_item)->measureItem();
							m_topHeight += ((*b_item)->getHeight() + 1);
						}
						else
							m_topHeight += (m_elementHeight + 1);
					}
				}
			}
		}
	}
}
/**
 * Add a new session to the switchbar.
 *
 * @param dat    _MessageWindowData *: session data for a client session. Must be fully initialized
 *               (that is, it can only be used after WM_INITIALOG completed).
 */
void CSideBar::addSession(const _MessageWindowData *dat, int position)
{
	if(!m_isActive)
		return;

	CSideBarButton *item = new CSideBarButton(dat, this);

	item->setLayout(m_currentLayout);

	if(m_dwFlags & SIDEBARLAYOUT_DYNHEIGHT) {
		SIZE sz = item->measureItem();
		m_topHeight += (sz.cy + 1);
	}
	else
		m_topHeight += (m_elementHeight + 1);

	m_iTopButtons++;
	if(position == -1)
		m_buttonlist.push_back(item);
	else {
		std::vector<CSideBarButton *>::iterator it = m_buttonlist.begin() + position;
		m_buttonlist.insert(it, item);
	}
	Invalidate();
}

/**
 * Remove a new session from the switchbar.
 *
 * @param dat    _MessageWindowData *: session data for a client session.
 */
HRESULT CSideBar::removeSession(const _MessageWindowData *dat)
{
	if(dat) {
		std::vector<CSideBarButton *>::iterator item = findSession(dat);

		if(item != m_buttonlist.end()) {
			m_iTopButtons--;
			if(m_dwFlags & SIDEBARLAYOUT_DYNHEIGHT) {
				SIZE sz = (*item)->getSize();
				m_topHeight -= (sz.cy + 1);
			}
			else
				m_topHeight -= (m_elementHeight + 1);
			delete *item;
			m_buttonlist.erase(item);
			Invalidate();
			return(S_OK);
		}
	}
	return(S_FALSE);
}

/**
 * make sure the given item is visible in a scrolled switch bar
 *
 * @param item   CSideBarButtonItem*: the item which must be visible in the switch bar
 */
void CSideBar::scrollIntoView(const CSideBarButton *item)
{
	LONG	spaceUsed = 0, itemHeight;
	bool	fNeedLayout = false;

	if(!m_isActive)
		return;

	if(item == 0)
		item = m_activeItem;

	ButtonIterator it = m_buttonlist.begin();

	while(it != m_buttonlist.end()) {
		itemHeight = (*it)->getHeight();
		spaceUsed += (itemHeight + 1);
		if(*it == item )
			break;
		it++;
	}

	RECT	rc;
	GetClientRect(m_hwndScrollWnd, &rc);

	if(m_topHeight <= rc.bottom) {
		m_firstVisibleOffset = 0;
		Layout();
		return;
	}
	if(it == m_buttonlist.end() || (it == m_buttonlist.begin() && m_firstVisibleOffset == 0)) {
		Layout();
		return;									// do nothing for the first item and .end() should not really happen
	}

	if(spaceUsed <= rc.bottom && spaceUsed - (itemHeight + 1) >= m_firstVisibleOffset) {
		Layout();
		return;						// item fully visible, do nothing
	}

	/*
	* button partially or not at all visible at the top
	*/
	if(spaceUsed < m_firstVisibleOffset || spaceUsed - (itemHeight + 1) < m_firstVisibleOffset) {
		m_firstVisibleOffset = spaceUsed - (itemHeight + 1);
		fNeedLayout = true;
	} else {
		if(spaceUsed > rc.bottom) {				// partially or not at all visible at the bottom
			fNeedLayout = true;
			m_firstVisibleOffset = spaceUsed - rc.bottom;
		}
	}

	//if(fNeedLayout)
		Layout();
}
/**
 * Invalidate the button associated with the given session.
 *
 * @param dat    _MessageWindowData*: Session data
 */
void CSideBar::updateSession(const _MessageWindowData *dat)
{
	if(!m_isVisible || !m_isActive)
		return;

	std::vector<CSideBarButton *>::iterator item = findSession(dat);
	//_DebugTraceW(_T("update session for %s"), dat->szNickname);
	if(item != m_buttonlist.end()) {
		if(m_dwFlags & SIDEBARLAYOUT_DYNHEIGHT) {
			LONG oldHeight = (*item)->getHeight();
			m_topHeight -= (oldHeight + 1);
			SIZE sz = (*item)->measureItem();
			m_topHeight += (sz.cy + 1);
			if(sz.cy != oldHeight)
				Invalidate();
			else
				::InvalidateRect((*item)->getHwnd(), NULL, FALSE);
		}
		else
			::InvalidateRect((*item)->getHwnd(), NULL, FALSE);
	}
}

/**
 * Sets the active session item
 * called from the global update handler in msgdialog/group room window
 * on every tab activation to ensure consistency
 *
 * @param dat    _MessageWindowData*: session data
 *
 * @return The previously active item (that can be zero)
 */
const CSideBarButton* CSideBar::setActiveItem(const _MessageWindowData *dat)
{
	std::vector<CSideBarButton *>::iterator item = findSession(dat);
	if(item != m_buttonlist.end()) {
		return(setActiveItem(*item));
	}
	return(0);
}

/**
 * Layout the buttons within the available space... ensure that buttons are
 * set to invisible if there is not enough space. Also, update the state of
 * the scroller buttons
 *
 * @param rc        RECT*:the window rectangle
 *
 * @param fOnlyCalc bool: if false (default), window positions will be updated, otherwise,
 *                  the method will only calculate the layout parameters. A final call to
 *                  Layout() with the parameter set to false is required to perform the
 *                  position update.
 */
void CSideBar::Layout(const RECT *rc, bool fOnlyCalc)
{
	if(!m_isVisible)
		return;

	RECT	rcWnd;

	rc = &rcWnd;
	::GetClientRect(m_hwndScrollWnd, &rcWnd);

	if(m_currentLayout->pfnLayout) {
		m_currentLayout->pfnLayout(this, const_cast<RECT *>(rc));
		return;
	}

	HDWP hdwp = ::BeginDeferWindowPos(1);

	int 	topCount = 0, bottomCount = 0;
	size_t  i = 0, j = 0;
	BOOL 	topEnabled = FALSE, bottomEnabled = FALSE;
	HWND 	hwnd;
	LONG 	spaceUsed = 0;
	DWORD 	dwFlags = SWP_NOZORDER|SWP_NOACTIVATE;
	LONG	iSpaceAvail = rc->bottom;

	m_firstVisibleOffset = max(0, m_firstVisibleOffset);

	ButtonIterator item = m_buttonlist.begin();

	m_totalItemHeight = 0;

	LONG height = m_elementHeight;

	while (item != m_buttonlist.end()) {
		hwnd = (*item)->getHwnd();

		if(m_dwFlags & SIDEBARLAYOUT_DYNHEIGHT)
			height = (*item)->getHeight();

		if(spaceUsed > iSpaceAvail || m_totalItemHeight + height < m_firstVisibleOffset) {
			::ShowWindow(hwnd, SW_HIDE);
			item++;
			m_totalItemHeight += (height + 1);
			continue;
		}

		if ((*item)->isTopAligned()) {
			if(m_totalItemHeight <= m_firstVisibleOffset) {				// partially visible
				if(!fOnlyCalc)
					hdwp = ::DeferWindowPos(hdwp, hwnd, 0, 2, -(m_firstVisibleOffset - m_totalItemHeight),
								   m_elementWidth, height, SWP_SHOWWINDOW | dwFlags);
				spaceUsed += ((height + 1) - (m_firstVisibleOffset - m_totalItemHeight));
				m_totalItemHeight += (height + 1);
			}
			else {
				if(!fOnlyCalc)
					hdwp = ::DeferWindowPos(hdwp, hwnd, 0, 2, spaceUsed,
								   m_elementWidth, height, SWP_SHOWWINDOW | dwFlags);
				spaceUsed += (height + 1);
				m_totalItemHeight += (height + 1);
			}
		}
		else {
			/*
			if(!fOnlyCalc)
				::SetWindowPos(hwnd, 0, 2, (rc->bottom - rc->top) - bottomOffset - height,
							   m_elementWidth, height, dwFlags);
			bottomOffset += (height + 1);
			*/
		}
		item++;
	}
	topEnabled = m_firstVisibleOffset > 0;
	bottomEnabled = (m_totalItemHeight - m_firstVisibleOffset > rc->bottom);
	::EndDeferWindowPos(hdwp);

	//_DebugTraceA("layout: total %d, used: %d, first visible: %d", m_totalItemHeight, spaceUsed, m_firstVisibleOffset);
	if(!fOnlyCalc) {
		RECT	rcContainer;
		::GetClientRect(m_pContainer->hwnd, &rcContainer);

		::SetWindowPos(m_up->getHwnd(), 0, m_pContainer->tBorder_outer_left, m_pContainer->tBorder_outer_top + m_pContainer->MenuBar->getHeight(),
					   m_elementWidth + 4, 14, dwFlags | SWP_SHOWWINDOW);
		::SetWindowPos(m_down->getHwnd(), 0, m_pContainer->tBorder_outer_left, (rcContainer.bottom - 14 - m_pContainer->statusBarHeight - 1),
					   m_elementWidth + 4, 14, dwFlags | SWP_SHOWWINDOW);
		::EnableWindow(m_up->getHwnd(), topEnabled);
		::EnableWindow(m_down->getHwnd(), bottomEnabled);
		::InvalidateRect(m_up->getHwnd(), NULL, FALSE);
		::InvalidateRect(m_down->getHwnd(), NULL, FALSE);
	}
}

inline void CSideBar::Invalidate()
{
	Layout(0);
}

void CSideBar::showAll(int showCmd)
{
	::ShowWindow(m_up->getHwnd(), showCmd);
	::ShowWindow(m_down->getHwnd(), showCmd);

	std::vector<CSideBarButton *>::iterator item = m_buttonlist.begin();

	while(item != m_buttonlist.end())
		::ShowWindow((*item++)->getHwnd(), showCmd);
}

/**
 * Helper function: find a button item associated to the given
 * session data
 *
 * @param dat    _MessageWindowData*: session information
 *
 * @return CSideBarButtonItem*: pointer to the found item. Zero, if none was found
 */
std::vector<CSideBarButton *>::iterator CSideBar::findSession(const _MessageWindowData *dat)
{
	if(dat) {
		std::vector<CSideBarButton *>::iterator item = m_buttonlist.begin();

		if(m_buttonlist.size() > 0) {
			while(item != m_buttonlist.end()) {
				if((*item)->getDat() == dat)
					return(item);
				item++;
			}
		}
	}
	return(m_buttonlist.end());
}

/**
 * Helper function: find a button item associated to the given
 * contact handle
 *
 * @param hContact  HANDLE: contact's handle to look for
 *
 * @return CSideBarButtonItem*: pointer to the found item. Zero, if none was found
 */
std::vector<CSideBarButton *>::iterator CSideBar::findSession(const HANDLE hContact)
{
	if(hContact) {
		std::vector<CSideBarButton *>::iterator item = m_buttonlist.begin();

		if(m_buttonlist.size() > 0) {
			while(item != m_buttonlist.end()) {
				if((*item)->getContactHandle() == hContact)
					return(item);
				item++;
			}
		}
	}
	return(m_buttonlist.end());
}

void CSideBar::processScrollerButtons(UINT commandID)
{
	if(!m_isActive || m_down == 0)
		return;

	/*if (commandID == IDC_SIDEBARUP)
		m_firstButton = m_firstButton > 0 ? --m_firstButton : 0;
	else
	 	m_firstButton += IsWindowEnabled(m_down->getHwnd()) ? 1 : 0;
	 	*/

	if(commandID == IDC_SIDEBARDOWN && ::IsWindowEnabled(m_down->getHwnd()))
		m_firstVisibleOffset += 10;
	else if(commandID == IDC_SIDEBARUP && ::IsWindowEnabled(m_up->getHwnd()))
		m_firstVisibleOffset = max(0, m_firstVisibleOffset - 10);

	Layout(0);
}

void CSideBar::resizeScrollWnd(LONG x, LONG y, LONG width, LONG height) const
{
	if(!m_isVisible || !m_isActive) {
		::ShowWindow(m_hwndScrollWnd, SW_HIDE);
		return;
	}
	::SetWindowPos(m_hwndScrollWnd, 0, x, y + 15, m_width, height - 30,
				   SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSENDCHANGING | SWP_DEFERERASE | SWP_ASYNCWINDOWPOS);
}

LRESULT CALLBACK CSideBar::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_SIZE:
			return(TRUE);
		case WM_ERASEBKGND: {
			HDC hdc = (HDC)wParam;
			RECT rc;
			GetClientRect(hwnd, &rc);
			::FillRect(hdc, &rc, (HBRUSH)(M->isAero() ? ::GetStockObject(BLACK_BRUSH) : ::GetSysColorBrush(COLOR_3DFACE)));
			return(1);
		}
		default:
			break;
	}
	return(DefWindowProc(hwnd, msg, wParam, lParam));
}

LRESULT CALLBACK CSideBar::wndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CSideBar *sideBar = (CSideBar *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if(sideBar)
		return(sideBar->wndProc(hwnd, msg, wParam, lParam));

	switch(msg) {
		case WM_NCCREATE: {
			CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
			return(TRUE);
		}
		default:
			break;
	}
	return(DefWindowProc(hwnd, msg, wParam, lParam));
}
/**
 * paints the background for a switchbar item. It can paint aero, visual styles, skins or
 * classic buttons (depending on os and current plugin settings).
 *
 * @param hdc     HDC: target device context
 * @param rc      RECT*: target rectangle
 * @param stateId the state identifier (normal, pressed, hot, disabled etc.)
 */
void __fastcall CSideBar::m_DefaultBackgroundRenderer(const HDC hdc, const RECT *rc,
													  const CSideBarButton *item)
{
	UINT	id = item->getID();
	int		stateId = item->m_buttonControl->stateId;
	bool	fIsActiveItem = (item->m_sideBar->getActiveItem() == item);

	if(M->isAero()) {
		if(id == IDC_SIDEBARUP || id == IDC_SIDEBARDOWN) {
			::InflateRect(const_cast<RECT *>(rc), -2, 0);
			if(stateId == PBS_HOT || stateId == PBS_PRESSED)
				DrawAlpha(hdc, const_cast<RECT *>(rc), 0xf0f0f0, 75, 0x000000, 0, 9,
						  31, 4, 0);
			else
				DrawAlpha(hdc, const_cast<RECT *>(rc), 0xf0f0f0, 85, 0x707070, 0, 9,
						  31, 4, 0);
		}
		else {
			CSkin::m_switchBarItem->setAlphaFormat(AC_SRC_ALPHA,
												   (stateId == PBS_HOT && !fIsActiveItem) ? 250 : (fIsActiveItem || stateId == PBS_PRESSED ? 250 : 200));
			CSkin::m_switchBarItem->Render(hdc, rc, true);
			/*

			if(fIsActiveItem) {
				RECT rcBlend = *rc;
				::InflateRect(&rcBlend, -1, -1);
				::DrawAlpha(hdc, &rcBlend, 0xf0f0f0, 255, 0x333333, 0, 9, 31, 8, 0);
			}
			if(stateId == PBS_HOT || stateId == PBS_PRESSED) {
				RECT rcGlow = *rc;
				rcGlow.left +=2;
				rcGlow.right -= 2;
				rcGlow.top += 1;
				rcGlow.bottom -= 10;

				CSkin::m_tabGlowTop->setAlphaFormat(AC_SRC_ALPHA, stateId == PBS_HOT ? 160 : 200);
				CSkin::m_tabGlowTop->Render(hdc, &rcGlow, true);
			}
			*/
			if(stateId == PBS_HOT || stateId == PBS_PRESSED || fIsActiveItem) {
 				RECT rcGlow = *rc;
 				rcGlow.top += 1;
				rcGlow.bottom -= 2;

				CSkin::m_tabGlowTop->setAlphaFormat(AC_SRC_ALPHA, (stateId == PBS_PRESSED || fIsActiveItem) ? 180 : 100);
 				CSkin::m_tabGlowTop->Render(hdc, &rcGlow, true);
 			}
		}
	}
	else if(M->isVSThemed()) {
		RECT *rcDraw = const_cast<RECT *>(rc);
		if(!(id == IDC_SIDEBARUP || id == IDC_SIDEBARDOWN)) {
			HBRUSH br = (stateId == PBS_HOT && !fIsActiveItem) ? ::GetSysColorBrush(COLOR_BTNSHADOW) : (fIsActiveItem || stateId == PBS_PRESSED ? ::GetSysColorBrush(COLOR_HOTLIGHT) : ::GetSysColorBrush(COLOR_BTNHIGHLIGHT));
			::FillRect(hdc, rc, br);
			::DrawEdge(hdc, rcDraw, (stateId == PBS_HOT && !fIsActiveItem) ? EDGE_ETCHED : (fIsActiveItem || stateId == PBS_PRESSED) ? EDGE_BUMP : EDGE_ETCHED, BF_RECT | BF_SOFT | BF_FLAT);
		}
		else {
			::FillRect(hdc, rc, stateId == PBS_HOT ? ::GetSysColorBrush(COLOR_HOTLIGHT) : ::GetSysColorBrush(COLOR_3DFACE));
			::InflateRect(rcDraw, -2, 0);
			::DrawEdge(hdc, rcDraw, EDGE_ETCHED, BF_SOFT|BF_RECT|BF_FLAT);
		}

	}
	else {
		RECT *rcDraw = const_cast<RECT *>(rc);
		if(!(id == IDC_SIDEBARUP || id == IDC_SIDEBARDOWN)) {
			HBRUSH br = (stateId == PBS_HOT && !fIsActiveItem) ? ::GetSysColorBrush(COLOR_BTNSHADOW) : (fIsActiveItem || stateId == PBS_PRESSED ? ::GetSysColorBrush(COLOR_HOTLIGHT) : ::GetSysColorBrush(COLOR_3DFACE));
			::FillRect(hdc, rc, br);
			::DrawEdge(hdc, rcDraw, (stateId == PBS_HOT && !fIsActiveItem) ? EDGE_ETCHED : (fIsActiveItem || stateId == PBS_PRESSED) ? EDGE_BUMP : EDGE_ETCHED, BF_RECT | BF_SOFT | BF_FLAT);
		}
		else {
			::FillRect(hdc, rc, stateId == PBS_HOT ? ::GetSysColorBrush(COLOR_HOTLIGHT) : ::GetSysColorBrush(COLOR_3DFACE));
			::InflateRect(rcDraw, -2, 0);
			::DrawEdge(hdc, rcDraw, EDGE_ETCHED, BF_SOFT|BF_RECT|BF_FLAT);
		}
	}
}

void __fastcall CSideBar::m_DefaultContentRenderer(const HDC hdc, const RECT *rcBox,
												   const CSideBarButton *item)
{
	UINT 						id = item->getID();
	const _MessageWindowData* 	dat = item->getDat();
	int	  						stateID = item->m_buttonControl->stateId;

	LONG	cx = rcBox->right - rcBox->left;
	LONG	cy = rcBox->bottom - rcBox->top;

	if(id == IDC_SIDEBARUP || id == IDC_SIDEBARDOWN) {
		::DrawIconEx(hdc, (rcBox->left + rcBox->right) / 2 - 8, (rcBox->top + rcBox->bottom) / 2 - 8, id == IDC_SIDEBARUP ? PluginConfig.g_buttonBarIcons[26] : PluginConfig.g_buttonBarIcons[16],
					 16, 16, 0, 0, DI_NORMAL);
		if(!M->isAero() && stateID == PBS_HOT)
			::DrawEdge(hdc, const_cast<RECT *>(rcBox), BDR_INNER, BF_RECT | BF_SOFT | BF_FLAT);
	}
	else if(dat)
		item->renderIconAndNick(hdc, rcBox);
}
