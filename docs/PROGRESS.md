# gleipnir 開発進捗メモ

最終更新: 2026-08-26

## 概要
Windows常駐ランチャー。Fenrirに類似した機能を目指す。
VS2022でMFCダイアログベーステンプレートを生成し、それをベースに実装を進めている。

- ビルド・デバッグ・実機確認: ユーザー側（Visual Studio 2022）
- コード実装: Claude（この作業ログを引き継いで作業を継続する）

## 完了した実装

### 1. タスクトレイ常駐
- `Shell_NotifyIcon` でトレイにアイコンを常駐。
- ×ボタン (`OnClose`) / 最小化 (`OnSysCommand`の`SC_MINIMIZE`) では
  プロセスを終了せず、`ShowWindow(SW_HIDE)`のみ行う。
- トレイアイコンをダブルクリックすると表示/非表示をトグル。
- 終了はトレイメニューの「終了」から `EndDialog(IDCANCEL)` で行う。

### 2. Caps Lockキーによる表示/非表示切り替え
- `WH_KEYBOARD_LL`の低レベルキーボードフックで、`KBDLLHOOKSTRUCT::scanCode`
  が`0x3A`(Caps Lock物理キーの位置)であることを判定して表示/非表示を切り替え。
- vkCodeではなくscanCodeで判定している理由: JIS配列キーボードでは
  単独押下時に`VK_CAPITAL`ではなく`VK_DBE_ALPHANUMERIC`(IME用仮想キー)に
  変換されてしまうため。scanCodeはレイアウトドライバの変換前のハードウェア値
  なのでUS/JIS共通で判定できる。
- US/JIS双方で動作確認済み。

### 3. ランチャー本体機能(今回追加)
- **インデックス作成**: トレイアイコン右クリック→「インデックス再構築」
  サブメニューから、検出済みの固定/リムーバブルドライブを選んで実行。
  `AfxBeginThread`によるバックグラウンドスレッドで再帰的に全ファイル/
  フォルダを列挙し、UIをブロックしない。完了時はトレイのバルーン通知で
  件数を表示。
- **保存/読み込み**: `%LOCALAPPDATA%\gleipnir\index.txt`に保存
  (UTF-16LE、1行1パスの単純なテキスト形式、表示名は読み込み時に
  フルパスから再計算)。次回起動時に自動で読み込む。
- **インクリメンタル検索**: `IDC_EDIT1`への入力のたびにファイル名部分一致
  (大文字小文字区別なし)でフィルタし、動的生成したリストボックス
  (`IDC_LIST_RESULTS`)に反映(最大200件、パフォーマンス配慮)。
- **起動**: リストをダブルクリック、またはEnterキーで選択項目を
  `ShellExecute`で起動。成功したら検索窓をクリアして自動的に非表示化。

### 4. スキャン実行前の確認・進行中表示・設定ファイル(scan.ini)対応
- **実行確認**: トレイメニューの「スキャン実行」、および`IDC_EDIT1`に
  「/scan」(大文字小文字区別なし)が入力された時のいずれも、
  `MessageBox(MB_YESNO)`で確認ダイアログを表示し、「はい」の場合のみ
  スキャンを開始する。両トリガーとも共通処理`ExecuteScanWithConfirmation()`
  を呼び出す形に統一した。
- **進行中表示**: スキャン中は`IDC_EDIT1`の下に配置したプログレスバー
  (`IDC_PROGRESS_SCAN`)をマーキー(不確定)スタイルで表示し、完了したら
  非表示に戻す。総ファイル数が事前にわからないため、パーセンテージ
  ではなく「動作中であること」を示すアニメーションのみとした。
- **完了通知**: スキャン完了時にトレイのバルーン通知に加えて、
  検出件数を含む完了ダイアログ(`MessageBox`)を表示するようにした。
