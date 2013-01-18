// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.


// MfcTimeFliesLikeAnArrow.h : main header file for the MfcTimeFliesLikeAnArrow application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CMfcTimeFliesLikeAnArrowApp:
// See MfcTimeFliesLikeAnArrow.cpp for the implementation of this class
//

class CMfcTimeFliesLikeAnArrowApp : public CWinApp
{
public:
	CMfcTimeFliesLikeAnArrowApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CMfcTimeFliesLikeAnArrowApp theApp;
