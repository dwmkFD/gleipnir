#pragma once

#include <vector>
#include <map>

struct FileEntry;

// 検索結果リスト。オーナードロー(LBS_OWNERDRAWFIXED)で、各行の左端に
// ファイル/フォルダのアイコン(システムイメージリストの小アイコン)を描く。
// アイコンの表示/非表示、文字色・背景色はオーナー(CgleipnirDlg)から設定する。
class CResultListBox : public CListBox
{
public:
	CResultListBox();

	// 行 i のアイコン取得に使うエントリ列(行 i と entries[i] が対応)。
	void SetEntries(const std::vector<FileEntry>* pEntries) { m_pEntries = pEntries; }

	void SetColors(COLORREF textColor, COLORREF bgColor);
	void SetShowIcons(bool show);
	bool GetShowIcons() const { return m_showIcons; }

	// 現在のフォントとアイコン表示設定から行の高さを計算し直して適用する。
	// フォント変更後・アイコン設定変更後に呼ぶ。
	void UpdateItemHeight();

protected:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS) override;
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS) override;

private:
	int GetIconIndex(const CString& path);
	int CalcItemHeight();

	const std::vector<FileEntry>* m_pEntries;
	COLORREF m_textColor;
	COLORREF m_bgColor;
	bool m_showIcons;
	HIMAGELIST m_hSysImageList;           // システムイメージリスト(小アイコン)。破棄不要。
	std::map<CString, int> m_iconCache;   // フルパス(小文字) → システムイメージリストの番号
};