- **設定ファイル `data\scan.ini`**:
  - `[DRIVE]` セクションの `TARGET=C:,D:,...` でスキャン対象ドライブを
    カンマ区切りで指定できる。
  - `[DRIVE]` セクション(`TARGET`キー)が無い場合はスキャンを実行せず、
    その旨をダイアログで通知する。
  - `scan.ini` 自体が存在しない場合は、既定として**現在の固定ドライブ
    (`DRIVE_FIXED`)のみ**を対象にする(リムーバブルドライブは対象外)。
  - `[regist]` セクションの `REGIST=*.txt,*.doc,...` で、**インデックスに
    登録する**拡張子をホワイトリスト形式で指定する(ワイルドカードの
    `*`部分は無視し、拡張子の一致のみで判定)。
    - 当初は`[ignore] IGNORE=...`による除外拡張子(ブラックリスト)方式
      だったが、`index.txt`が肥大化しすぎるため、**逆に登録したい
      拡張子だけを指定するホワイトリスト方式**に変更した。
    - `REGIST`が未指定(またはキー自体が無い)場合、ファイルは
      **1件もインデックスに登録されない**(基本的に全て無視、指定された
      拡張子だけ登録するという設計を文字通り実装)。フォルダ自体は
      (`FOLDERNAME`による除外に該当しない限り)引き続き登録される。
  - `[ignore]` セクションの `FOLDERNAME=***,...` で、フォルダ名に
    指定文字列を含む(部分一致、大文字小文字区別なし)フォルダを
    スキャン対象外にできる(該当フォルダ自体も配下も探索しない)。
    こちらは従来通りブラックリスト方式のまま。
  - INI値の読み込みには`GetPrivateProfileString`を使用。
- **設計変更点**: 従来はトレイメニューに検出ドライブ一覧のサブメニューを
  表示し、選んだ1ドライブのみをスキャンする方式だったが、`scan.ini`での
  ドライブ指定と役割が重複するため、トレイメニューは「スキャン実行」の
  1項目に統合した。個別ドライブを指定したい場合は`scan.ini`の`TARGET`で
  指定する運用に変更している。
  - 補足: ユーザー指示内で設定ファイル名が「scan.ini」「scan.txt」の
    両方で言及されていたが、内容がINI形式(`[セクション]`+`KEY=値`)
    であるため、ファイル名は`scan.ini`として実装した。

### 5. instant.ini対応・設定メニュー(今回追加)
- **instant.ini (`data\instant.ini`)**: 本家Fenrirの`instant.ini`と
  ほぼ同じ書式を採用した。
  - 書式: `[InstantCommand]`セクションに`キーワード=コマンドライン`。
  - 検索欄(`IDC_EDIT1`)に「キーワードで始まる文字列」を入力してEnterを
    押すと、キーワードより後ろの文字列を`%A`に差し込んだ上でコマンドを
    実行する(`PreTranslateMessage`のEnter処理で、通常の検索結果起動より
    先にinstant.iniのキーワード一致を試みる)。
  - 対応マクロ: `%A`(引数) `%C`(クリップボード文字列)
    `%W`(gleipnir.exeのフォルダ) `%V`(そのドライブ) `%scanfile`
    `%pathfile` `%initfile`(設定ファイル、後述) `%cmddir`(dataフォルダ)。
  - **ダブルクォーテーションの二重表記**: `GetPrivateProfileString`は
    値の前後が`"`で囲まれている場合、外側の1組だけを自動的に取り除く
    仕様がある。本家Fenrirはこれを逆手に取り、パスにスペースを含む
    コマンドラインを`""C:\Program Files\foo\foo.exe"" "%A"`のように
    二重に囲んで記述する(読み込み後に外側の1組が剥がれ、正しい
    コマンドラインが残る)。本実装も同じAPIを使っているため、この
    慣習がそのまま通用する。
  - コマンドラインの実行は`file`部分と`params`部分に分割して
    `ShellExecute`に渡す(`SplitCommandLine`)。これにより
    `"exeパス" "引数"`のような形式と、`"http://...URL..."`のような
    単体のURL文字列のどちらも扱える。
  - `instant.ini`が存在しない場合は何もしない(全ての通常検索が
    そのまま機能する)。
  - キーワードの一致判定は大文字小文字を区別する(本家Fenrirに合わせた)。
