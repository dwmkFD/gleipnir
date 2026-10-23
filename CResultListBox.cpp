// CResultListBox.cpp : 検索結果リスト(アイコン付きオーナードロー)の実装
//

#include "pch.h"
#include "framework.h"
#include "gleipnir.h"
#include "gleipnirDlg.h"   // FileEntry
#include "CResultListBox.h"
#include <shellapi.h>
#include <commctrl.h>

namespace
{
	const int kPaddingX = 3;        // 行の左端・アイコンと文字の間の余白(ピクセル)
	const int kPaddingY = 2;        // 行の上下の余白(ピクセル、合計)
	const size_t kMaxIconCache = 4000; // キャッシュがこれを超えたら一旦破棄する
}

CResultListBox::CResultListBox()
	: m_pEntries(nullptr)
	, m_textColor(RGB(0, 255, 0))
	, m_bgColor(RGB(0, 0, 0))
	, m_showIcons(true)
	, m_hSysImageList(nullptr)
{
}

void CResultListBox::SetColors(COLORREF textColor, COLORREF bgColor)
{
	m_textColor = textColor;
	m_bgColor = bgColor;
	if (GetSafeHwnd() != nullptr)
	{
		Invalidate();
	}
}

void CResultListBox::SetShowIcons(bool show)
{
	m_showIcons = show;
	if (GetSafeHwnd() != nullptr)
	{
		UpdateItemHeight();
	}
}

int CResultListBox::CalcItemHeight()
{
	int textHeight = 16;
	if (GetSafeHwnd() != nullptr)
	{
		CClientDC dc(this);
		CFont* pFont = GetFont();
		CFont* pOld = (pFont != nullptr) ? dc.SelectObject(pFont) : nullptr;
		TEXTMETRIC tm = {};
		dc.GetTextMetrics(&tm);
		if (pOld != nullptr)
		{
			dc.SelectObject(pOld);
		}
		textHeight = tm.tmHeight;
	}

	int height = textHeight;
	if (m_showIcons)
	{
		const int iconHeight = ::GetSystemMetrics(SM_CYSMICON);
		if (iconHeight > height)
		{
			height = iconHeight;
		}
	}
	return height + kPaddingY;
}

void CResultListBox::UpdateItemHeight()
{
	// LBS_OWNERDRAWFIXED では WM_SETFONT で行の高さが再計算されないため、
	// フォントやアイコン設定を変えたら明示的に設定し直す(インデックス0で全行に適用)。
	SetItemHeight(0, CalcItemHeight());
	Invalidate();
}

void CResultListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	// 生成直後(フォント未設定)に一度だけ呼ばれる。以後は UpdateItemHeight() で更新する。
	lpMIS->itemHeight = CalcItemHeight();
}

int CResultListBox::GetIconIndex(const CString& path)
{
	CString key = path;
	key.MakeLower();

	auto it = m_iconCache.find(key);
	if (it != m_iconCache.end())
	{
		return it->second;
	}

	if (m_iconCache.size() >= kMaxIconCache)
	{
		m_iconCache.clear();
	}

	// 実ファイルからアイコンを取得する(exe や ico、ショートカットは固有アイコン)。
	SHFILEINFO sfi = {};
	DWORD_PTR result = ::SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	if (result == 0)
	{
		// 削除・移動済み等で取得できない場合は、拡張子から推定したアイコンを使う。
		result = ::SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	}

	int index = -1;
	if (result != 0)
	{
		// 戻り値はシステムイメージリストのハンドル(全アイコン共通、破棄しない)。
		m_hSysImageList = reinterpret_cast<HIMAGELIST>(result);
		index = sfi.iIcon;
	}

	m_iconCache[key] = index;
	return index;
}

void CResultListBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	CRect rect(lpDIS->rcItem);

	// 項目が無い状態でフォーカスだけ描く場合
	if (lpDIS->itemID == static_cast<UINT>(-1))
	{
		if (lpDIS->itemState & ODS_FOCUS)
		{
			pDC->DrawFocusRect(&rect);
		}
		return;
	}

	const bool selected = (lpDIS->itemState & ODS_SELECTED) != 0;
	const COLORREF bg = selected ? ::GetSysColor(COLOR_HIGHLIGHT) : m_bgColor;
	const COLORREF fg = selected ? ::GetSysColor(COLOR_HIGHLIGHTTEXT) : m_textColor;

	pDC->FillSolidRect(&rect, bg);

	int textLeft = rect.left + kPaddingX;

	if (m_showIcons && m_pEntries != nullptr && lpDIS->itemID < m_pEntries->size())
	{
		const int iconW = ::GetSystemMetrics(SM_CXSMICON);
		const int iconH = ::GetSystemMetrics(SM_CYSMICON);
		const int index = GetIconIndex((*m_pEntries)[lpDIS->itemID].fullPath);
		if (index >= 0 && m_hSysImageList != nullptr)
		{
			const int iconY = rect.top + (rect.Height() - iconH) / 2;
			::ImageList_Draw(m_hSysImageList, index, lpDIS->hDC, textLeft, iconY, ILD_TRANSPARENT);
		}
		// アイコンが取れなくても文字の開始位置は揃える
		textLeft += iconW + kPaddingX;
	}

	CString text;
	GetText(static_cast<int>(lpDIS->itemID), text);

	CRect textRect(textLeft, rect.top, rect.right - kPaddingX, rect.bottom);
	const int oldMode = pDC->SetBkMode(TRANSPARENT);
	const COLORREF oldColor = pDC->SetTextColor(fg);
	pDC->DrawText(text, &textRect, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	pDC->SetTextColor(oldColor);
	pDC->SetBkMode(oldMode);

	if (lpDIS->itemState & ODS_FOCUS)
	{
		pDC->DrawFocusRect(&rect);
	}
}
