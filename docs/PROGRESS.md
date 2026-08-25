# gleipnir 開発進捗メモ

最終更新: 2026-08-24

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
- トレイアイコンを右クリックすると動的に生成したポップアップメニュー
  （「終了」のみ）を表示。`.rc`ファイルは変更していない。
- トレイアイコンをダブルクリックすると表示/非表示をトグル。
- 終了はトレイメニューの「終了」から `EndDialog(IDCANCEL)` で行う
  （元々 `DoModal()` ベースのアプリのため）。

### 2. Caps Lockキーによる表示/非表示トグル
- **v1(RegisterHotKey版)**: `RegisterHotKey(m_hWnd, ID, 0, VK_CAPITAL)`
  → 動作しないことが判明。
  - 原因: オリジナルのFenrirが低レベルキーボードフック(`WH_KEYBOARD_LL`)で
    Caps Lockを先に消費し、OSのデフォルト動作(ロック状態トグル)を
    上書きしている。`RegisterHotKey`による`WM_HOTKEY`判定は
    低レベルフックより後段の処理のため、Fenrirに消費された入力は
    gleipnirまで到達しない。
- **v2(低レベルフック版・現行)**: `SetWindowsHookEx(WH_KEYBOARD_LL, ...)`
  に変更。
  - Caps Lockのkeydown/keyup検知時に戻り値`1`を返し、OSへの伝播と
    Caps Lockロック状態自体のトグルを抑止。
  - keydown時のみ、メインウィンドウへ独自メッセージ`WM_CAPSLOCK_TOGGLE`
    (`WM_APP+2`)を`PostMessage`し、通常のメッセージハンドラ側で
    `ToggleVisibility()`を実行（フック処理自体は軽量に保つ定石）。
  - フックチェーンはLIFO（後から`SetWindowsHookEx`した方が先に呼ばれる）
    ため、Fenrirより後にgleipnirを起動すればgleipnirが優先される。

## 既知の制約・要検証事項
- **起動順序依存**: gleipnir起動後にFenrirを（再）起動すると、
  再びFenrir側が優先されうる。両立させたい場合の運用方法は未検討
  （例: Fenrirの自動起動を無効化する、gleipnirを最後に起動する運用にする等）。
- Caps Lockキーそのものはロック状態が完全に変化しなくなる
  （意図した挙動かどうか、実機での使用感を含めて要確認）。
- 管理者権限で実行中のプロセス（タスクマネージャー等）が前面にある場合、
  標準権限で動くgleipnirの低レベルフックはUIPI制限により
  入力を拾えないことがある（UAC越えの制約）。
- `resource.h`に `ID_TRAY_EXIT` (=32772) を追加済み。今後リソースエディタで
  新規コントロール/メニューを追加する際はIDの重複に注意。

## 変更ファイル履歴
- 2026-08-24: `gleipnirDlg.h` / `gleipnirDlg.cpp` / `resource.h` に
  タスクトレイ常駐 + `RegisterHotKey`版のCaps Lockトグルを実装。
- 2026-08-24: Caps Lockが機能しない問題を受け、`WH_KEYBOARD_LL`低レベル
  フック方式に変更(`gleipnirDlg.h` / `gleipnirDlg.cpp`のみ更新、
  `resource.h`は変更なし)。

## 未着手 (TODO)
- `IDC_EDIT1` を使ったインクリメンタル検索・ランチャー本体機能
- 起動対象(アプリ/フォルダ/URL等)のレジストリ/INIでの登録・永続化
- 設定ダイアログ（ホットキーのカスタマイズ含む）
- Fenrirとの起動順序問題への対応方針の検討・実装

## 次のセッションへの引き継ぎ事項
1. 現行コード（`WH_KEYBOARD_LL`版）をビルドし、Caps Lockでの
   表示/非表示切り替えが実機で機能するか確認。
2. オリジナルのFenrirを併用しているかどうか、また起動順序による
   挙動の違いを確認。
3. 問題なければ、次はインクリメンタル検索(`IDC_EDIT1`)の実装に着手予定。
4. このファイル（`PROGRESS.md`）はリポジトリ直下または`docs/`に置き、
   セッションをまたぐ際は都度この内容を更新していく想定。