- **設定関連のトレイメニュー(暫定)**: 「設定」サブメニューを追加し、
  以下を実行できるようにした。
  - `scan.ini を編集`: 無ければ既定テンプレートを作成した上でメモ帳で開く。
  - `instant.ini を編集`: 同上。
  - `設定を再読み込み`: `instant.ini`を再読み込みする
    (`scan.ini`はスキャン実行の都度読み直すため、再読み込みボタンは
    実質`instant.ini`専用)。
  - `データフォルダを開く`: `data`フォルダをエクスプローラーで開く。
  - これは「設定ダイアログ」の**暫定実装**であり、GUIフォームではなく
    トレイメニュー経由でのテキストファイル編集という形になっている
    (理由は下記「設定ダイアログ本体について」を参照)。

#### 設定ダイアログ本体について(2026-08-26 追記: リソース追加・実装完了)
- ユーザー側でVisual Studioのリソースエディタに`IDD_SETTING_DIALOG`
  (タブコントロール`IDC_SETTING_DIALOG_TAB`)・`IDD_SETTING_DIALOG_COMMON`・
  `IDD_SETTING_DIALOG_OTHER`を追加していただいた。
- Claude側で`CSettingDlg`/`CSettingCommonDlg`/`CSettingOtherDlg`を実装し、
  タブ切り替えと`/option`コマンドでの起動に対応した。詳細は
  「7. 設定ダイアログ本体・タブ構成・「/option」起動」を参照。
- 以前の依頼内容(ダイアログテンプレートの追加手順)はこれで解消したため、
  本項は経緯の記録として残す。


## 実装の詳細・設計メモ
- **データ保存先の方針**: インデックスファイル(および今後追加予定の
  スキャン設定・アプリ設定ファイル)は、`%LOCALAPPDATA%`のような
  ユーザープロファイル依存の場所ではなく、**gleipnir.exeが置かれている
  フォルダ直下の`data`フォルダ**に保存する方針とした。
  - 理由: ポータブル運用(USB等に入れて持ち運ぶ、複数環境で共有する等)を
    想定し、実行ファイルと設定/データを同じ場所にまとめておきたいため。
  - 実装: `GetDataFolderPath()`が`GetModuleFileName`でexeの場所を取得し、
    その直下に`data`フォルダを作成して返す共通ヘルパー。
    インデックスファイルパスはこれに`index.txt`を付与して取得
    (`GetIndexFilePath()`)。今後の設定ファイルもこのヘルパー経由で
    `data`フォルダ以下のパスを組み立てる想定。
  - 例: `(gleipnir.exeのフォルダ)\data\index.txt`、
    将来的に `(gleipnir.exeのフォルダ)\data\config.ini` 等を追加予定。
- リストボックス(`IDC_LIST_RESULTS`)は`.rc`を編集せず、`OnInitDialog`内で
  `IDC_EDIT1`の位置を基準にコードから動的生成している(`.rc`の内容を
  直接編集できないための対応)。
- インデックス作成完了は`WM_INDEX_COMPLETE`(`WM_APP+3`)でワーカースレッド
  からメインスレッドへ結果ポインタの所有権を渡す形で通知している。
- シンボリックリンク/ジャンクション(`FILE_ATTRIBUTE_REPARSE_POINT`)は
  無限ループ防止のため辿らない。
- アクセス不可なディレクトリは`FindFirstFile`が失敗するだけで、
  特別なエラー処理はせずスキップする。

### 6. インデックス作成の詳細な進捗表示・キャンセル機能(今回追加)
- **タイトルバーへの進捗表示**: スキャン中はタイトルバーに
  「gleipnir - スキャン中: N件」の形式で、見つかった件数を随時更新表示する。
  - ワーカースレッド側で新規/フォルダを1件見つけるたびにカウントを増やし、
    150msごとにスロットリングして`WM_INDEX_PROGRESS`(`WM_APP+4`)を
    メインスレッドへPostMessageする(高速に大量ヒットした場合の
    メッセージキュー圧迫を避けるため)。
  - 完了時は「gleipnir - 検索完了：N件」に更新(従来からの完了ダイアログも
    そのまま表示する)。
  - キャンセルされた場合は「gleipnir - スキャンを中断しました(N件、未保存)」
    に更新し、完了ダイアログの代わりにキャンセルされた旨のダイアログを表示する。
