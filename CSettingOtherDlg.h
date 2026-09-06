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

	// 現時点では保存対象のコントロールが無いため何もしない
	// (将来このタブに設定項目を追加した際に実装する)。
	void LoadFromConfig(const CString& configPath);
	void SaveToConfig(const CString& configPath);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	// このページは CSettingDlg に埋め込まれる子ウィンドウであり、
	// DoModal() では表示されない。リソース上に誤って IDOK/IDCANCEL の
	// ボタンが残っていた場合でも EndDialog() が呼ばれないようにする保険。
	virtual void OnOK();
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
};
