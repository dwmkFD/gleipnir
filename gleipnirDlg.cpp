
// gleipnirDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "gleipnir.h"
#include "gleipnirDlg.h"
#include "afxdialogex.h"
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CgleipnirDlg ダイアログ

HHOOK CgleipnirDlg::s_hKeyboardHook = nullptr;
HWND CgleipnirDlg::s_hMainWnd = nullptr;

CgleipnirDlg::CgleipnirDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GLEIPNIR_DIALOG, pParent)
	, m_bTrayIconAdded(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	ZeroMemory(&m_nid, sizeof(m_nid));
}

void CgleipnirDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CgleipnirDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SYSCOMMAND()
	ON_MESSAGE(WM_TRAYICON, &CgleipnirDlg::OnTrayIcon)
	ON_MESSAGE(WM_CAPSLOCK_TOGGLE, &CgleipnirDlg::OnCapsLockToggle)
	ON_COMMAND(ID_TRAY_EXIT, &CgleipnirDlg::OnTrayExit)
END_MESSAGE_MAP()


// CgleipnirDlg メッセージ ハンドラー

BOOL CgleipnirDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// このダイアログのアイコンを設定します。
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// タスクトレイにアイコンを常駐させる
	AddTrayIcon();

	// Caps Lock を横取りするための低レベルキーボードフックを設置する。
	// RegisterHotKey は、他アプリ(オリジナルのFenrir等)が低レベルフックで
	// Caps Lock を先に消費してしまうと発火しないため使用しない。
	s_hMainWnd = m_hWnd;
	s_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, AfxGetInstanceHandle(), 0);
	if (s_hKeyboardHook == nullptr)
	{
		TRACE(traceAppMsg, 0, "警告: キーボードフックの設置に失敗しました。\n");
	}

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CgleipnirDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CgleipnirDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CgleipnirDlg::OnDestroy()
{
	// キーボードフックの解除とタスクトレイアイコンの削除
	if (s_hKeyboardHook != nullptr)
	{
		UnhookWindowsHookEx(s_hKeyboardHook);
		s_hKeyboardHook = nullptr;
	}
	s_hMainWnd = nullptr;

	RemoveTrayIcon();

	CDialogEx::OnDestroy();
}

void CgleipnirDlg::OnClose()
{
	// ×ボタン等でウィンドウを閉じる操作では、アプリを終了させずタスクトレイに隠す。
	// 終了はタスクトレイアイコンの右クリックメニューから行う。
	ShowWindow(SW_HIDE);
}

void CgleipnirDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// 最小化操作もタスクトレイへの格納として扱う
	if ((nID & 0xFFF0) == SC_MINIMIZE)
	{
		ShowWindow(SW_HIDE);
		return;
	}

	CDialogEx::OnSysCommand(nID, lParam);
}

LRESULT CgleipnirDlg::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);

	switch (lParam)
	{
	case WM_LBUTTONDBLCLK:
		ToggleVisibility();
		break;

	case WM_RBUTTONUP:
	{
		CMenu menu;
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, ID_TRAY_EXIT, _T("終了(&X)"));

		CPoint pt;
		GetCursorPos(&pt);

		// ポップアップメニューを正しく表示・終了させるための定石処理
		SetForegroundWindow();
		menu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
		PostMessage(WM_NULL, 0, 0);
		break;
	}
	}

	return 0;
}

LRESULT CgleipnirDlg::OnCapsLockToggle(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	ToggleVisibility();
	return 0;
}

void CgleipnirDlg::OnTrayExit()
{
	EndDialog(IDCANCEL);
}

void CgleipnirDlg::AddTrayIcon()
{
	ZeroMemory(&m_nid, sizeof(m_nid));
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.hWnd = m_hWnd;
	m_nid.uID = 1;
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_nid.uCallbackMessage = WM_TRAYICON;
	m_nid.hIcon = m_hIcon;
	_tcscpy_s(m_nid.szTip, _T("gleipnir"));

	if (Shell_NotifyIcon(NIM_ADD, &m_nid))
	{
		m_bTrayIconAdded = true;
	}
}

void CgleipnirDlg::RemoveTrayIcon()
{
	if (m_bTrayIconAdded)
	{
		Shell_NotifyIcon(NIM_DELETE, &m_nid);
		m_bTrayIconAdded = false;
	}
}

void CgleipnirDlg::ToggleVisibility()
{
	if (IsWindowVisible())
	{
		ShowWindow(SW_HIDE);
	}
	else
	{
		ShowWindow(SW_SHOW);
		SetForegroundWindow();
	}
}

// static
LRESULT CALLBACK CgleipnirDlg::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && s_hMainWnd != nullptr)
	{
		const KBDLLHOOKSTRUCT* pkbhs = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);

		if (pkbhs->vkCode == VK_CAPITAL)
		{
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
			{
				// 表示/非表示の切り替え処理はメインウィンドウにポストするのみとし、
				// フック内では即座に戻る(フック処理は極力軽量にする定石に従う)。
				::PostMessage(s_hMainWnd, WM_CAPSLOCK_TOGGLE, 0, 0);
			}

			// キーダウン・キーアップともにOSへ伝えず、Caps Lockのロック状態自体の
			// トグルを抑止する。戻り値を1にすることで、これ以降のフックチェーンにも
			// イベントを渡さない。
			return 1;
		}
	}

	return CallNextHookEx(s_hKeyboardHook, nCode, wParam, lParam);
}
