
// gleipnir.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'pch.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CgleipnirApp:
// このクラスの実装については、gleipnir.cpp を参照してください
//

class CgleipnirApp : public CWinApp
{
public:
	CgleipnirApp();

// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CgleipnirApp theApp;
