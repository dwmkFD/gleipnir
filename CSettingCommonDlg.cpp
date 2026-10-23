// CSettingCommonDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "gleipnir.h"
#include "afxdialogex.h"
#include "CSettingCommonDlg.h"


// CSettingCommonDlg ダイアログ

IMPLEMENT_DYNAMIC(CSettingCommonDlg, CDialogEx)

CSettingCommonDlg::CSettingCommonDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTING_DIALOG_COMMON, pParent)
	, m_maxEnumCount(50)
	, m_autoDeleteBrokenPath(FALSE)
	, m_clearInputOnHide(TRUE)
{

}

CSettingCommonDlg::~CSettingCommonDlg()
{
}

void CSettingCommonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT2, m_folderExpandKey);
	DDX_Text(pDX, IDC_EDIT3, m_parentFolderExpandKey);
	DDX_Text(pDX, IDC_EDIT4, m_selectDownKey);
	DDX_Text(pDX, IDC_EDIT5, m_selectUpKey);
	DDX_CBString(pDX, IDC_COMBO1, m_searchMethodNormal);
	DDX_CBString(pDX, IDC_COMBO2, m_searchMethodFolderExpand);
	DDX_CBString(pDX, IDC_COMBO3, m_candidateIcon);
	DDX_Text(pDX, IDC_EDIT6, m_maxEnumCount);
	DDV_MinMaxInt(pDX, m_maxEnumCount, 0, 100000);
	DDX_Check(pDX, IDC_CHECK1, m_autoDeleteBrokenPath);
	DDX_Text(pDX, IDC_EDIT7, m_textColor);
	DDX_Text(pDX, IDC_EDIT8, m_backgroundColor);
	DDX_Text(pDX, IDC_EDIT9, m_fontName);
}


BEGIN_MESSAGE_MAP(CSettingCommonDlg, CDialogEx)
END_MESSAGE_MAP()


// CSettingCommonDlg メッセージ ハンドラー

BOOL CSettingCommonDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 呼び出しキーは現状 Caps Lock 固定のため、設定項目としては用意しつつも
	// 変更不可(無効化)として表示のみ行う(保存対象にも含めない)。
	SetDlgItemText(IDC_EDIT_HOTKEY, _T("Caps Lock"));
	CWnd* pHotKeyWnd = GetDlgItem(IDC_EDIT_HOTKEY);
	if (pHotKeyWnd != nullptr)
	{
		pHotKeyWnd->EnableWindow(FALSE);
	}

	return TRUE;
}

void CSettingCommonDlg::LoadFromConfig(const CString& configPath)
{
	TCHAR buf[512] = { 0 };

	::GetPrivateProfileString(_T("Common"), _T("FolderExpandKey"), _T(""), buf, _countof(buf), configPath);
	m_folderExpandKey = buf;

	::GetPrivateProfileString(_T("Common"), _T("ParentFolderExpandKey"), _T(""), buf, _countof(buf), configPath);
	m_parentFolderExpandKey = buf;

	::GetPrivateProfileString(_T("Common"), _T("SelectDownKey"), _T(""), buf, _countof(buf), configPath);
	m_selectDownKey = buf;

	::GetPrivateProfileString(_T("Common"), _T("SelectUpKey"), _T(""), buf, _countof(buf), configPath);
	m_selectUpKey = buf;

	::GetPrivateProfileString(_T("Common"), _T("SearchMethodNormal"), _T(""), buf, _countof(buf), configPath);
	m_searchMethodNormal = buf;

	::GetPrivateProfileString(_T("Common"), _T("SearchMethodFolderExpand"), _T(""), buf, _countof(buf), configPath);
	m_searchMethodFolderExpand = buf;

	::GetPrivateProfileString(_T("Common"), _T("CandidateIcon"), _T(""), buf, _countof(buf), configPath);
	m_candidateIcon = buf;

	m_maxEnumCount = ::GetPrivateProfileInt(_T("Common"), _T("MaxEnumCount"), 50, configPath);
	m_autoDeleteBrokenPath = ::GetPrivateProfileInt(_T("Common"), _T("AutoDeleteBrokenPath"), 0, configPath) ? TRUE : FALSE;

	::GetPrivateProfileString(_T("Common"), _T("TextColor"), _T(""), buf, _countof(buf), configPath);
	m_textColor = buf;

	::GetPrivateProfileString(_T("Common"), _T("BackgroundColor"), _T(""), buf, _countof(buf), configPath);
	m_backgroundColor = buf;

	::GetPrivateProfileString(_T("Common"), _T("Font"), _T(""), buf, _countof(buf), configPath);
	m_fontName = buf;

	m_clearInputOnHide = ::GetPrivateProfileInt(_T("Common"), _T("ClearInputOnHide"), 1, configPath) ? TRUE : FALSE;

	// 読み込んだ値をコントロールへ反映する。
	UpdateData(FALSE);
}

void CSettingCommonDlg::SaveToConfig(const CString& configPath)
{
	// コントロールの現在値をメンバー変数へ取り込む。
	UpdateData(TRUE);

	::WritePrivateProfileString(_T("Common"), _T("FolderExpandKey"), m_folderExpandKey, configPath);
	::WritePrivateProfileString(_T("Common"), _T("ParentFolderExpandKey"), m_parentFolderExpandKey, configPath);
	::WritePrivateProfileString(_T("Common"), _T("SelectDownKey"), m_selectDownKey, configPath);
	::WritePrivateProfileString(_T("Common"), _T("SelectUpKey"), m_selectUpKey, configPath);
	::WritePrivateProfileString(_T("Common"), _T("SearchMethodNormal"), m_searchMethodNormal, configPath);
	::WritePrivateProfileString(_T("Common"), _T("SearchMethodFolderExpand"), m_searchMethodFolderExpand, configPath);
	::WritePrivateProfileString(_T("Common"), _T("CandidateIcon"), m_candidateIcon, configPath);

	CString numBuf;
	numBuf.Format(_T("%d"), m_maxEnumCount);
	::WritePrivateProfileString(_T("Common"), _T("MaxEnumCount"), numBuf, configPath);

	numBuf.Format(_T("%d"), m_autoDeleteBrokenPath ? 1 : 0);
	::WritePrivateProfileString(_T("Common"), _T("AutoDeleteBrokenPath"), numBuf, configPath);

	::WritePrivateProfileString(_T("Common"), _T("TextColor"), m_textColor, configPath);
	::WritePrivateProfileString(_T("Common"), _T("BackgroundColor"), m_backgroundColor, configPath);
	::WritePrivateProfileString(_T("Common"), _T("Font"), m_fontName, configPath);

	numBuf.Format(_T("%d"), m_clearInputOnHide ? 1 : 0);
	::WritePrivateProfileString(_T("Common"), _T("ClearInputOnHide"), numBuf, configPath);
}
