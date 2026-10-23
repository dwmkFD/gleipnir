
// gleipnirDlg.h : ヘッダー ファイル
//

#pragma once

#include <vector>
#include <atomic>

// タスクトレイアイコンのコールバックメッセージ
#define WM_TRAYICON (WM_APP + 1)

// 低レベルキーボードフックから Caps Lock 検知を通知するためのメッセージ
#define WM_CAPSLOCK_TOGGLE (WM_APP + 2)

// インデックス作成完了を通知するためのメッセージ(ワーカースレッド → メインスレッド)
#define WM_INDEX_COMPLETE (WM_APP + 3)

// インデックス作成の途中経過(件数)を通知するためのメッセージ
#define WM_INDEX_PROGRESS (WM_APP + 4)

// タスクトレイメニューのコマンドID
#define ID_TRAY_EXIT               32772
#define ID_TRAY_SCAN               32773
#define ID_TRAY_EDIT_SCAN_INI      32774
#define ID_TRAY_EDIT_INSTANT_INI   32775
#define ID_TRAY_RELOAD_SETTINGS    32776
#define ID_TRAY_OPEN_DATA_FOLDER   32777
#define ID_TRAY_CANCEL_SCAN        32778

// 動的生成するコントロールのID
#define IDC_LIST_RESULTS   2001
#define IDC_PROGRESS_SCAN  2002

// 検索対象の1エントリ(ファイル/フォルダ)
struct FileEntry
{
	CString fullPath;     // フルパス(起動・検索対象)
	CString displayName;  // ファイル名のみ(検索・表示用)
};

// instant.ini の [InstantCommand] セクションの1エントリ
struct InstantCommandEntry
{
	CString keyword;      // 検索欄の先頭に一致させるキーワード(例: "g/")
	bool needsArg;        // コマンドラインに %A を含む(引数入力が必要)か
	// ※セキュリティ上、コマンドライン本体はメモリに常駐させない。
	//   実行のたびに instant.ini から読み直し、使用後は上書き消去する。
};

// スキャン処理の設定・進捗・キャンセル状態を束ねる構造体。
// ワーカースレッド内で ScanDirectoryRecursive に渡され、進捗通知や
// キャンセル判定に使われる。
struct ScanContext
{
	std::vector<CString> registerExtensions;  // ホワイトリスト(拡張子)
	std::vector<CString> ignoreFolderNames;   // 除外フォルダ名キーワード
	std::atomic<bool>* cancelFlag = nullptr;  // trueになったらスキャンを中断する
	HWND hMainWnd = nullptr;                  // 進捗通知(WM_INDEX_PROGRESS)の送り先
	size_t foundCount = 0;                    // ここまでに見つかった件数
	ULONGLONG lastReportTick = 0;             // 前回進捗通知を送った時刻(GetTickCount64)
};

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
	virtual BOOL PreTranslateMessage(MSG* pMsg);


