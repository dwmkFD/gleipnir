// CSettingOtherDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "gleipnir.h"
#include "afxdialogex.h"
#include "CSettingOtherDlg.h"


// CSettingOtherDlg ダイアログ

IMPLEMENT_DYNAMIC(CSettingOtherDlg, CDialogEx)

CSettingOtherDlg::CSettingOtherDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTING_DIALOG_OTHER, pParent)
{

}

CSettingOtherDlg::~CSettingOtherDlg()
{
}

void CSettingOtherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSettingOtherDlg, CDialogEx)
END_MESSAGE_MAP()


// CSettingOtherDlg メッセージ ハンドラー
