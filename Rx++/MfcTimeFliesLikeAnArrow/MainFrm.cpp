// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.


// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "MfcTimeFliesLikeAnArrow.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
//  ON_WM_SETFOCUS()
    ON_WM_CLOSE()
    ON_WM_MOUSEMOVE()
    ON_WM_PAINT()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}


// inspired by: http://minirx.codeplex.com/
void CMainFrame::UserInit()
{
    auto mouseMove = BindEventToObservable(mouseMoveEvent);

    // set up labels and query
    auto msg = L"Time flies like an arrow";

    for (int i = 0; msg[i]; ++i)
    {
        auto label = CreateLabelFromLetter(msg[i], this);

        // note: distinct_until_changed is necessary; while 
        //  Winforms filters duplicate mouse moves, 
        //  user32 doesn't filter this for you: http://blogs.msdn.com/b/oldnewthing/archive/2003/10/01/55108.aspx

        auto s = rxcpp::from(mouseMove)
            .select([](MouseMoveEventValue e) { return e.point; })
            .distinct_until_changed()   
            .delay(i * 100 + 1)
            .on_dispatcher()
            .subscribe([=](CPoint point) 
            {
                label->SetWindowPos(nullptr, point.x+20*i, point.y-20, 20, 30, SWP_NOOWNERZORDER);
                label->Invalidate();

                // repaint early, for fluid animation
                label->UpdateWindow();
                this->UpdateWindow();
            });

        composableDisposable.Add(s);
    }
}

void CMainFrame::OnClose()
{
    // TODO: figure out cause of crash -- until then, exit instead
    ::ExitProcess(0);

    // shutdown subscription.
    composableDisposable.Dispose();
    
    CFrameWnd::OnClose();
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

    UserInit();

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW));
	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	//m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	//if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	//	return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnMouseMove(UINT nFlags, CPoint point)
{
    MouseMoveEventValue v = {nFlags, point};
    this->mouseMoveEvent(v);
}

void CMainFrame::OnPaint( )
{
    CPaintDC dc(this); // device context for painting
	
    RECT rc;
    this->GetClientRect(&rc);
    CBrush brush;
    brush.CreateSolidBrush(RGB(240,240,240));
    dc.FillRect(&rc, &brush);
}