- **キャンセル機能**: `std::atomic<bool> m_scanCancelRequested`を
  ワーカースレッドとメインスレッドで共有し、キャンセル要求時に`true`を
  立てる。ワーカースレッド(`ScanDirectoryRecursive`)は各ディレクトリの
  列挙開始時、および列挙ループの各反復でこのフラグを確認し、
  `true`であれば速やかに処理を打ち切って呼び出し元へ戻る(再帰全体が
  連鎖的に終了する)。
  - **キャンセル時は`index.txt`を上書きしない**仕様とした
    (`OnIndexComplete`で`canceled`フラグを見て、`SaveIndexToFile`の
    呼び出し自体をスキップする)。
  - トリガー: トレイメニューの「スキャンをキャンセル」(スキャン中のみ
    有効化)、または`IDC_EDIT1`への`/scan stop`・`/scan cancel`入力
    (両者は同義語として扱う)。
  - スキャン中でない時にキャンセルを要求した場合は、トレイバルーンで
    「実行中のスキャンはありません。」と通知するのみで何もしない。
- **`/scan`コマンドの拡張**: 後方互換のため単体の`/scan`は引き続き
  「開始」として扱う。加えて`/scan start`(開始の明示形)、
  `/scan stop`・`/scan cancel`(キャンセル)に対応した。
  - 前後の空白はトリムした上で完全一致で判定するシンプルな実装。
    サブコマンドの追加(例: `/scan status`)は今後の拡張候補。
- **スキャン中の終了保護**: `m_bIndexing`がtrueの間はトレイメニューの
  「終了」を選んでも警告ダイアログを表示するのみで終了しない
  (先にキャンセルしてもらう運用)。これは、スキャン用ワーカースレッドが
  メインウィンドウへ`PostMessage`する前提でウィンドウが破棄されないことを
  保証するための安全策でもある。


### 7. 設定ダイアログ本体・タブ構成・「/option」起動
- ユーザー側でVisual Studioのリソースエディタに以下を追加済み
  (`resource.h`は本セッション開始前から`IDD_SETTING_DIALOG`等を定義済み):
  - `IDD_SETTING_DIALOG`(タブコントロール`IDC_SETTING_DIALOG_TAB`を含む)
  - `IDD_SETTING_DIALOG_COMMON`(「共通」タブページ)
  - `IDD_SETTING_DIALOG_OTHER`(「その他」タブページ)
- **クラス**(GitHubリポジトリでは`C`プレフィックス付きで管理):
  - `CSettingDlg`(`CSettingDlg.h/.cpp`): 設定ダイアログ本体。
    `OnInitDialog`でタブに「共通」「その他」の2項目を追加し、
    `CreateTabPages()`でタブの表示領域(見出しを除いた内側の矩形)を
    計算した上に`CSettingCommonDlg`/`CSettingOtherDlg`を子ウィンドウとして
    生成・重ねて配置する。`TCN_SELCHANGE`通知でタブ切り替え時に
    どちらか一方だけを`ShowWindow`で表示する。
  - `CSettingCommonDlg`(`CSettingCommonDlg.h/.cpp`):
    `IDD_SETTING_DIALOG_COMMON`に対応する子ダイアログ。
  - `CSettingOtherDlg`(`CSettingOtherDlg.h/.cpp`): 同上、`IDD_SETTING_DIALOG_OTHER`用。
- **重要な前提条件**: `IDD_SETTING_DIALOG_COMMON`/`IDD_SETTING_DIALOG_OTHER`の
  各ダイアログリソース自体のプロパティで、`Style=Child`・`Border=None`・
  タイトルバー/システムメニュー無効、になっている必要がある。
  これらが通常の`Style=Popup`のままだと、タブの中に埋め込まれず
  別ウィンドウとして浮いて表示されてしまう。
- **起動方法**: `gleipnirDlg`の`IDC_EDIT1`に`/option`(大文字小文字区別なし)
  と入力すると、`CSettingDlg`をモーダル(`DoModal()`)で表示する
  (`/scan`系コマンドと同じ`OnEnChangeEdit1`内で判定)。
  トレイメニューへのショートカット追加は見送った(必要であれば
  「設定」サブメニューに1項目追加するだけで対応可能)。
- **レイアウト上の注意**: `IDD_SETTING_DIALOG_COMMON`(309×309)は、
  親の`IDD_SETTING_DIALOG`のタブ表示領域(263×237程度)より大きいため、
  現状だと右・下がはみ出す可能性がある。気になる場合は
  `IDD_SETTING_DIALOG`本体を大きくすることを検討。

