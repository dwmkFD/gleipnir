// CSettingCommonDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "gleipnir.h"
#include "afxdialogex.h"
#include "CSettingCommonDlg.h"
#include "gleipnirDlg.h"   // SearchMethod と config.ini 値の変換

namespace
{
	// コンボの並び(番号)と検索方式の対応
	const SearchMethod kSearchMethodsByIndex[] = {
		SearchMethod::Prefix,     // 0: 先頭部分一致
		SearchMethod::Substring,  // 1: 中間部分一致
		SearchMethod::Skip,       // 2: スキップマッチング
	};

	int SearchMethodToIndex(SearchMethod method)
	{
		for (int i = 0; i < static_cast<int>(_countof(kSearchMethodsByIndex)); ++i)
		{
			if (kSearchMethodsByIndex[i] == method)
			{
				return i;
			}
		}
		return 1;
	}

	SearchMethod IndexToSearchMethod(int index, SearchMethod defaultMethod)
	{
		return (index >= 0 && index < static_cast<int>(_countof(kSearchMethodsByIndex))) ? kSearchMethodsByIndex[index] : defaultMethod;
	}
}


// CSettingCommonDlg ダイアログ

IMPLEMENT_DYNAMIC(CSettingCommonDlg, CDialogEx)

CSettingCommonDlg::CSettingCommonDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTING_DIALOG_COMMON, pParent)
	, m_maxEnumCount(50)
	, m_autoDeleteBrokenPath(FALSE)
	, m_candidateIconSel(0)
	, m_searchMethodNormalSel(1)
	, m_searchMethodFolderExpandSel(2)
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
	DDX_CBIndex(pDX, IDC_COMBO1, m_searchMethodNormalSel);
	DDX_CBIndex(pDX, IDC_COMBO2, m_searchMethodFolderExpandSel);
	DDX_CBIndex(pDX, IDC_COMBO3, m_candidateIconSel);
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

	// 検索方式: 3択(並びは kSearchMethodsByIndex と対応させる)。
	const UINT searchCombos[] = { IDC_COMBO1, IDC_COMBO2 };
	for (UINT id : searchCombos)
	{
		CComboBox* pCombo = static_cast<CComboBox*>(GetDlgItem(id));
		if (pCombo != nullptr)
		{
			pCombo->ResetContent();
			pCombo->AddString(_T("先頭部分一致"));       // 0
			pCombo->AddString(_T("中間部分一致"));       // 1
			pCombo->AddString(_T("スキップマッチング")); // 2
		}
	}

	// 候補リストのアイコン: 表示/非表示の二択(並びは CBIndex と対応させる)。
	CComboBox* pIconCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO3));
	if (pIconCombo != nullptr)
	{
		pIconCombo->ResetContent();
		pIconCombo->AddString(_T("表示"));   // 0
		pIconCombo->AddString(_T("非表示")); // 1
		pIconCombo->SetCurSel(0);
	}

	// ビジュアル欄の書式を、空欄時の薄字ヒント(キューバナー)で示す。
	// 空欄のままなら既定値(緑文字・黒背景・メイリオ太字)が使われる。
	struct CueBanner { UINT id; LPCWSTR text; };
	const CueBanner cues[] = {
		{ IDC_EDIT7, L"#00FF00 または 0,255,0" },
		{ IDC_EDIT8, L"#000000 または 0,0,0" },
		{ IDC_EDIT9, L"Meiryo,10,bold" },
	};
	for (const CueBanner& cue : cues)
	{
		CEdit* pEdit = static_cast<CEdit*>(GetDlgItem(cue.id));
		if (pEdit != nullptr)
		{
			pEdit->SetCueBanner(cue.text, TRUE);
		}
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
	m_searchMethodNormalSel = SearchMethodToIndex(CgleipnirDlg::ParseSearchMethod(buf, SearchMethod::Substring));

	::GetPrivateProfileString(_T("Common"), _T("SearchMethodFolderExpand"), _T(""), buf, _countof(buf), configPath);
	m_searchMethodFolderExpandSel = SearchMethodToIndex(CgleipnirDlg::ParseSearchMethod(buf, SearchMethod::Skip));

	// "0"/"非表示" のときだけ非表示。未設定・その他の値は既定の「表示」。
	::GetPrivateProfileString(_T("Common"), _T("CandidateIcon"), _T("1"), buf, _countof(buf), configPath);
	{
		CString v = buf;
		v.Trim();
		m_candidateIconSel = (v == _T("0") || v == _T("非表示")) ? 1 : 0;
	}

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
	::WritePrivateProfileString(_T("Common"), _T("SearchMethodNormal"),
		CgleipnirDlg::SearchMethodToConfigValue(IndexToSearchMethod(m_searchMethodNormalSel, SearchMethod::Substring)), configPath);
	::WritePrivateProfileString(_T("Common"), _T("SearchMethodFolderExpand"),
		CgleipnirDlg::SearchMethodToConfigValue(IndexToSearchMethod(m_searchMethodFolderExpandSel, SearchMethod::Skip)), configPath);
	::WritePrivateProfileString(_T("Common"), _T("CandidateIcon"), (m_candidateIconSel == 1) ? _T("0") : _T("1"), configPath);

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
