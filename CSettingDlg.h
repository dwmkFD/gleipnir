#pragma once
#include "afxdialogex.h"
#include "CSettingCommonDlg.h"
#include "CSettingOtherDlg.h"

// CSettingDlg.h : ヘッダー ファイル
//
// 設定ダイアログ本体。IDD_SETTING_DIALOG に対応する。
// IDC_SETTING_DIALOG_TAB のタブコントロールに、
// 「共通」(CSettingCommonDlg / IDD_SETTING_DIALOG_COMMON) と
// 「その他」(CSettingOtherDlg / IDD_SETTING_DIALOG_OTHER) の
// 2ページを子ダイアログとして埋め込み、タブ選択に応じて
// 表示/非表示を切り替える。
//
// gleipnirDlg 側から「/option」入力で DoModal() 表示される想定。
// OKで閉じると各ページの設定値を data\config.ini へ保存する。


// CSettingDlg ダイアログ

class CSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingDlg)

public:
	CSettingDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~CSettingDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTING_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	CTabCtrl m_tabCtrl;
	CSettingCommonDlg m_pageCommon;
	CSettingOtherDlg m_pageOther;

	// タブ内にページを埋め込む位置(タブコントロールの表示領域)を計算し、
	// 2つの子ダイアログをそこに配置・生成する。生成後、data\config.ini
	// があれば読み込んで各ページのコントロールへ反映する。
	void CreateTabPages();
	// 現在選択されているタブに応じて、表示するページを切り替える。
	void UpdatePageVisibility();

	// data\config.ini のパスを返す(gleipnir.exeのフォルダ基準。
	// CgleipnirDlg::GetDataFolderPath() と同じ規則)。
	static CString GetConfigFilePath();

	afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};
