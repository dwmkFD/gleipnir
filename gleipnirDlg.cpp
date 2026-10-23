
// gleipnirDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "gleipnir.h"
#include "gleipnirDlg.h"
#include "afxdialogex.h"
#include "CSettingDlg.h"
#include <shellapi.h>
#include <memory>
#include <set>

#pragma comment(lib, "shell32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	// Caps Lock 物理キーのスキャンコード(スキャンコードセット1)。
	// US配列/JIS配列共通のハードウェア値であり、キーボードレイアウト
	// ドライバによる仮想キーへの変換の影響を受けない。
	//
	// 注意: JIS配列キーボードの「Caps Lock 英数」キーは、単独押下では
	// VK_CAPITAL ではなく VK_DBE_ALPHANUMERIC (IME用の仮想キー) に
	// 変換されてしまうため、vkCode ではなく scanCode で判定している。
	constexpr WORD SCANCODE_CAPSLOCK = 0x3A;

	// 検索結果リストボックスに表示する最大件数(パフォーマンス配慮)。
	constexpr int kMaxDisplayResults = 200;

	// Edit Control にこの文字列(大文字小文字区別なし)が入力されたら
	// スキャンを実行する。
	// Edit Control にこれらの文字列(大文字小文字区別なし)が入力されたら
	// スキャンの開始/キャンセルを行う。
	constexpr LPCTSTR kScanCommand = _T("/scan");             // 単体でも開始扱い(後方互換)
	constexpr LPCTSTR kScanStartCommand = _T("/scan start");
	constexpr LPCTSTR kScanStopCommand = _T("/scan stop");
	constexpr LPCTSTR kScanCancelCommand = _T("/scan cancel");

	// Edit Control にこの文字列(大文字小文字区別なし)が入力されたら
	// 設定ダイアログを開く。
	constexpr LPCTSTR kOptionCommand = _T("/option");

	// instant.ini が無い場合に作成する既定のテンプレート。
	constexpr LPCTSTR kDefaultInstantIniTemplate =
		_T("[InstantCommand]\r\n")
		_T(";\r\n")
		_T("; [書式]\r\n")
		_T("; 文字列=コマンドライン\r\n")
		_T(";\r\n")
		_T("; 検索欄に「文字列」で始まる入力をしてEnterを押すと、\r\n")
		_T("; 文字列より後ろの部分が %A に差し込まれた上でコマンドラインが実行されます。\r\n")
		_T(";\r\n")
		_T("; このiniファイルを読み込むAPIには、値の両端にあるダブルクォーテーションを\r\n")
		_T("; 自動で1組だけ取り除く仕様があります。そのため、両端にダブルクォーテーションが\r\n")
		_T("; 必要なコマンドラインを指定する場合は、\r\n")
		_T("; /f=\"\"C:\\Program Files\\foo\\foo.exe\"\"\r\n")
		_T("; のように二重にダブルクォーテーションをつける必要があります。\r\n")
		_T(";\r\n")
		_T("; [置換マクロ]\r\n")
		_T("; %A          インスタントコマンドに渡された引数。\r\n")
		_T("; %W          gleipnir.exe のあるフォルダのパス。\r\n")
		_T("; %V          %W のドライブ部分(例: C:)。\r\n")
		_T("; %C          クリップボードに格納されている文字列。\r\n")
		_T("; %scanfile   scan.ini のパス(既定: data\\scan.ini)。\r\n")
		_T("; %pathfile   インデックスファイルのパス(既定: data\\index.txt)。\r\n")
		_T("; %initfile   動作設定ファイルのパス(既定: data\\config.ini)。\r\n")
		_T("; %cmddir     data フォルダのパス。\r\n")
		_T(";\r\n")
		_T("\r\n")
		_T("/n=notepad\r\n")
		_T("/c=calc\r\n")
		_T("/e=explorer\r\n")
		_T("\r\n")
		_T("; クリップボードへのコピー。「/clip xxxx」と入力すると xxxx がコピーされる。\r\n")
		_T("; コマンドラインが @clip で始まる場合、実行せず後続の文字列をコピーする。\r\n")
		_T("; 固定文字列の登録も可能(例: /mail=@clip foo@example.com)。\r\n")
		_T("/clip=@clip %A\r\n")
		_T("\r\n")
		_T("; Google(日本語)で検索語を検索する。「/g 検索語」と入力すると実行される。\r\n")
		_T("/g=\"\"C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe\" \"http://www.google.co.jp/search?ie=utf-8&oe=utf-8&hl=ja&lr=lang_ja&q=%A\"\"\r\n");

	// scan.ini が無い場合に作成する既定のテンプレート。
	constexpr LPCTSTR kDefaultScanIniTemplate =
		_T("[DRIVE]\r\n")
		_T("; スキャン対象ドライブをカンマ区切りで指定します(例: TARGET=C:,D:)。\r\n")
		_T("; このセクション自体が無い、またはTARGETが空の場合はスキャンを実行しません。\r\n")
		_T("TARGET=C:\r\n")
		_T("\r\n")
		_T("[regist]\r\n")
		_T("; インデックスに登録する拡張子をカンマ区切りで指定します(ホワイトリスト方式)。\r\n")
		_T("; 未指定の場合、ファイルは1件も登録されません(フォルダのみ登録されます)。\r\n")
		_T("REGIST=*.txt,*.doc,*.docx,*.xlsx,*.pdf\r\n")
		_T("\r\n")
		_T("[ignore]\r\n")
		_T("; フォルダ名にこの文字列を含む場合、そのフォルダ(配下含む)をスキャン対象外にします。\r\n")
		_T("FOLDERNAME=System Volume Information,$RECYCLE.BIN,node_modules\r\n");

	// フルパスからファイル名部分のみを取り出す。
	CString ExtractFileName(const CString& path)
	{
		int pos = path.ReverseFind(_T('\\'));
		if (pos < 0)
		{
			return path;
		}
		return path.Mid(pos + 1);
	}

	// カンマ区切りの文字列を分割し、各要素の前後の空白を取り除いた
	// 一覧を返す(空要素は除外)。
	std::vector<CString> SplitCommaList(const CString& value)
	{
		std::vector<CString> result;

		int curPos = 0;
		CString token = value.Tokenize(_T(","), curPos);
		while (!token.IsEmpty())
		{
			token.Trim();
			if (!token.IsEmpty())
			{
				result.push_back(token);
			}
			token = value.Tokenize(_T(","), curPos);
		}

		return result;
	}
}

// インデックス作成ワーカースレッドへ渡すパラメータ
struct IndexThreadParam
{
	HWND hMainWnd;
	std::vector<CString> driveRoots;
	std::vector<CString> registerExtensions; // 小文字、ドット付き(例: ".txt")のホワイトリスト
	std::vector<CString> ignoreFolderNames;  // 小文字の部分一致キーワード
	std::atomic<bool>* cancelFlag;           // trueになったらスキャンを中断する
};

// スキャン完了時にワーカースレッドからメインスレッドへ渡す結果一式。
struct IndexCompleteResult
{
	std::vector<FileEntry>* results; // 所有権はメインスレッド側に渡る
	bool canceled;                   // キャンセルにより中断されたか
};

// CgleipnirDlg ダイアログ

HHOOK CgleipnirDlg::s_hKeyboardHook = nullptr;
HWND CgleipnirDlg::s_hMainWnd = nullptr;