// 実装
protected:
	HICON m_hIcon;
	NOTIFYICONDATA m_nid;		// タスクトレイアイコン情報
	bool m_bTrayIconAdded;		// タスクトレイアイコンが追加済みかどうか

	// 検索結果表示用リストボックス、スキャン中プログレスバー(いずれも動的生成)
	CListBox m_listResults;
	CProgressCtrl m_progressBar;

	// 外観(黒背景・緑文字・メイリオ太字)。将来 config.ini の
	// 文字色/背景色/フォント設定と連動させる想定。
	CFont m_font;
	CBrush m_bgBrush;
	COLORREF m_textColor = RGB(0, 255, 0);
	COLORREF m_bgColor = RGB(0, 0, 0);
	void ApplyAppearance();

	// ファイルインデックス
	std::vector<FileEntry> m_fileIndex;       // 保存/読み込み済みの全件
	std::vector<FileEntry> m_filteredResults; // 直近の検索結果(リストボックスの行と対応)
	std::vector<CString> m_launchHistory;     // 起動履歴(フルパス、先頭ほど最近使った)
	bool m_bIndexing;                         // インデックス作成中かどうか
	std::atomic<bool> m_scanCancelRequested;  // スキャンのキャンセルが要求されたか

	// instant.ini から読み込んだインスタントコマンド一覧
	std::vector<InstantCommandEntry> m_instantCommands;

	void AddTrayIcon();
	void RemoveTrayIcon();
	void ShowTrayBalloon(const CString& title, const CString& text);
	void ToggleVisibility();	// ウィンドウの表示/非表示を切り替える

	void CreateDynamicControls();
	void UpdateSearchResults(const CString& query);
	void LaunchSelectedResult();
	// 起動に成功したパスを履歴の先頭に記録し、data\history.txt へ保存する。
	void RecordLaunchHistory(const CString& fullPath);

	// スキャン実行前の確認ダイアログ。Yesならtrueを返す。
	bool ConfirmScan(const CString& message);
	// 確認ダイアログ表示 → scan.ini読み込み → スキャン開始、までを一括して行う。
	// トレイメニューの「スキャン実行」と、Edit Controlへの「/scan」入力の
	// どちらからも、この共通処理を呼び出す。
	void ExecuteScanWithConfirmation();
	// 実行中のスキャンにキャンセルを要求する(確認ダイアログ無し、即時)。
	// スキャン中でなければ何もしない。
	void RequestCancelScan();

	// instant.ini ([InstantCommand]セクション) を読み込み、m_instantCommands
	// (キーワードと引数要否のみ。コマンドライン本体は保持しない)を更新する。
	void LoadInstantCommands();
	// 指定キーワードのコマンドライン本体を instant.ini から読み出す。
	// 呼び出し側は使用後に必ず上書き消去すること。見つかればtrue。
	bool ReadInstantCommandLine(const CString& keyword, CString& commandLine) const;
	// 入力文字列がinstant.iniのいずれかのキーワードで始まっていれば、
	// 該当コマンドを実行してtrueを返す。一致しなければfalseを返す
	// (呼び出し側は通常の検索結果起動にフォールバックしてよい)。
	bool TryExecuteInstantCommand(const CString& input);
	// 入力がinstant.iniのキーワードと完全一致し、他に続く候補が無く、かつ
	// %Aを含まない(引数不要)場合にtrue。Enter無しで自動実行する判定に使う。
	bool ShouldAutoExecuteInstant(const CString& input) const;
	// コマンドライン中の %A / %C / %W / %V / %scanfile / %pathfile /
	// %initfile / %cmddir を実際の値に置換する。
	CString ExpandInstantMacros(const CString& commandLine, const CString& arg) const;

	// "file" "params..." 形式のコマンドラインを ShellExecute 用に
	// file(先頭トークン、クォート考慮) と params(残り) に分割する。
	static void SplitCommandLine(const CString& cmdLine, CString& file, CString& params);
	static CString GetClipboardText(HWND hWndOwner);
	// 文字列をクリップボード(CF_UNICODETEXT)へ設定する。成功時true。
	static bool SetClipboardText(HWND hWndOwner, const CString& text);

	// data フォルダに無ければ既定のテンプレート内容で作成する
	// (scan.ini / instant.ini を初めて編集しようとした際に使う)。
	static void EnsureConfigFileExists(const CString& path, const CString& defaultContent);
	void OpenInNotepad(const CString& path);

	// exe自身があるフォルダのパスを返す(dataフォルダの親)。
	static CString GetExeFolderPath();
	// data フォルダ(exeと同じ場所に作成)のパスを返す。
	// インデックスファイル・scan.ini・instant.ini・今後追加予定のアプリ
	// 設定ファイルもこの下に置く想定の共通ヘルパー。
	static CString GetDataFolderPath();
	static CString GetIndexFilePath();
	static CString GetHistoryFilePath();
	static CString GetScanConfigFilePath();
	static CString GetInstantConfigFilePath();
	// 現時点では設定ダイアログ本体は未実装だが、instant.iniの%initfile
	// マクロや将来の設定保存先として先にパスだけ定義しておく。
	static CString GetSettingsFilePath();

	// scan.ini を読み込み、対象ドライブ・登録対象拡張子(ホワイトリスト)・
	// 除外フォルダ名を取得する。
	// 戻り値: スキャンを実行すべきかどうか。
	//   - scan.ini が存在しない場合: true を返し、driveRoots には
	//     現在の固定ドライブ(DRIVE_FIXED)のみを設定する。
	//   - scan.ini は存在するが [DRIVE] セクションの TARGET が
	//     取得できない場合: false を返す(呼び出し側はスキャンしない)。
	//
	// registerExtensions はホワイトリストであり、ここに含まれる拡張子を
	// 持つファイルのみインデックスに登録する(index.txt肥大化対策のため、
	// 「基本的に全て無視、指定された拡張子だけ登録する」という設計)。
	// 空の場合はファイルを1件も登録しない(フォルダのみ登録される)。
	static bool LoadScanConfig(std::vector<CString>& driveRoots,
		std::vector<CString>& registerExtensions,
		std::vector<CString>& ignoreFolderNames);

	static void SaveIndexToFile(const CString& filePath, const std::vector<FileEntry>& entries);
	static bool LoadIndexFromFile(const CString& filePath, std::vector<FileEntry>& entries);
	static void SaveHistoryToFile(const CString& filePath, const std::vector<CString>& history);
	static bool LoadHistoryFromFile(const CString& filePath, std::vector<CString>& history);
	static void ScanDirectoryRecursive(const CString& path, std::vector<FileEntry>& results, ScanContext& ctx);

	void BuildIndexAsync(const std::vector<CString>& driveRoots,
		const std::vector<CString>& registerExtensions,
		const std::vector<CString>& ignoreFolderNames);
	static UINT IndexThreadProc(LPVOID pParam);

	// Caps Lock を横取りするための低レベルキーボードフック。
	// (JIS配列では単独押下時に vkCode が VK_DBE_ALPHANUMERIC に化けるため、
	//  scanCode で判定している。詳細は .cpp 内のコメント参照)
	static HHOOK s_hKeyboardHook;
	static HWND s_hMainWnd;
	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCapsLockToggle(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIndexComplete(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIndexProgress(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTrayExit();
	afx_msg void OnTrayScan();
	afx_msg void OnTrayCancelScan();
	afx_msg void OnTrayEditScanIni();
	afx_msg void OnTrayEditInstantIni();
	afx_msg void OnTrayReloadSettings();
	afx_msg void OnTrayOpenDataFolder();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnLbnDblclkListResults();
	DECLARE_MESSAGE_MAP()
};
