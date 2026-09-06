// CSettingDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "gleipnir.h"
#include "afxdialogex.h"
#include "CSettingDlg.h"


// CSettingDlg ダイアログ

IMPLEMENT_DYNAMIC(CSettingDlg, CDialogEx)

CSettingDlg::CSettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTING_DIALOG, pParent)
	, m_pageCommon(nullptr)
	, m_pageOther(nullptr)
{

}

CSettingDlg::~CSettingDlg()
{
}

void CSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTING_DIALOG_TAB, m_tabCtrl);
}


BEGIN_MESSAGE_MAP(CSettingDlg, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_SETTING_DIALOG_TAB, &CSettingDlg::OnTabSelChange)
END_MESSAGE_MAP()


// CSettingDlg メッセージ ハンドラー

BOOL CSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_tabCtrl.InsertItem(0, _T("共通"));
	m_tabCtrl.InsertItem(1, _T("その他"));

	CreateTabPages();
	UpdatePageVisibility();

	return TRUE;
}

void CSettingDlg::OnOK()
{
	// OKで閉じる際に、各ページのコントロール値を config.ini へ保存する。
	CString configPath = GetConfigFilePath();
	m_pageCommon.SaveToConfig(configPath);
	m_pageOther.SaveToConfig(configPath);

	CDialogEx::OnOK();
}

void CSettingDlg::CreateTabPages()
{
	// タブコントロールの位置(ダイアログのクライアント座標)を取得する。
	CRect tabRect;
	m_tabCtrl.GetWindowRect(&tabRect);
	ScreenToClient(&tabRect);

	// タブの見出し部分を除いた表示領域を計算する。
	// AdjustRect はタブコントロール自身のクライアント座標系(左上原点)で
	// 計算結果を返すため、後でタブコントロールの位置(tabRect.left/top)を
	// 加算してダイアログのクライアント座標系に変換する。
	CRect displayRect(0, 0, tabRect.Width(), tabRect.Height());
	m_tabCtrl.AdjustRect(FALSE, &displayRect);
	displayRect.OffsetRect(tabRect.left, tabRect.top);

	// 「共通」ページ(IDD_SETTING_DIALOG_COMMON)を、タブと同じ親(このダイアログ)の
	// 子ウィンドウとして生成し、表示領域に重ねて配置する。
	// ※ IDD_SETTING_DIALOG_COMMON / IDD_SETTING_DIALOG_OTHER のリソース側で
	//   Style=Child, Border=None, Title Bar=なし に設定しておく必要がある。
	//   (別ウィンドウとして浮いてしまう場合はこの設定を確認してください)
	m_pageCommon.Create(IDD_SETTING_DIALOG_COMMON, this);
	m_pageCommon.SetWindowPos(nullptr,
		displayRect.left, displayRect.top, displayRect.Width(), displayRect.Height(),
		SWP_NOZORDER);

	m_pageOther.Create(IDD_SETTING_DIALOG_OTHER, this);
	m_pageOther.SetWindowPos(nullptr,
		displayRect.left, displayRect.top, displayRect.Width(), displayRect.Height(),
		SWP_NOZORDER);

	// data\config.ini があれば読み込み、各ページのコントロールへ反映する。
	CString configPath = GetConfigFilePath();
	m_pageCommon.LoadFromConfig(configPath);
	m_pageOther.LoadFromConfig(configPath);
}

void CSettingDlg::UpdatePageVisibility()
{
	int sel = m_tabCtrl.GetCurSel();

	m_pageCommon.ShowWindow(sel == 0 ? SW_SHOW : SW_HIDE);
	m_pageOther.ShowWindow(sel == 1 ? SW_SHOW : SW_HIDE);
}

void CSettingDlg::OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);

	UpdatePageVisibility();

	*pResult = 0;
}

// static
CString CSettingDlg::GetConfigFilePath()
{
	// gleipnir.exe と同じフォルダの data フォルダ以下に保存する
	// (CgleipnirDlg::GetDataFolderPath() と同じ規則。クラスをまたぐ
	//  依存を避けるため、ここでは同じロジックを簡潔に複製している)。
	TCHAR exePath[MAX_PATH] = { 0 };
	::GetModuleFileName(nullptr, exePath, MAX_PATH);

	CString dir = exePath;
	int pos = dir.ReverseFind(_T('\\'));
	if (pos >= 0)
	{
		dir = dir.Left(pos);
	}
	dir += _T("\\data");

	::CreateDirectory(dir, nullptr); // 既に存在する場合のエラーは無視してよい

	return dir + _T("\\config.ini");
}