### 8. 設定ダイアログの値の保存/読み込み・config.ini対応(今回追加)
GitHubリポジトリ(`dwmkFD/gleipnir`)の`resource.h`/`gleipnir.rc`を実際に
読み込み、各タブページ内のコントロールが何を表すラベルかを確認した上で実装した。

- **`IDC_EDIT1`→`IDC_EDIT_HOTKEY`にリネーム**: `IDD_SETTING_DIALOG_COMMON`内の
  「呼び出しキー」欄が、メイン検索ウィンドウの検索欄と同じ`IDC_EDIT1`(値1000)を
  使っていたため、新規ID`IDC_EDIT_HOTKEY`(値1013)に変更(`resource.h`更新、
  `_APS_NEXT_CONTROL_VALUE`も1014に修正)。
  - 呼び出しキーは現状Caps Lock固定で変更不可のため、`.rc`側で
    `ES_READONLY | WS_DISABLED`を付与し、`CSettingCommonDlg::OnInitDialog`側でも
    `SetDlgItemText(IDC_EDIT_HOTKEY, "Caps Lock")` + `EnableWindow(FALSE)`を実行
    (保存対象からは除外)。
- **`gleipnir.rc`の副次的な修正**: `IDD_SETTING_DIALOG_OTHER`のリソースに、
  タブ埋め込み用ページにもかかわらず独自のOK/キャンセルボタン
  (`IDOK`/`IDCANCEL`)が残っていた。埋め込み子ウィンドウの状態でこれらを
  クリックすると`CDialogEx`既定の`EndDialog()`が呼ばれ、モードレスの
  子ダイアログに対して未定義動作となる恐れがあったため、`.rc`からボタンを
  削除。念のため`CSettingOtherDlg::OnOK()`/`OnCancel()`を空実装で
  オーバーライドし、二重に安全策とした。
- **`data\config.ini`への保存/読み込み**:
  - `CSettingCommonDlg`に各コントロールに対応するメンバー変数を追加し、
    `DoDataExchange`で`DDX_Text`/`DDX_CBString`/`DDX_Check`によりバインド。
  - `LoadFromConfig(configPath)`: `GetPrivateProfileString`/`GetPrivateProfileInt`で
    `[Common]`セクションから読み込み、`UpdateData(FALSE)`でコントロールへ反映。
  - `SaveToConfig(configPath)`: `UpdateData(TRUE)`でコントロールの現在値を
    取り込んだ上で`WritePrivateProfileString`により書き込み。
  - `CSettingDlg::CreateTabPages()`内で子ページ生成後に両ページの
    `LoadFromConfig`を呼び出し、`CSettingDlg::OnOK()`をオーバーライドして
    OK押下時に両ページの`SaveToConfig`を呼んでから`CDialogEx::OnOK()`を
    実行(閉じる前に保存)。キャンセル時は保存しない。
  - `CSettingOtherDlg`は現時点で保存対象のコントロールが無いため、
    `LoadFromConfig`/`SaveToConfig`は空実装(将来コントロール追加時に実装)。
  - 設定ファイルパスは`CgleipnirDlg::GetDataFolderPath()`と同じ規則
    (exeのフォルダ直下の`data`フォルダ)。クラスをまたぐ依存を避けるため、
    `CSettingDlg::GetConfigFilePath()`として同ロジックを複製している
    (将来的に共通ユーティリティへ切り出す余地あり)。
  - `[Common]`に保存されるキー: `FolderExpandKey` / `ParentFolderExpandKey` /
    `SelectDownKey` / `SelectUpKey` / `SearchMethodNormal` /
    `SearchMethodFolderExpand` / `CandidateIcon` / `MaxEnumCount` /
    `AutoDeleteBrokenPath`(0/1) / `TextColor` / `BackgroundColor` / `Font`。
- **未実装(値の保存/読み込みのみ)**: これらの設定値は`config.ini`に
  保存・読み込みされるだけで、実際のランチャー動作(検索方式の切り替え、
  候補アイコンの表示、文字色/背景色/フォントの反映、フォルダ展開キーの
  実キー割り当て等)にはまだ反映されていない。今回のスコープは
  「値の永続化」のみ。


