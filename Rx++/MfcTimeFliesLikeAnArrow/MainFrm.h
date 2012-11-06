// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.


// MainFrm.h : interface of the CMainFrame class
//

#pragma once


template <class T>
static std::shared_ptr<rxcpp::Observable<T>>
    BindEventToObservable(std::function<void(T)>& event)
{
    auto eSubject = rxcpp::CreateSubject<T>();
    event = [=](const T& eventValue) {
        eSubject->OnNext(eventValue);
    };
    return eSubject;
}

inline CStatic* CreateLabelFromLetter(wchar_t c, CWnd* parent)
{
    CStatic* label;
    label = new CStatic();
        
    RECT r = { 0, 0, };
    r.right = r.left+20;
    r.bottom = r.top + 30;

    auto message = new std::wstring(&c, &c+1);
    label->Create(
        message->c_str(), 
        WS_CHILD | WS_VISIBLE,
        r,
        parent);
    return label;
}

class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar        m_wndStatusBar;

    struct MouseMoveEventValue
    {
        UINT nFlags;
        CPoint point;
    };

    std::function<void(MouseMoveEventValue)> mouseMoveEvent;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
    afx_msg void OnClose();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnPaint( );

	DECLARE_MESSAGE_MAP()

    void UserInit();
    rxcpp::ComposableDisposable composableDisposable;
};