CgleipnirDlg::CgleipnirDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GLEIPNIR_DIALOG, pParent)
	, m_bTrayIconAdded(false)
	, m_bIndexing(false)
	, m_scanCancelRequested(false)
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
	ON_WM_CTLCOLOR()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SYSCOMMAND()
	ON_MESSAGE(WM_TRAYICON, &CgleipnirDlg::OnTrayIcon)
	ON_MESSAGE(WM_CAPSLOCK_TOGGLE, &CgleipnirDlg::OnCapsLockToggle)
	ON_MESSAGE(WM_INDEX_COMPLETE, &CgleipnirDlg::OnIndexComplete)
	ON_MESSAGE(WM_INDEX_PROGRESS, &CgleipnirDlg::OnIndexProgress)
	ON_COMMAND(ID_TRAY_EXIT, &CgleipnirDlg::OnTrayExit)
	ON_COMMAND(ID_TRAY_SCAN, &CgleipnirDlg::OnTrayScan)
	ON_COMMAND(ID_TRAY_CANCEL_SCAN, &CgleipnirDlg::OnTrayCancelScan)
	ON_COMMAND(ID_TRAY_EDIT_SCAN_INI, &CgleipnirDlg::OnTrayEditScanIni)
	ON_COMMAND(ID_TRAY_EDIT_INSTANT_INI, &CgleipnirDlg::OnTrayEditInstantIni)
	ON_COMMAND(ID_TRAY_RELOAD_SETTINGS, &CgleipnirDlg::OnTrayReloadSettings)
	ON_COMMAND(ID_TRAY_OPEN_DATA_FOLDER, &CgleipnirDlg::OnTrayOpenDataFolder)
	ON_EN_CHANGE(IDC_EDIT1, &CgleipnirDlg::OnEnChangeEdit1)
	ON_LBN_DBLCLK(IDC_LIST_RESULTS, &CgleipnirDlg::OnLbnDblclkListResults)
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
	s_hMainWnd = m_hWnd;
	s_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, AfxGetInstanceHandle(), 0);
	if (s_hKeyboardHook == nullptr)
	{
		TRACE(traceAppMsg, 0, "警告: キーボードフックの設置に失敗しました。\n");
	}

	// 検索結果リストボックス・スキャン中プログレスバーを動的生成する
	CreateDynamicControls();

	// 前回保存されたインデックスがあれば読み込む(無ければ何もしない。
	// トレイメニューまたは「/scan」入力から手動で作成してもらう)
	LoadIndexFromFile(GetIndexFilePath(), m_fileIndex);

	// 起動履歴があれば読み込む(無ければ何もしない。まだ何も起動していない状態)
	LoadHistoryFromFile(GetHistoryFilePath(), m_launchHistory);

	// instant.ini があれば読み込む(無ければ何もしない)
	LoadInstantCommands();

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CgleipnirDlg::CreateDynamicControls()
{
	CRect editRect;
	GetDlgItem(IDC_EDIT1)->GetWindowRect(&editRect);
	ScreenToClient(&editRect);

	CRect clientRect;
	GetClientRect(&clientRect);

	// IDC_EDIT1 の左マージンを基準に、右にも同じ幅の余白を取って配置する
	// (.rc の詳細なレイアウトが分からないための簡易的な計算)。
	const int progressHeight = 8;
	CRect progressRect(
		editRect.left,
		editRect.bottom + 4,
		clientRect.right - editRect.left,
		editRect.bottom + 4 + progressHeight);

	// スキャン中であることを示すプログレスバー。総ファイル数が事前に
	// わからないため、パーセンテージ表示ではなく不確定(マーキー)表示にする。
	m_progressBar.Create(WS_CHILD | PBS_MARQUEE, progressRect, this, IDC_PROGRESS_SCAN);
	m_progressBar.ShowWindow(SW_HIDE);

	CRect listRect(
		editRect.left,
		progressRect.bottom + 4,
		clientRect.right - editRect.left,
		clientRect.bottom - editRect.left);

	m_listResults.Create(
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | WS_TABSTOP | LBS_NOTIFY | LBS_HASSTRINGS,
		listRect, this, IDC_LIST_RESULTS);

	CFont* pFont = GetFont();
	if (pFont != nullptr)
	{
		m_listResults.SetFont(pFont);
	}

	ApplyAppearance();
}

void CgleipnirDlg::ApplyAppearance()
{
	// config.ini [Common] の TextColor / BackgroundColor / Font を読み込む。
	// 空欄・不正値の項目は既定値(緑文字・黒背景・メイリオ太字)を使う。
	const CString configPath = GetSettingsFilePath();
	TCHAR buf[512] = { 0 };

	m_textColor = RGB(0, 255, 0); // 既定: 緑
	::GetPrivateProfileString(_T("Common"), _T("TextColor"), _T(""), buf, _countof(buf), configPath);
	ParseColorString(buf, m_textColor);

	m_bgColor = RGB(0, 0, 0);     // 既定: 黒
	::GetPrivateProfileString(_T("Common"), _T("BackgroundColor"), _T(""), buf, _countof(buf), configPath);
	ParseColorString(buf, m_bgColor);

	// 既定: ダイアログフォントのサイズを引き継ぎ、メイリオ太字。
	// 指定書体が無い環境ではGDIが近い書体に自動で置き換える。
	LOGFONT lf = {};
	CFont* pBase = GetFont();
	if (pBase != nullptr)
	{
		pBase->GetLogFont(&lf);
	}
	else
	{
		lf.lfHeight = -12;
	}
	lf.lfWeight = FW_BOLD;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfQuality = CLEARTYPE_QUALITY;
	_tcscpy_s(lf.lfFaceName, _T("Meiryo"));

	::GetPrivateProfileString(_T("Common"), _T("Font"), _T(""), buf, _countof(buf), configPath);
	{
		CClientDC dc(this);
		ParseFontString(buf, lf, dc.GetSafeHdc());
	}

	// コントロールに新しいフォントを渡してから古いフォントを破棄する
	// (使用中の HFONT を先に消さないよう、一時オブジェクト経由で差し替える)。
	CFont newFont;
	newFont.CreateFontIndirect(&lf);

	CWnd* pEdit = GetDlgItem(IDC_EDIT1);
	pEdit->SetFont(&newFont);
	m_listResults.SetFont(&newFont);

	m_font.DeleteObject();
	m_font.Attach(newFont.Detach());

	m_bgBrush.DeleteObject();
	m_bgBrush.CreateSolidBrush(m_bgColor);

	SetBackgroundColor(m_bgColor);

	// フォントの高さに合わせて検索欄の高さを調整し、その下のプログレスバーと
	// 候補リストを詰め直す(.rc で決めた高さより小さくはしない)。
	CRect editRect;
	pEdit->GetWindowRect(&editRect);
	ScreenToClient(&editRect);
	if (m_editMinHeight == 0)
	{
		m_editMinHeight = editRect.Height();
	}

	TEXTMETRIC tm = {};
	{
		CClientDC dc(pEdit);
		CFont* pOld = dc.SelectObject(&m_font);
		dc.GetTextMetrics(&tm);
		dc.SelectObject(pOld);
	}
	const int fontEditHeight = static_cast<int>(tm.tmHeight) + 8; // 枠線と内側の余白ぶん
	const int editHeight = (fontEditHeight > m_editMinHeight) ? fontEditHeight : m_editMinHeight;
	editRect.bottom = editRect.top + editHeight;
	pEdit->MoveWindow(&editRect);

	CRect clientRect;
	GetClientRect(&clientRect);

	CRect progressRect;
	m_progressBar.GetWindowRect(&progressRect);
	ScreenToClient(&progressRect);
	const int progressHeight = progressRect.Height();
	progressRect.top = editRect.bottom + 4;
	progressRect.bottom = progressRect.top + progressHeight;
	m_progressBar.MoveWindow(&progressRect);

	CRect listRect(
		editRect.left,
		progressRect.bottom + 4,
		clientRect.right - editRect.left,
		clientRect.bottom - editRect.left);
	m_listResults.MoveWindow(&listRect);

	Invalidate();
	pEdit->Invalidate();
	m_listResults.Invalidate();
}