## 既知の制約・今後の改善候補
- スキャン中はタイトルバーで件数を確認できるが、進捗率(%)や
  残りドライブ数などの詳細情報は表示していない。
- キャンセルの反映には若干のタイムラグがある(現在処理中のディレクトリの
  列挙が終わるか、ループの次の反復に達するまでは中断されない)。
- `/scan start` `/scan stop` `/scan cancel` は前後の空白をトリムした上での
  完全一致判定。表記ゆれ(例: 全角スペース)には対応していない。
- 検索は部分一致のみ(あいまい検索・優先度付けなし)。
- リストボックスとエディットボックス間の矢印キーでのフォーカス移動などは未実装。
- ウィンドウ位置・サイズの記憶は未実装(毎回既定位置に表示)。
- フックチェーンはLIFO。gleipnir起動後に他の低レベルフック常駐ソフトを
  (再)起動すると再び横取りされる可能性がある。
- 管理者権限で動くプロセスが前面にある場合、UIPI制限で
  標準権限プロセスの低レベルフックが届かないことがある。
- `instant.ini`のキーワード一致は前方一致・大文字小文字区別ありのみ。
  正規表現等の高度なマッチングには未対応。
- 設定ダイアログの値は`config.ini`に保存/読み込みされるが、実際の
  ランチャー動作への反映(検索方式・候補アイコン・文字色/背景色/フォント・
  フォルダ展開キー等の実装)はまだ無い(詳細は「8. 設定ダイアログの値の
  保存/読み込み・config.ini対応」を参照)。
- `CSettingDlg::GetConfigFilePath()`は`CgleipnirDlg::GetDataFolderPath()`と
  同じロジックをクラスをまたいで複製している(重複コード、将来的に
  共通ユーティリティへの切り出しを検討)。

## 次のセッションへの引き継ぎ事項
1. GitHub上の`resource.h`・`gleipnir.rc`・`CSettingDlg.h/.cpp`・
   `CSettingCommonDlg.h/.cpp`・`CSettingOtherDlg.h/.cpp`・`gleipnirDlg.cpp`を
   最新版に置き換えてビルド。`/option`で設定ダイアログを開き、各項目に
   値を入れてOKで閉じ、`data\config.ini`に保存されるか、再度開いたときに
   復元されるか確認。
   - `IDD_SETTING_DIALOG_COMMON`(309×309)が親ダイアログのタブ表示領域より
     大きく、はみ出す可能性がある点に注意(「7.」の「レイアウト上の注意」参照)。
2. 「呼び出しキー」欄が無効化され「Caps Lock」と表示された状態で
   問題ないか確認。
3. `IDD_SETTING_DIALOG_OTHER`のOK/キャンセルボタンを`.rc`から削除した点、
   ビルド・表示に問題が無いか確認。
4. 保存された`config.ini`の値を実際のランチャー動作に反映する実装
   (検索方式の切り替え、候補アイコン表示、文字色/背景色/フォント適用、
   フォルダ展開キーの実キー割り当て等)に着手するかどうか、
   優先順位を相談したい。
5. ビルド・実行し、スキャン中にタイトルバーの件数表示が更新されるか、
   `/scan stop`または`/scan cancel`(あるいはトレイメニューの
   「スキャンをキャンセル」)で実際に中断され、`index.txt`が
   上書きされないことを確認(未確認であれば)。
6. `data\instant.ini`を作成し、`n/`や`g/`などのキーワードで実際に
   コマンドが起動するか確認(未確認であれば)。

## 未着手 (TODO)
- `config.ini`に保存された各設定値を実際のランチャー動作へ反映
  (検索方式、候補アイコン、文字色/背景色/フォント、フォルダ展開キー等)
- `CSettingOtherDlg`への設定項目追加(現状コントロール無し)
- 設定ダイアログを開くトレイメニュー項目の追加(任意、現状は`/option`のみ)
- `IDD_SETTING_DIALOG`本体のサイズ調整(タブページとのサイズ不一致解消)
- `GetConfigFilePath()`のクラス間重複コードの共通化
- スキャン進捗の詳細化(進捗率、残りドライブ数等)
- リストボックスとエディットボックス間のキーボード操作の改善
- ウィンドウ位置・サイズの記憶
- `instant.ini`のキーワードマッチングの拡張(正規表現等)
- `/scan`サブコマンドの拡張(例: `/scan status`で状態確認)

