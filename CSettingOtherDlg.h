#pragma once
#include "afxdialogex.h"


// CSettingOtherDlg ダイアログ

class CSettingOtherDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingOtherDlg)

public:
	CSettingOtherDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~CSettingOtherDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTING_DIALOG_OTHER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