// static
bool CgleipnirDlg::ParseColorString(const CString& text, COLORREF& color)
{
	CString s = text;
	s.Trim();
	if (s.IsEmpty())
	{
		return false;
	}

	// "R,G,B" 形式(各 0〜255)
	if (s.Find(_T(',')) >= 0)
	{
		int values[3] = { 0 };
		int count = 0;
		int pos = 0;
		CString token = s.Tokenize(_T(","), pos);
		while (!token.IsEmpty())
		{
			token.Trim();
			if (count >= 3 || token.IsEmpty() || token.SpanIncluding(_T("0123456789")) != token)
			{
				return false;
			}
			const int v = _ttoi(token);
			if (v < 0 || v > 255)
			{
				return false;
			}
			values[count++] = v;
			token = s.Tokenize(_T(","), pos);
		}
		if (count != 3)
		{
			return false;
		}
		color = RGB(values[0], values[1], values[2]);
		return true;
	}

	// "#RRGGBB" / "RRGGBB" 形式(HTML・CSS と同じ並び)
	if (s[0] == _T('#'))
	{
		s = s.Mid(1);
	}
	if (s.GetLength() != 6 || s.SpanIncluding(_T("0123456789abcdefABCDEF")) != s)
	{
		return false;
	}
	const unsigned long rgb = _tcstoul(s, nullptr, 16);
	color = RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
	return true;
}

// static
void CgleipnirDlg::ParseFontString(const CString& text, LOGFONT& lf, HDC hDC)
{
	// "書体名[,サイズ(pt)][,bold|normal]"。カンマ区切りの各要素を種類で判定するので
	// 順不同でもよい(数字だけ→サイズ、bold/normal 等→太さ、それ以外→書体名)。
	int pos = 0;
	CString token = text.Tokenize(_T(","), pos);
	while (!token.IsEmpty())
	{
		token.Trim();
		if (!token.IsEmpty())
		{
			if (token.SpanIncluding(_T("0123456789")) == token)
			{
				const int pt = _ttoi(token);
				if (pt >= 6 && pt <= 72 && hDC != nullptr)
				{
					lf.lfHeight = -::MulDiv(pt, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
					lf.lfWidth = 0;
				}
			}
			else if (token.CompareNoCase(_T("bold")) == 0 || token == _T("太字"))
			{
				lf.lfWeight = FW_BOLD;
			}
			else if (token.CompareNoCase(_T("normal")) == 0 || token.CompareNoCase(_T("regular")) == 0
				|| token == _T("標準"))
			{
				lf.lfWeight = FW_NORMAL;
			}
			else if (token.GetLength() < LF_FACESIZE)
			{
				_tcscpy_s(lf.lfFaceName, token);
			}
		}
		token = text.Tokenize(_T(","), pos);
	}
}

HBRUSH CgleipnirDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	// ApplyAppearance() 前(ブラシ未作成)は既定動作に任せる。
	if (m_bgBrush.GetSafeHandle() == nullptr)
	{
		return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	}

	switch (nCtlColor)
	{
	case CTLCOLOR_EDIT:
	case CTLCOLOR_LISTBOX:
	case CTLCOLOR_STATIC:
		pDC->SetTextColor(m_textColor);
		pDC->SetBkColor(m_bgColor);
		return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
	default:
		return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	}
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
	HideLauncherWindow();
}

void CgleipnirDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == SC_MINIMIZE)
	{
		HideLauncherWindow();
		return;
	}

	CDialogEx::OnSysCommand(nID, lParam);
}

BOOL CgleipnirDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		if (GetFocus() == GetDlgItem(IDC_EDIT1) || GetFocus() == &m_listResults)
		{
			CString input;
			GetDlgItemText(IDC_EDIT1, input);

			// instant.ini のキーワードに一致すれば、そちらを優先して実行する。
			if (!input.IsEmpty() && TryExecuteInstantCommand(input))
			{
				return TRUE;
			}

			LaunchSelectedResult();
			return TRUE; // Enterによるダイアログの既定動作(閉じる等)を抑止する
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
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

		CMenu settingsMenu;
		settingsMenu.CreatePopupMenu();
		settingsMenu.AppendMenu(MF_STRING, ID_TRAY_EDIT_SCAN_INI, _T("scan.ini を編集(&S)"));
		settingsMenu.AppendMenu(MF_STRING, ID_TRAY_EDIT_INSTANT_INI, _T("instant.ini を編集(&I)"));
		settingsMenu.AppendMenu(MF_SEPARATOR);
		settingsMenu.AppendMenu(MF_STRING, ID_TRAY_RELOAD_SETTINGS, _T("設定を再読み込み(&R)"));
		settingsMenu.AppendMenu(MF_STRING, ID_TRAY_OPEN_DATA_FOLDER, _T("データフォルダを開く(&D)"));

		menu.AppendMenu(MF_STRING, ID_TRAY_SCAN, _T("スキャン実行(&S)"));
		menu.AppendMenu(MF_STRING | (m_bIndexing ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)),
			ID_TRAY_CANCEL_SCAN, _T("スキャンをキャンセル(&C)"));
		menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(settingsMenu.Detach()), _T("設定(&O)"));
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, ID_TRAY_EXIT, _T("終了(&X)"));

		CPoint pt;
		GetCursorPos(&pt);

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

LRESULT CgleipnirDlg::OnIndexComplete(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);

	// ワーカースレッドから所有権が渡された完了情報を受け取る
	std::unique_ptr<IndexCompleteResult> completion(reinterpret_cast<IndexCompleteResult*>(lParam));
	std::unique_ptr<std::vector<FileEntry>> pResults(completion->results);

	m_bIndexing = false;

	// プログレスバーを止めて隠す
	m_progressBar.SetMarquee(FALSE, 0);
	m_progressBar.ShowWindow(SW_HIDE);

	if (completion->canceled)
	{
		// キャンセルされた場合、index.txt はその時点までの結果では
		// 上書きしない(既存のインデックスをそのまま維持する)。
		CString title;
		title.Format(_T("gleipnir - スキャンを中断しました(%zu 件、未保存)"), pResults->size());
		SetWindowText(title);

		MessageBox(
			_T("スキャンをキャンセルしました。\nインデックスは更新していません。"),
			_T("gleipnir"), MB_OK | MB_ICONINFORMATION);

		return 0;
	}

	m_fileIndex = std::move(*pResults);

	SaveIndexToFile(GetIndexFilePath(), m_fileIndex);

	CString title;
	title.Format(_T("gleipnir - 検索完了：%zu 件"), m_fileIndex.size());
	SetWindowText(title);

	// スキャン完了ダイアログ(検出件数を表示)
	CString msg;
	msg.Format(_T("スキャンが完了しました。\n%zu 件のファイル/フォルダが見つかりました。"), m_fileIndex.size());
	MessageBox(msg, _T("gleipnir"), MB_OK | MB_ICONINFORMATION);

	// 検索中であれば、最新のインデックスで結果を更新する
	CString query;
	GetDlgItemText(IDC_EDIT1, query);
	if (!query.IsEmpty())
	{
		UpdateSearchResults(query);
	}

	return 0;
}

