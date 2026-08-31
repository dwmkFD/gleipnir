#pragma once
#include "afxdialogex.h"

// SettingCommonDlg.h : ヘッダー ファイル
//
// 設定ダイアログ(CSettingDlg)のタブコントロールに埋め込む
// 「共通」タブのページ。IDD_SETTING_DIALOG_COMMON に対応する。
//
// 現時点ではまだ具体的な設定項目の機能は実装していない(箱のみ)。
// 中に配置するコントロール(IDC_EDIT2 等)が確定したら、
// DoDataExchange に DDX_Text 等を追加して値の保存/読み込みを実装する想定。

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

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
};