## 変更ファイル履歴
- 2026-08-24: タスクトレイ常駐 + `RegisterHotKey`版のCaps Lockトグルを実装。
- 2026-08-24: `WH_KEYBOARD_LL`低レベルフック方式(vkCode判定)に変更。
- 2026-08-25: `VK_DBE_ALPHANUMERIC`問題を特定し`scanCode`判定に変更。
  動作確認済み。
- 2026-08-26: 診断用ログを削除。ランチャー本体機能(ドライブ全検索・
  インデックス保存・インクリメンタル検索・起動)を実装。
  (`gleipnirDlg.h`/`.cpp`を全面更新、`resource.h`は変更なし)
- 2026-08-26: インデックス保存先を`%LOCALAPPDATA%`から
  `(gleipnir.exeのフォルダ)\data`に変更(ポータブル運用を想定)。
- 2026-08-26: スキャン実行前の確認ダイアログ、スキャン中のマーキー
  プログレスバー表示、`IDC_EDIT1`への「/scan」入力によるスキャン起動
  を追加。
- 2026-08-26: `data\scan.ini`による対象ドライブ指定・拡張子/フォルダ名
  除外設定に対応。トレイメニューのドライブ選択サブメニューを廃止し、
  「スキャン実行」1項目に統合(`/scan`入力と共通処理化)。スキャン完了時に
  検出件数入りの完了ダイアログを表示するよう変更。
- 2026-08-26: `index.txt`肥大化対策のため、拡張子フィルタを
  `[ignore] IGNORE=...`(除外ブラックリスト)から
  `[regist] REGIST=...`(登録ホワイトリスト)方式に変更。
  `REGIST`未指定時はファイルを1件も登録しない仕様とした。
- 2026-08-26: `data\instant.ini`によるカスタムコマンド(本家Fenrir
  互換の`[InstantCommand]`書式、`%A`等のマクロ置換)に対応。
  トレイメニューに「設定」サブメニュー(scan.ini/instant.iniの編集、
  再読み込み、dataフォルダを開く)を追加(設定ダイアログ本体は
  `IDD_SETTINGS`リソース追加待ちの暫定実装)。
- 2026-08-26: インデックス作成中、タイトルバーに件数を随時表示する
  進捗表示を追加。`/scan stop`・`/scan cancel`(またはトレイメニューの
  「スキャンをキャンセル」)によるキャンセル機能を追加し、キャンセル時は
  `index.txt`を上書きしない仕様とした。スキャン中は「終了」を
  ブロックするようにした。
- 2026-08-26: ユーザーがリソースエディタで追加した`IDD_SETTING_DIALOG`
  (タブコントロール)・`IDD_SETTING_DIALOG_COMMON`・
  `IDD_SETTING_DIALOG_OTHER`に対し、`CSettingDlg`/`CSettingCommonDlg`/
  `CSettingOtherDlg`(新規ファイル`SettingDlg.h/.cpp`・
  `SettingCommonDlg.h/.cpp`・`SettingOtherDlg.h/.cpp`)を実装。
  タブ切り替えでのページ表示/非表示、`/option`入力での設定ダイアログ
  起動に対応した。タブページ内の個々のコントロールへの機能実装は未着手。
- 2026-08-26: GitHubリポジトリ(`dwmkFD/gleipnir`、公開)の`resource.h`/
  `gleipnir.rc`を直接読み込み、タブページ内の各コントロールの意味を確認。
  `IDC_EDIT1`(呼び出しキー欄)を`IDC_EDIT_HOTKEY`にリネームし、Caps Lock
  固定のため無効化表示に変更。`IDD_SETTING_DIALOG_OTHER`に残っていた
  不要なOK/キャンセルボタンを`.rc`から削除。`CSettingCommonDlg`に
  各コントロールのDDXバインドと`data\config.ini`への保存/読み込み
  (`LoadFromConfig`/`SaveToConfig`)を実装し、`CSettingDlg`の
  ページ生成時に読み込み・`OnOK`時に保存するようにした。ファイル名は
  実リポジトリに合わせ`C`プレフィックス付き(`CSettingDlg.h/.cpp`等)に
  統一。