LRESULT CgleipnirDlg::OnIndexProgress(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	size_t count = static_cast<size_t>(wParam);

	CString title;
	title.Format(_T("gleipnir - スキャン中: %zu 件"), count);
	SetWindowText(title);

	return 0;
}

void CgleipnirDlg::OnTrayExit()
{
	if (m_bIndexing)
	{
		MessageBox(
			_T("スキャン中は終了できません。先にキャンセルしてください。"),
			_T("gleipnir"), MB_OK | MB_ICONWARNING);
		return;
	}

	EndDialog(IDCANCEL);
}

void CgleipnirDlg::OnTrayScan()
{
	ExecuteScanWithConfirmation();
}

void CgleipnirDlg::OnTrayCancelScan()
{
	RequestCancelScan();
}

void CgleipnirDlg::OnTrayEditScanIni()
{
	EnsureConfigFileExists(GetScanConfigFilePath(), kDefaultScanIniTemplate);
	OpenInNotepad(GetScanConfigFilePath());
}

void CgleipnirDlg::OnTrayEditInstantIni()
{
	EnsureConfigFileExists(GetInstantConfigFilePath(), kDefaultInstantIniTemplate);
	OpenInNotepad(GetInstantConfigFilePath());
}

void CgleipnirDlg::OnTrayReloadSettings()
{
	LoadInstantCommands();
	ApplyAppearance(); // config.ini を手で編集した場合の外観反映
	ShowTrayBalloon(_T("gleipnir"), _T("設定を再読み込みしました。"));
}

void CgleipnirDlg::OnTrayOpenDataFolder()
{
	::ShellExecute(nullptr, _T("open"), GetDataFolderPath(), nullptr, nullptr, SW_SHOWNORMAL);
}

void CgleipnirDlg::OnEnChangeEdit1()
{
	CString query;
	GetDlgItemText(IDC_EDIT1, query);

	CString trimmed = query;
	trimmed.Trim();

	if (trimmed.CompareNoCase(kScanCommand) == 0 || trimmed.CompareNoCase(kScanStartCommand) == 0)
	{
		// コマンド入力を検索欄に残さないようクリアしてから確認ダイアログを出す
		SetDlgItemText(IDC_EDIT1, _T(""));
		m_listResults.ResetContent();
		m_filteredResults.clear();

		ExecuteScanWithConfirmation();
		return;
	}

	if (trimmed.CompareNoCase(kScanStopCommand) == 0 || trimmed.CompareNoCase(kScanCancelCommand) == 0)
	{
		SetDlgItemText(IDC_EDIT1, _T(""));
		m_listResults.ResetContent();
		m_filteredResults.clear();

		RequestCancelScan();
		return;
	}

	if (trimmed.CompareNoCase(kOptionCommand) == 0)
	{
		SetDlgItemText(IDC_EDIT1, _T(""));
		m_listResults.ResetContent();
		m_filteredResults.clear();

		CSettingDlg dlg(this);
		if (dlg.DoModal() == IDOK)
		{
			// 保存された文字色・背景色・フォントを即座に反映する。
			ApplyAppearance();
		}
		return;
	}

	// 引数不要(%Aを含まない)のインスタントコマンドが、入力により一意に定まった
	// 時点でEnter無しで自動実行する。
	if (ShouldAutoExecuteInstant(query) && TryExecuteInstantCommand(query))
	{
		return;
	}

	UpdateSearchResults(query);
}

bool CgleipnirDlg::ShouldAutoExecuteInstant(const CString& input) const
{
	const InstantCommandEntry* pMatch = nullptr;
	for (const auto& cmd : m_instantCommands)
	{
		if (cmd.keyword.IsEmpty())
		{
			continue;
		}

		if (cmd.keyword == input)
		{
			pMatch = &cmd;
		}
		else if (cmd.keyword.GetLength() > input.GetLength() &&
			cmd.keyword.Left(input.GetLength()) == input)
		{
			return false; // より長いキーワードが続く可能性があるので確定しない
		}
	}

	// 引数(%A)を受け取るコマンドは、引数入力のためEnterでの確定を待つ。
	return pMatch != nullptr && !pMatch->needsArg;
}

