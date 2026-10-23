#pragma once
#include "afxdialogex.h"


// CSettingCommonDlg ダイアログ

class CSettingCommonDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingCommonDlg)

public:
	CSettingCommonDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~CSettingCommonDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTING_DIALOG_COMMON };
#endif

	// data\config.ini から値を読み込み、コントロールへ反映する。
	void LoadFromConfig(const CString& configPath);
	// コントロールの現在値を data\config.ini へ書き込む。
	void SaveToConfig(const CString& configPath);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

	// [検索方式]・[候補リスト]・[インデックスデータ]・[ビジュアル] 各項目。
	// 「呼び出しキー」欄(IDC_EDIT_HOTKEY)は現状Caps Lock固定のため、
	// 変更不可(無効化)として表示のみ行い、保存対象には含めない。
	CString m_folderExpandKey;          // フォルダ展開キー
	CString m_parentFolderExpandKey;    // 親フォルダ展開キー
	CString m_selectDownKey;            // 下選択キー
	CString m_selectUpKey;              // 上選択キー
	CString m_searchMethodNormal;       // 検索方式(通常時)
	CString m_searchMethodFolderExpand; // 検索方式(フォルダ展開時)
	int     m_candidateIconSel;         // 候補リストのアイコン(0=表示, 1=非表示。config.ini には 1/0 で保存)
	int     m_maxEnumCount;             // 候補リストの最大列挙数
	BOOL    m_autoDeleteBrokenPath;     // リンク切れパスの自動削除
	CString m_textColor;                // 文字色
	CString m_backgroundColor;          // 背景色
	CString m_fontName;                 // フォント
	// 非表示にした時点で入力中の文字列・候補をクリアするか(既定: TRUE)。
	// 画面リソース未作成のため、現状はconfig.iniの読み書きのみ(DDX未接続)。
	// リソース追加後、DoDataExchangeに DDX_Check(pDX, <チェックボックスID>,
	// m_clearInputOnHide); を追加すれば画面と連動する。
	BOOL    m_clearInputOnHide;

	DECLARE_MESSAGE_MAP()
};
