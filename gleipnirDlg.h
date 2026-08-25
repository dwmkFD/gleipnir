
// gleipnirDlg.h : ヘッダー ファイル
//

#pragma once

// タスクトレイアイコンのコールバックメッセージ
#define WM_TRAYICON (WM_APP + 1)

// 低レベルキーボードフックから Caps Lock 検知を通知するためのメッセージ
#define WM_CAPSLOCK_TOGGLE (WM_APP + 2)

// CgleipnirDlg ダイアログ
class CgleipnirDlg : public CDialogEx
{
// コンストラクション
public:
	CgleipnirDlg(CWnd* pParent = nullptr);	// 標準コンストラクター

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GLEIPNIR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート


// 実装
protected:
	HICON m_hIcon;
	NOTIFYICONDATA m_nid;		// タスクトレイアイコン情報
	bool m_bTrayIconAdded;		// タスクトレイアイコンが追加済みかどうか

	void AddTrayIcon();
	void RemoveTrayIcon();
	void ToggleVisibility();	// ウィンドウの表示/非表示を切り替える

	// Caps Lock を横取りするための低レベルキーボードフック。
	// RegisterHotKey は、他アプリ(オリジナルのFenrir等)が低レベルフックで
	// Caps Lock を先に消費してしまうケースで発火しないため、
	// 同じ低レベルフックの仕組みで対抗する。
	static HHOOK s_hKeyboardHook;
	static HWND s_hMainWnd;
	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCapsLockToggle(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTrayExit();
	DECLARE_MESSAGE_MAP()
};
