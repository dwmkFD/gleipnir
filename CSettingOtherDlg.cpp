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

void CSettingOtherDlg::OnOK()
{
	// 埋め込みページのため何もしない(EndDialogを呼ばない)。
}

void CSettingOtherDlg::OnCancel()
{
	// 埋め込みページのため何もしない(EndDialogを呼ばない)。
}

void CSettingOtherDlg::LoadFromConfig(const CString& configPath)
{
	UNREFERENCED_PARAMETER(configPath);
	// このタブには現時点で保存対象のコントロールが無い。
}

void CSettingOtherDlg::SaveToConfig(const CString& configPath)
{
	UNREFERENCED_PARAMETER(configPath);
	// このタブには現時点で保存対象のコントロールが無い。
}