void CgleipnirDlg::OnLbnDblclkListResults()
{
	LaunchSelectedResult();
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

void CgleipnirDlg::ShowTrayBalloon(const CString& title, const CString& text)
{
	if (!m_bTrayIconAdded)
	{
		return;
	}

	NOTIFYICONDATA nid = m_nid;
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_INFO;
	nid.uTimeout = 4000;
	_tcscpy_s(nid.szInfoTitle, title.Left(63));
	_tcscpy_s(nid.szInfo, text.Left(255));

	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void CgleipnirDlg::HideLauncherWindow()
{
	// config.ini [Common] ClearInputOnHide (既定: 1)。
	// 0 を指定した場合のみ、非表示にしても入力中の文字列・候補を残す。
	// 設定ダイアログからの変更を即座に反映できるよう、都度読み出す。
	const bool clearOnHide =
		::GetPrivateProfileInt(_T("Common"), _T("ClearInputOnHide"), 1, GetSettingsFilePath()) != 0;

	if (clearOnHide)
	{
		SetDlgItemText(IDC_EDIT1, _T(""));
		m_listResults.ResetContent();
		m_filteredResults.clear();
	}

	ShowWindow(SW_HIDE);
}

void CgleipnirDlg::ToggleVisibility()
{
	if (IsWindowVisible())
	{
		HideLauncherWindow();
	}
	else
	{
		ShowWindow(SW_SHOW);
		SetForegroundWindow();
		GetDlgItem(IDC_EDIT1)->SetFocus();
	}
}

bool CgleipnirDlg::ConfirmScan(const CString& message)
{
	int result = MessageBox(message, _T("gleipnir"), MB_YESNO | MB_ICONQUESTION);
	return result == IDYES;
}

void CgleipnirDlg::ExecuteScanWithConfirmation()
{
	if (m_bIndexing)
	{
		ShowTrayBalloon(_T("gleipnir"), _T("既にインデックスを作成中です。"));
		return;
	}

	if (!ConfirmScan(_T("スキャンを実行してもよろしいですか？")))
	{
		return;
	}

	std::vector<CString> driveRoots;
	std::vector<CString> registerExtensions;
	std::vector<CString> ignoreFolderNames;

	bool shouldScan = LoadScanConfig(driveRoots, registerExtensions, ignoreFolderNames);

	if (!shouldScan || driveRoots.empty())
	{
		MessageBox(
			_T("scan.ini に [DRIVE] セクションの TARGET 指定が無いため、スキャンを実行しません。"),
			_T("gleipnir"), MB_OK | MB_ICONINFORMATION);
		return;
	}

	BuildIndexAsync(driveRoots, registerExtensions, ignoreFolderNames);
}

void CgleipnirDlg::RequestCancelScan()
{
	if (!m_bIndexing)
	{
		ShowTrayBalloon(_T("gleipnir"), _T("実行中のスキャンはありません。"));
		return;
	}

	// 確認ダイアログは出さず、即座にキャンセルを要求する。
	// 実際の中断はワーカースレッド側がこのフラグを見て行うため、
	// 反映まで多少のタイムラグが生じる場合がある。
	m_scanCancelRequested = true;
	ShowTrayBalloon(_T("gleipnir"), _T("スキャンのキャンセルを要求しました。"));
}

namespace
{
	// 文字列の内容を0で上書きしてから空にする(秘匿情報のメモリ残留を減らす。
	// GetBuffer()は共有バッファの場合に複製を作るため、最後の所有者が元の
	// 領域を上書きすることになる。ベストエフォートの対策)。
	void WipeCString(CString& s)
	{
		int len = s.GetLength();
		if (len > 0)
		{
			LPTSTR p = s.GetBuffer();
			::SecureZeroMemory(p, static_cast<SIZE_T>(len) * sizeof(TCHAR));
			s.ReleaseBuffer(0);
		}
	}

	// スコープを抜ける時(途中のreturn含む)に、登録した文字列を消去する。
	class StringWiper
	{
	public:
		void Add(CString& s) { m_items.push_back(&s); }
		~StringWiper()
		{
			for (CString* p : m_items)
			{
				WipeCString(*p);
			}
		}
	private:
		std::vector<CString*> m_items;
	};

	// instant.ini の [InstantCommand] セクションを取得する。
	// 使用後は buffer を SecureZeroMemory で消去すること。
	bool ReadInstantSection(const CString& path, std::vector<TCHAR>& buffer)
	{
		if (::GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
		{
			return false; // instant.ini が無ければ何もしない
		}

		// GetPrivateProfileSectionは "key=value\0key=value\0...\0\0" 形式で返す
		buffer.assign(32768, _T('\0'));
		::GetPrivateProfileSection(_T("InstantCommand"), buffer.data(),
			static_cast<DWORD>(buffer.size()), path);
		return true;
	}

	void WipeBuffer(std::vector<TCHAR>& buffer)
	{
		::SecureZeroMemory(buffer.data(), buffer.size() * sizeof(TCHAR));
	}
}

void CgleipnirDlg::LoadInstantCommands()
{
	m_instantCommands.clear();

	std::vector<TCHAR> buffer;
	if (!ReadInstantSection(GetInstantConfigFilePath(), buffer))
	{
		return;
	}

	// キーワードと「%Aを含むか」だけを保持し、コマンドライン本体は残さない。
	const TCHAR* p = buffer.data();
	while (*p != _T('\0'))
	{
		CString line = p;
		int eq = line.Find(_T('='));
		if (eq > 0)
		{
			InstantCommandEntry entry;
			entry.keyword = line.Left(eq);
			entry.needsArg = (line.Find(_T("%A"), eq + 1) >= 0);
			m_instantCommands.push_back(entry);
		}
		p += line.GetLength() + 1;
		WipeCString(line);
	}

	WipeBuffer(buffer);
}

bool CgleipnirDlg::ReadInstantCommandLine(const CString& keyword, CString& commandLine) const
{
	commandLine.Empty();

	std::vector<TCHAR> buffer;
	if (!ReadInstantSection(GetInstantConfigFilePath(), buffer))
	{
		return false;
	}

	bool found = false;
	const TCHAR* p = buffer.data();
	while (*p != _T('\0'))
	{
		CString line = p;
		int eq = line.Find(_T('='));
		if (!found && eq > 0 && line.Left(eq) == keyword)
		{
			commandLine = line.Mid(eq + 1);
			found = true;
		}
		p += line.GetLength() + 1;
		WipeCString(line);
		if (found)
		{
			break;
		}
	}

	WipeBuffer(buffer);
	return found;
}

bool CgleipnirDlg::TryExecuteInstantCommand(const CString& input)
{
	// キーワードとの一致は大文字小文字を区別する(本家Fenrirと同様)。
	// 複数一致する場合(例: "/c" と "/clip")は、最も長いキーワードを採用する。
	const InstantCommandEntry* pBest = nullptr;
	for (const auto& cmd : m_instantCommands)
	{
		if (cmd.keyword.IsEmpty())
		{
			continue;
		}

		if (input.GetLength() >= cmd.keyword.GetLength() &&
			input.Left(cmd.keyword.GetLength()) == cmd.keyword)
		{
			if (pBest == nullptr || cmd.keyword.GetLength() > pBest->keyword.GetLength())
			{
				pBest = &cmd;
			}
		}
	}

	if (pBest == nullptr)
	{
		return false;
	}

	// キーワード直後の空白(例: "/clip xxxx" の区切り)は引数に含めない。
	CString arg = input.Mid(pBest->keyword.GetLength());
	arg.TrimLeft();

	// コマンドライン本体は常駐させず、実行のたびに instant.ini から読み出し、
	// 使い終わったら(途中のreturnも含め)上書き消去する。
	CString commandLine;
	CString expanded;
	CString trimmed;
	CString clipText;
	CString file;
	CString params;
	StringWiper wiper;
	wiper.Add(arg);
	wiper.Add(commandLine);
	wiper.Add(expanded);
	wiper.Add(trimmed);
	wiper.Add(clipText);
	wiper.Add(file);
	wiper.Add(params);

	if (!ReadInstantCommandLine(pBest->keyword, commandLine))
	{
		return true; // instant.ini が削除/変更された場合は何もしない
	}

	expanded = ExpandInstantMacros(commandLine, arg);

	auto finishAndHide = [this]()
	{
		SetDlgItemText(IDC_EDIT1, _T(""));
		m_listResults.ResetContent();
		m_filteredResults.clear();
		ShowWindow(SW_HIDE);
	};

	// "@clip 文字列" : 実行せず、文字列をクリップボードへコピーする。
	trimmed = expanded;
	trimmed.TrimLeft();
	const int clipLen = 5; // "@clip"
	if (trimmed.GetLength() >= clipLen &&
		trimmed.Left(clipLen).CompareNoCase(_T("@clip")) == 0 &&
		(trimmed.GetLength() == clipLen ||
			trimmed[clipLen] == _T(' ') || trimmed[clipLen] == _T('\t')))
	{
		clipText = trimmed.Mid(clipLen);
		clipText.TrimLeft();

		if (SetClipboardText(m_hWnd, clipText))
		{
			finishAndHide();
		}
		return true;
	}

	SplitCommandLine(expanded, file, params);

	if (file.IsEmpty())
	{
		return true; // キーワードは一致したが実行対象が無いので何もしない
	}

	HINSTANCE hResult = ::ShellExecute(nullptr, _T("open"), file,
		params.IsEmpty() ? nullptr : static_cast<LPCTSTR>(params),
		nullptr, SW_SHOWNORMAL);

	if (reinterpret_cast<INT_PTR>(hResult) > 32)
	{
		finishAndHide();
	}

	return true; // キーワード一致時点で、通常の検索起動へはフォールバックしない
}

// static
bool CgleipnirDlg::SetClipboardText(HWND hWndOwner, const CString& text)
{
	if (!::OpenClipboard(hWndOwner))
	{
		return false;
	}

	bool ok = false;
	::EmptyClipboard();

	const SIZE_T chars = static_cast<SIZE_T>(text.GetLength());
	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, (chars + 1) * sizeof(wchar_t));
	if (hMem != nullptr)
	{
		wchar_t* pDst = static_cast<wchar_t*>(::GlobalLock(hMem));
		if (pDst != nullptr)
		{
			if (chars > 0)
			{
				memcpy(pDst, static_cast<LPCWSTR>(text), chars * sizeof(wchar_t));
			}
			pDst[chars] = L'\0';
			::GlobalUnlock(hMem);

			// 成功時は所有権がクリップボードへ移るので解放しない。
			if (::SetClipboardData(CF_UNICODETEXT, hMem) != nullptr)
			{
				ok = true;
			}
		}
		if (!ok)
		{
			::GlobalFree(hMem);
		}
	}

	::CloseClipboard();
	return ok;
}

CString CgleipnirDlg::ExpandInstantMacros(const CString& commandLine, const CString& arg) const
{
	CString result = commandLine;

	result.Replace(_T("%A"), arg);
	{
		CString clip = GetClipboardText(m_hWnd);
		result.Replace(_T("%C"), clip);
		WipeCString(clip);
	}

	CString exeDir = GetExeFolderPath();
	result.Replace(_T("%W"), exeDir);

	CString drive = exeDir.Left(2); // 例: "C:"
	result.Replace(_T("%V"), drive);

	result.Replace(_T("%scanfile"), GetScanConfigFilePath());
	result.Replace(_T("%pathfile"), GetIndexFilePath());
	result.Replace(_T("%initfile"), GetSettingsFilePath());
	result.Replace(_T("%cmddir"), GetDataFolderPath());

	return result;
}

// static
void CgleipnirDlg::SplitCommandLine(const CString& cmdLine, CString& file, CString& params)
{
	CString trimmed = cmdLine;
	trimmed.TrimLeft();

	if (trimmed.IsEmpty())
	{
		file.Empty();
		params.Empty();
		return;
	}

	if (trimmed[0] == _T('"'))
	{
		int closeQuote = trimmed.Find(_T('"'), 1);
		if (closeQuote >= 0)
		{
			file = trimmed.Mid(1, closeQuote - 1);
			params = trimmed.Mid(closeQuote + 1);
			params.TrimLeft();
			return;
		}

		// 閉じクォーテーションが無い場合は、そのまま1トークンとして扱う。
		file = trimmed;
		params.Empty();
		return;
	}

	int spacePos = trimmed.Find(_T(' '));
	if (spacePos < 0)
	{
		file = trimmed;
		params.Empty();
	}
	else
	{
		file = trimmed.Left(spacePos);
		params = trimmed.Mid(spacePos + 1);
		params.TrimLeft();
	}
}

// static
CString CgleipnirDlg::GetClipboardText(HWND hWndOwner)
{
	CString result;

	if (!::OpenClipboard(hWndOwner))
	{
		return result;
	}

	HANDLE hData = ::GetClipboardData(CF_UNICODETEXT);
	if (hData != nullptr)
	{
		wchar_t* pText = static_cast<wchar_t*>(::GlobalLock(hData));
		if (pText != nullptr)
		{
			result = pText;
			::GlobalUnlock(hData);
		}
	}

	::CloseClipboard();
	return result;
}

// static
void CgleipnirDlg::EnsureConfigFileExists(const CString& path, const CString& defaultContent)
{
	if (::GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
	{
		return; // 既に存在する場合は何もしない(上書きしない)
	}

	CFile file;
	if (!file.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive))
	{
		return;
	}

	const WORD bom = 0xFEFF;
	file.Write(&bom, sizeof(bom));
	file.Write(defaultContent.GetString(), defaultContent.GetLength() * sizeof(TCHAR));
}

void CgleipnirDlg::OpenInNotepad(const CString& path)
{
	CString quotedPath;
	quotedPath.Format(_T("\"%s\""), static_cast<LPCTSTR>(path));

	::ShellExecute(nullptr, _T("open"), _T("notepad.exe"), quotedPath, nullptr, SW_SHOWNORMAL);
}

void CgleipnirDlg::UpdateSearchResults(const CString& query)
{
	m_listResults.ResetContent();
	m_filteredResults.clear();

	if (query.IsEmpty())
	{
		return;
	}

	CString lowerQuery = query;
	lowerQuery.MakeLower();

	// 重複除去用(小文字化したフルパスの集合)。
	std::set<CString> addedPaths;

	// 起動履歴(最近使った順)から一致するものを優先して先に追加する。
	for (const auto& histPath : m_launchHistory)
	{
		CString displayName = ExtractFileName(histPath);
		CString lowerName = displayName;
		lowerName.MakeLower();

		if (lowerName.Find(lowerQuery) < 0)
		{
			continue;
		}

		FileEntry entry;
		entry.fullPath = histPath;
		entry.displayName = displayName;
		m_filteredResults.push_back(entry);

		CString key = histPath;
		key.MakeLower();
		addedPaths.insert(key);

		if (static_cast<int>(m_filteredResults.size()) >= kMaxDisplayResults)
		{
			break;
		}
	}

	// 履歴側で埋まっていない残り枠を、インデックス全体からの通常検索で埋める
	// (履歴に既に含まれるパスは重複させない)。
	if (static_cast<int>(m_filteredResults.size()) < kMaxDisplayResults)
	{
		for (const auto& entry : m_fileIndex)
		{
			CString lowerName = entry.displayName;
			lowerName.MakeLower();

			if (lowerName.Find(lowerQuery) < 0)
			{
				continue;
			}

			CString key = entry.fullPath;
			key.MakeLower();
			if (addedPaths.find(key) != addedPaths.end())
			{
				continue;
			}

			m_filteredResults.push_back(entry);
			if (static_cast<int>(m_filteredResults.size()) >= kMaxDisplayResults)
			{
				break;
			}
		}
	}

	for (const auto& entry : m_filteredResults)
	{
		CString line = entry.displayName + _T("   [") + entry.fullPath + _T("]");
		m_listResults.AddString(line);
	}

	if (m_listResults.GetCount() > 0)
	{
		m_listResults.SetCurSel(0);
	}
}

void CgleipnirDlg::LaunchSelectedResult()
{
	int sel = m_listResults.GetCurSel();
	if (sel < 0 || sel >= static_cast<int>(m_filteredResults.size()))
	{
		return;
	}

	const CString& path = m_filteredResults[sel].fullPath;

	HINSTANCE hResult = ::ShellExecute(nullptr, _T("open"), path, nullptr, nullptr, SW_SHOWNORMAL);
	if (reinterpret_cast<INT_PTR>(hResult) > 32)
	{
		// 起動履歴に記録する(検索時に優先表示するため)。
		RecordLaunchHistory(path);

		// 起動に成功したら検索窓をクリアし、自身は非表示にする
		SetDlgItemText(IDC_EDIT1, _T(""));
		m_listResults.ResetContent();
		m_filteredResults.clear();
		ShowWindow(SW_HIDE);
	}
}

void CgleipnirDlg::RecordLaunchHistory(const CString& fullPath)
{
	// 既に履歴にあれば削除してから先頭に挿入し、最近使った順に並べ直す。
	for (auto it = m_launchHistory.begin(); it != m_launchHistory.end(); ++it)
	{
		if (it->CompareNoCase(fullPath) == 0)
		{
			m_launchHistory.erase(it);
			break;
		}
	}

	m_launchHistory.insert(m_launchHistory.begin(), fullPath);

	// 件数上限を超えたら古いものを切り捨てる。
	constexpr size_t kMaxHistoryEntries = 50;
	if (m_launchHistory.size() > kMaxHistoryEntries)
	{
		m_launchHistory.resize(kMaxHistoryEntries);
	}

	SaveHistoryToFile(GetHistoryFilePath(), m_launchHistory);
}

// static
CString CgleipnirDlg::GetExeFolderPath()
{
	TCHAR exePath[MAX_PATH] = { 0 };
	::GetModuleFileName(nullptr, exePath, MAX_PATH);

	CString dir = exePath;
	int pos = dir.ReverseFind(_T('\\'));
	if (pos >= 0)
	{
		dir = dir.Left(pos);
	}

	return dir;
}

// static
CString CgleipnirDlg::GetDataFolderPath()
{
	// exe自身の場所を基準にする(インストール先の変更やUSB等での持ち運びを
	// 想定し、%LOCALAPPDATA%等のユーザープロファイル依存にはしない)。
	// インデックスファイル・scan.ini・instant.ini・今後追加予定のアプリ
	// 設定ファイルもこのフォルダ以下に保存する想定の共通ヘルパー。
	CString dir = GetExeFolderPath() + _T("\\data");

	::CreateDirectory(dir, nullptr); // 既に存在する場合のエラーは無視してよい

	return dir;
}

// static
CString CgleipnirDlg::GetIndexFilePath()
{
	return GetDataFolderPath() + _T("\\index.txt");
}

// static
CString CgleipnirDlg::GetHistoryFilePath()
{
	return GetDataFolderPath() + _T("\\history.txt");
}

// static
CString CgleipnirDlg::GetScanConfigFilePath()
{
	return GetDataFolderPath() + _T("\\scan.ini");
}

// static
CString CgleipnirDlg::GetInstantConfigFilePath()
{
	return GetDataFolderPath() + _T("\\instant.ini");
}

// static
CString CgleipnirDlg::GetSettingsFilePath()
{
	// 現時点では設定ダイアログ本体は未実装だが、instant.iniの%initfile
	// マクロや将来の設定保存先として先にパスだけ定義しておく。
	return GetDataFolderPath() + _T("\\config.ini");
}

// static
bool CgleipnirDlg::LoadScanConfig(std::vector<CString>& driveRoots,
	std::vector<CString>& registerExtensions,
	std::vector<CString>& ignoreFolderNames)
{
	driveRoots.clear();
	registerExtensions.clear();
	ignoreFolderNames.clear();

	CString configPath = GetScanConfigFilePath();

	if (::GetFileAttributes(configPath) == INVALID_FILE_ATTRIBUTES)
	{
		// scan.ini が存在しない場合は、既定として現在の固定ドライブのみを対象にする。
		DWORD driveMask = ::GetLogicalDrives();
		for (int i = 0; i < 26; ++i)
		{
			if ((driveMask & (1u << i)) == 0)
			{
				continue;
			}

			CString root;
			root.Format(_T("%c:\\"), _T('A') + i);

			if (::GetDriveType(root) == DRIVE_FIXED)
			{
				driveRoots.push_back(root);
			}
		}

		return true;
	}

	// [DRIVE] TARGET=C:,D:,...
	TCHAR targetBuf[1024] = { 0 };
	::GetPrivateProfileString(_T("DRIVE"), _T("TARGET"), _T(""), targetBuf, _countof(targetBuf), configPath);
	CString targetValue = targetBuf;

	if (targetValue.IsEmpty())
	{
		// [DRIVE]セクション(またはTARGETキー)が無い場合はスキャン自体を行わない。
		return false;
	}

	for (CString token : SplitCommaList(targetValue))
	{
		if (token.Right(1) != _T("\\"))
		{
			token += _T("\\");
		}
		driveRoots.push_back(token);
	}

	// [regist] REGIST=*.txt,*.doc,...
	// ここに列挙された拡張子を持つファイルのみインデックスに登録する
	// (ホワイトリスト方式。index.txtの肥大化対策)。
	TCHAR registBuf[1024] = { 0 };
	::GetPrivateProfileString(_T("regist"), _T("REGIST"), _T(""), registBuf, _countof(registBuf), configPath);
	CString registValue = registBuf;

	for (CString token : SplitCommaList(registValue))
	{
		// "*.txt" のようなワイルドカード表記から拡張子部分だけを取り出す。
		int dotPos = token.ReverseFind(_T('.'));
		CString ext = (dotPos >= 0) ? token.Mid(dotPos) : token;
		ext.MakeLower();
		if (!ext.IsEmpty())
		{
			registerExtensions.push_back(ext);
		}
	}

	// [ignore] FOLDERNAME=***,...
	TCHAR folderBuf[1024] = { 0 };
	::GetPrivateProfileString(_T("ignore"), _T("FOLDERNAME"), _T(""), folderBuf, _countof(folderBuf), configPath);
	CString folderValue = folderBuf;

	for (CString token : SplitCommaList(folderValue))
	{
		token.MakeLower();
		ignoreFolderNames.push_back(token);
	}

	return true;
}

// static
void CgleipnirDlg::SaveIndexToFile(const CString& filePath, const std::vector<FileEntry>& entries)
{
	CFile file;
	if (!file.Open(filePath, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive))
	{
		TRACE(traceAppMsg, 0, "警告: インデックスファイルの保存に失敗しました。\n");
		return;
	}

	// UTF-16LE BOM (自前フォーマットなので必須ではないが、他エディタで
	// 開いたときに文字化けしないよう付与しておく)
	const WORD bom = 0xFEFF;
	file.Write(&bom, sizeof(bom));

	for (const auto& entry : entries)
	{
		CString line = entry.fullPath;
		line += _T("\r\n");
		file.Write(line.GetString(), line.GetLength() * sizeof(TCHAR));
	}
}

// static
bool CgleipnirDlg::LoadIndexFromFile(const CString& filePath, std::vector<FileEntry>& entries)
{
	entries.clear();

	CFile file;
	if (!file.Open(filePath, CFile::modeRead | CFile::shareDenyWrite))
	{
		return false; // 未作成なら何もしない(呼び出し側でトレイメニュー等から作成してもらう)
	}

	ULONGLONG size = file.GetLength();
	if (size < sizeof(wchar_t))
	{
		return false;
	}

	std::vector<BYTE> buffer(static_cast<size_t>(size));
	file.Read(buffer.data(), static_cast<UINT>(size));
	file.Close();

	const wchar_t* pData = reinterpret_cast<const wchar_t*>(buffer.data());
	size_t count = buffer.size() / sizeof(wchar_t);

	size_t start = 0;
	if (count > 0 && pData[0] == 0xFEFF)
	{
		start = 1; // BOMをスキップ
	}

	CString line;
	for (size_t i = start; i < count; ++i)
	{
		wchar_t ch = pData[i];
		if (ch == L'\n')
		{
			line.TrimRight(_T("\r"));
			if (!line.IsEmpty())
			{
				FileEntry entry;
				entry.fullPath = line;
				entry.displayName = ExtractFileName(line);
				entries.push_back(entry);
			}
			line.Empty();
		}
		else
		{
			line += ch;
		}
	}
	if (!line.IsEmpty())
	{
		line.TrimRight(_T("\r"));
		FileEntry entry;
		entry.fullPath = line;
		entry.displayName = ExtractFileName(line);
		entries.push_back(entry);
	}

	return true;
}

// static
void CgleipnirDlg::SaveHistoryToFile(const CString& filePath, const std::vector<CString>& history)
{
	CFile file;
	if (!file.Open(filePath, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive))
	{
		TRACE(traceAppMsg, 0, "警告: 履歴ファイルの保存に失敗しました。\n");
		return;
	}

	// UTF-16LE BOM(index.txtと同じ自前フォーマット)。
	const WORD bom = 0xFEFF;
	file.Write(&bom, sizeof(bom));

	for (const auto& path : history)
	{
		CString line = path;
		line += _T("\r\n");
		file.Write(line.GetString(), line.GetLength() * sizeof(TCHAR));
	}
}

// static
bool CgleipnirDlg::LoadHistoryFromFile(const CString& filePath, std::vector<CString>& history)
{
	history.clear();

	CFile file;
	if (!file.Open(filePath, CFile::modeRead | CFile::shareDenyWrite))
	{
		return false; // 未作成なら何もしない(まだ何も起動していない状態)
	}

	ULONGLONG size = file.GetLength();
	if (size < sizeof(wchar_t))
	{
		return false;
	}

	std::vector<BYTE> buffer(static_cast<size_t>(size));
	file.Read(buffer.data(), static_cast<UINT>(size));
	file.Close();

	const wchar_t* pData = reinterpret_cast<const wchar_t*>(buffer.data());
	size_t count = buffer.size() / sizeof(wchar_t);

	size_t start = 0;
	if (count > 0 && pData[0] == 0xFEFF)
	{
		start = 1; // BOMをスキップ
	}

	CString line;
	for (size_t i = start; i < count; ++i)
	{
		wchar_t ch = pData[i];
		if (ch == L'\n')
		{
			line.TrimRight(_T("\r"));
			if (!line.IsEmpty())
			{
				history.push_back(line);
			}
			line.Empty();
		}
		else
		{
			line += ch;
		}
	}
	if (!line.IsEmpty())
	{
		line.TrimRight(_T("\r"));
		history.push_back(line);
	}

	return true;
}

// static
void CgleipnirDlg::ScanDirectoryRecursive(const CString& path, std::vector<FileEntry>& results, ScanContext& ctx)
{
	// キャンセルが要求されていれば、このディレクトリの探索自体を行わない。
	if (ctx.cancelFlag != nullptr && ctx.cancelFlag->load())
	{
		return;
	}

	CString searchPath = path;
	if (searchPath.Right(1) != _T("\\"))
	{
		searchPath += _T("\\");
	}
	CString pattern = searchPath + _T("*");

	WIN32_FIND_DATA findData;
	HANDLE hFind = ::FindFirstFile(pattern, &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return; // アクセス不可などは無視して戻る
	}

	do
	{
		// ループの各反復でもキャンセルを確認し、要求されていれば即座に中断する。
		if (ctx.cancelFlag != nullptr && ctx.cancelFlag->load())
		{
			break;
		}

		CString name = findData.cFileName;
		if (name == _T(".") || name == _T(".."))
		{
			continue;
		}

		CString fullPath = searchPath + name;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// シンボリックリンクやジャンクション(再解析ポイント)は
			// 無限ループ防止のため辿らない
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
			{
				continue;
			}

			// scan.ini の [ignore] FOLDERNAME に指定された文字列を
			// フォルダ名(部分一致、大文字小文字区別なし)に含む場合は、
			// このフォルダ自体をインデックスに含めず、配下も探索しない。
			CString lowerName = name;
			lowerName.MakeLower();

			bool folderIgnored = false;
			for (const auto& keyword : ctx.ignoreFolderNames)
			{
				if (!keyword.IsEmpty() && lowerName.Find(keyword) >= 0)
				{
					folderIgnored = true;
					break;
				}
			}
			if (folderIgnored)
			{
				continue;
			}

			FileEntry entry;
			entry.fullPath = fullPath;
			entry.displayName = name;
			results.push_back(entry);
			++ctx.foundCount;

			ScanDirectoryRecursive(fullPath, results, ctx);
		}
		else
		{
			// ホワイトリスト方式: scan.ini の [regist] REGIST に指定された
			// 拡張子を持つファイルのみインデックスに登録する。
			// registerExtensions が空の場合はファイルを1件も登録しない
			// (基本的に全て無視、指定された拡張子だけ登録するという設計)。
			CString lowerName = name;
			lowerName.MakeLower();

			int dotPos = lowerName.ReverseFind(_T('.'));
			CString ext = (dotPos >= 0) ? lowerName.Mid(dotPos) : CString();

			bool registered = false;
			for (const auto& regExt : ctx.registerExtensions)
			{
				if (!regExt.IsEmpty() && ext == regExt)
				{
					registered = true;
					break;
				}
			}
			if (!registered)
			{
				continue;
			}

			FileEntry entry;
			entry.fullPath = fullPath;
			entry.displayName = name;
			results.push_back(entry);
			++ctx.foundCount;
		}

		// 進捗通知は頻度を絞って送る(高速に大量ヒットした場合の
		// メッセージキュー圧迫を避けるため、150ms間隔でスロットリングする)。
		if (ctx.hMainWnd != nullptr)
		{
			ULONGLONG now = ::GetTickCount64();
			if (now - ctx.lastReportTick >= 150)
			{
				ctx.lastReportTick = now;
				::PostMessage(ctx.hMainWnd, WM_INDEX_PROGRESS, static_cast<WPARAM>(ctx.foundCount), 0);
			}
		}

	} while (::FindNextFile(hFind, &findData));

	::FindClose(hFind);
}

void CgleipnirDlg::BuildIndexAsync(const std::vector<CString>& driveRoots,
	const std::vector<CString>& registerExtensions,
	const std::vector<CString>& ignoreFolderNames)
{
	m_bIndexing = true;
	m_scanCancelRequested = false;

	// スキャン中であることを示すプログレスバーを表示・アニメーション開始
	m_progressBar.ShowWindow(SW_SHOW);
	m_progressBar.SetMarquee(TRUE, 30);
	SetWindowText(_T("gleipnir - スキャン準備中..."));

	IndexThreadParam* param = new IndexThreadParam();
	param->hMainWnd = m_hWnd;
	param->driveRoots = driveRoots;
	param->registerExtensions = registerExtensions;
	param->ignoreFolderNames = ignoreFolderNames;
	param->cancelFlag = &m_scanCancelRequested;

	AfxBeginThread(&CgleipnirDlg::IndexThreadProc, param);

	CString driveList;
	for (size_t i = 0; i < driveRoots.size(); ++i)
	{
		if (i > 0)
		{
			driveList += _T(", ");
		}
		driveList += driveRoots[i];
	}
	ShowTrayBalloon(_T("gleipnir"), driveList + _T(" のインデックスを作成中です..."));
}

// static
UINT CgleipnirDlg::IndexThreadProc(LPVOID pParam)
{
	std::unique_ptr<IndexThreadParam> param(static_cast<IndexThreadParam*>(pParam));

	ScanContext ctx;
	ctx.registerExtensions = param->registerExtensions;
	ctx.ignoreFolderNames = param->ignoreFolderNames;
	ctx.cancelFlag = param->cancelFlag;
	ctx.hMainWnd = param->hMainWnd;
	ctx.lastReportTick = ::GetTickCount64();

	std::vector<FileEntry>* pResults = new std::vector<FileEntry>();

	for (const auto& driveRoot : param->driveRoots)
	{
		if (ctx.cancelFlag != nullptr && ctx.cancelFlag->load())
		{
			break;
		}
		ScanDirectoryRecursive(driveRoot, *pResults, ctx);
	}

	bool canceled = (ctx.cancelFlag != nullptr && ctx.cancelFlag->load());

	IndexCompleteResult* pCompletion = new IndexCompleteResult();
	pCompletion->results = pResults;
	pCompletion->canceled = canceled;

	// 所有権をメインスレッド側に渡す
	::PostMessage(param->hMainWnd, WM_INDEX_COMPLETE, 0, reinterpret_cast<LPARAM>(pCompletion));

	return 0;
}

// static
LRESULT CALLBACK CgleipnirDlg::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && s_hMainWnd != nullptr)
	{
		const KBDLLHOOKSTRUCT* pkbhs = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);

		if (pkbhs->scanCode == SCANCODE_CAPSLOCK)
		{
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
			{
				::PostMessage(s_hMainWnd, WM_CAPSLOCK_TOGGLE, 0, 0);
			}

			// キーダウン・キーアップともにOSへ伝えず、Caps Lockのロック状態自体の
			// トグルおよびIME切り替え動作を抑止する。
			return 1;
		}
	}

	return CallNextHookEx(s_hKeyboardHook, nCode, wParam, lParam);
}
