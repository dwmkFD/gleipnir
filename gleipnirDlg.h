
// gleipnirDlg.h : ヘッダー ファイル
//

#pragma once

#include <vector>
#include <atomic>
#include "CResultListBox.h"
#include "KeyBinding.h"

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
	CString lowerName;    // displayName の小文字版(検索用キャッシュ。初回検索時に作成)
	bool isFolder = false; // フォルダ展開時の列挙でのみ設定(インデックス側では使わない)
};

// 候補リストの最大列挙数(設定「候補リスト → 最大列挙数」、config.ini [Common] MaxEnumCount)
constexpr int kDefaultMaxEnumCount = 200;  // 既定(従来の固定値と同じ)
constexpr int kMinMaxEnumCount = 1;
constexpr int kMaxMaxEnumCount = 10000;    // リストボックスへの追加が重くなりすぎない上限

// 検索方式(設定「検索方式」。config.ini には prefix / substring / skip で保存)
enum class SearchMethod
{
	Prefix,     // 先頭部分一致: 名前が入力語で始まる
	Substring,  // 中間部分一致: 名前のどこかに入力語を含む
	Skip,       // スキップマッチング: 入力語の文字が順番どおりに含まれる(間に他の文字があってよい)
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
	CResultListBox m_listResults; // アイコン付きオーナードロー
	CProgressCtrl m_progressBar;

	// 外観。config.ini [Common] の TextColor / BackgroundColor / Font から
	// 読み込む(未指定・不正値の場合は既定の 緑文字 / 黒背景 / メイリオ太字)。
	CFont m_font;
	CBrush m_bgBrush;
	COLORREF m_textColor = RGB(0, 255, 0);
	COLORREF m_bgColor = RGB(0, 0, 0);
	int m_editMinHeight = 0; // .rc で決めた検索欄の高さ(フォントに合わせて広げる際の下限)
	void ApplyAppearance();

	// 色文字列("#RRGGBB" / "RRGGBB" / "R,G,B")を解釈する。不正なら false。
	static bool ParseColorString(const CString& text, COLORREF& color);
	// フォント文字列("書体名[,サイズ(pt)][,bold|normal]")を解釈し、lf に反映する。
	// 省略された要素は lf の値をそのまま残す。サイズ変換に hDC を使う。
	static void ParseFontString(const CString& text, LOGFONT& lf, HDC hDC);
	// config.ini の CandidateIcon の値を解釈する("0"/"非表示" 以外は表示)。
	static bool IsCandidateIconEnabled(const CString& value);

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
	// 入力欄・候補をクリアしてから(設定 ClearInputOnHide が有効な場合)非表示にする。
	void HideLauncherWindow();
	void ToggleVisibility();	// ウィンドウの表示/非表示を切り替える

	void CreateDynamicControls();
	void UpdateSearchResults(const CString& query);

	// 検索方式の設定(config.ini [Common] SearchMethodNormal / SearchMethodFolderExpand)。
	// フォルダ展開時の方式は、フォルダ展開機能の実装時に使う(現状は読み込みのみ)。
	SearchMethod m_searchMethodNormal = SearchMethod::Substring;
	SearchMethod m_searchMethodFolderExpand = SearchMethod::Skip;
	// フォルダ展開キー(config.ini [Common] FolderExpandKey / ParentFolderExpandKey。
	// 既定: Ctrl+F / Ctrl+B)。LoadSearchSettings() で一緒に読み込む。
	KeyBinding m_folderExpandKey = DefaultFolderExpandKey();
	KeyBinding m_parentFolderExpandKey = DefaultParentFolderExpandKey();
	// 候補リストの選択を上下に動かすキー(config.ini [Common] SelectDownKey / SelectUpKey。
	// 既定: Ctrl+N / Ctrl+P)。
	KeyBinding m_selectDownKey = DefaultSelectDownKey();
	KeyBinding m_selectUpKey = DefaultSelectUpKey();
	// 通常検索で表示する最大件数(起動履歴の一致分を含む。フォルダ展開時は別枠)。
	int m_maxEnumCount = kDefaultMaxEnumCount;
	// 選択したパスが存在しなかったとき、インデックス・起動履歴から削除するか
	// (config.ini [Common] AutoDeleteBrokenPath。既定: 0 = 削除しない)。
	bool m_autoDeleteBrokenPath = false;
	void LoadSearchSettings();
	// 押されたキーが設定画面で割り当てたキー(フォルダ展開/親フォルダ展開/下選択/上選択)
	// なら処理し、true を返す。
	bool HandleAssignedKey(const MSG* pMsg);
	// 候補リストの選択を delta 行動かす(端で止まる。未選択なら先頭/末尾を選ぶ)。
	void MoveResultSelection(int delta);

	// フォルダ展開。選択中の候補がフォルダなら(キー押下の時点で実在を確認)、
	// その中身を実際に列挙してリストに表示し、入力欄はその中の絞り込みに使う。
	// 展開中でなければ m_expandedFolder は空。
	CString m_expandedFolder;
	std::vector<FileEntry> m_folderItems;  // 展開中フォルダの中身(展開時に1回だけ列挙)
	bool ExpandSelectedFolder();           // 選択中の候補がフォルダなら展開する
	bool ExpandFolder(const CString& folderPath, const CString& selectPath = CString());
	bool ExpandParentFolder();             // 展開中フォルダの親へ(ルートなら展開を終了)
	void LeaveFolderExpansion();           // 通常の検索モードへ戻る
	void UpdateFolderExpandResults(const CString& query);
	static bool IsDirectoryPath(const CString& path);
	static void EnumerateFolder(const CString& folderPath, std::vector<FileEntry>& items);
	static CString GetParentFolderPath(const CString& folderPath); // ルートなら空
public:
	static SearchMethod ParseSearchMethod(const CString& value, SearchMethod defaultMethod);
	static LPCTSTR SearchMethodToConfigValue(SearchMethod method);
	// 入力を空白(半角/全角)で区切った語の配列にし、小文字化する(AND検索用)。
	static std::vector<CString> SplitQueryTerms(const CString& query);
	// 小文字化済みの名前が全ての語に一致するか判定し、一致すれば score に
	// 並び順用の値(小さいほど上位)を返す。
	static bool MatchName(const CString& lowerName, const std::vector<CString>& terms,
		SearchMethod method, int& score);
protected:
	void LaunchSelectedResult();
	// 起動に成功したパスを履歴の先頭に記録し、data\history.txt へ保存する。
	void RecordLaunchHistory(const CString& fullPath);

	// リンク切れ(選択したパスが存在しない)の処理。存在しない旨を表示し、
	// 設定が有効ならインデックス・起動履歴から削除して検索結果を更新する。
	// sel は候補リスト上の行番号(削除後の選択位置の復元に使う)。
	void HandleBrokenPath(const CString& path, int sel);
	// path を含むドライブ(または UNC の共有)自体にアクセスできるか。
	// 外付けドライブの取り外し・ネットワーク切断中に誤って削除しないための確認。
	static bool IsVolumeAvailable(const CString& path);
	// インデックスと起動履歴から path(フォルダなら配下も)を取り除く。取り除いた件数を返す。
	size_t RemovePathFromIndex(const CString& path);

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
	// config.ini が無ければ全項目を既定値で作成し(UTF-16LE)、あれば足りない項目だけを
	// 既定値で書き足す。旧版で作られた ANSI(Shift-JIS)の config.ini は UTF-16LE に変換する。
	static void EnsureSettingsFile();
	// BOM の無い(ANSI)設定ファイルを UTF-16LE(BOM付き)に変換する。
	static void ConvertProfileToUtf16(const CString& path);
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
