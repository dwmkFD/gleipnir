// CKeyCaptureEdit.cpp : キー割り当て入力欄の実装
//

#include "pch.h"
#include "framework.h"
#include "CKeyCaptureEdit.h"

BEGIN_MESSAGE_MAP(CKeyCaptureEdit, CEdit)
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
	ON_WM_SYSKEYDOWN()
	ON_WM_CHAR()
	ON_WM_SYSCHAR()
	ON_MESSAGE(WM_PASTE, &CKeyCaptureEdit::OnPaste)
END_MESSAGE_MAP()

UINT CKeyCaptureEdit::OnGetDlgCode()
{
	// Tab や矢印キーもダイアログのフォーカス移動に使わせず、この欄で受け取る。
	// Enter / Esc だけはダイアログ(OK / キャンセル)に任せる。
	UINT code = CEdit::OnGetDlgCode() | DLGC_WANTTAB | DLGC_WANTARROWS;
	const MSG* pCurrent = GetCurrentMessage();
	const MSG* pMsg = (pCurrent != nullptr) ? reinterpret_cast<const MSG*>(pCurrent->lParam) : nullptr;
	// WM_SYSCHAR も含めて受け取ることで、Alt+文字 がダイアログのニーモニック(項目移動)に
	// 化けないようにする(Enter / Esc の文字コードは VK_RETURN / VK_ESCAPE と同値)。
	if (pMsg != nullptr && (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN
		|| pMsg->message == WM_CHAR || pMsg->message == WM_SYSCHAR)
		&& pMsg->wParam != VK_RETURN && pMsg->wParam != VK_ESCAPE)
	{
		code |= DLGC_WANTMESSAGE;
	}
	return code;
}

void CKeyCaptureEdit::CaptureKey(UINT nChar)
{
	// 修飾キー単体の押下は、続けて押される本体のキーを待つ。
	switch (nChar)
	{
	case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
	case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
	case VK_MENU: case VK_LMENU: case VK_RMENU:
	case VK_LWIN: case VK_RWIN:
	case VK_CAPITAL: case VK_NUMLOCK: case VK_SCROLL:
	case VK_PROCESSKEY: // IME 変換中
		return;
	default:
		break;
	}

	KeyBinding kb;
	kb.vk = nChar;
	kb.ctrl = ::GetKeyState(VK_CONTROL) < 0;
	kb.shift = ::GetKeyState(VK_SHIFT) < 0;
	kb.alt = ::GetKeyState(VK_MENU) < 0;

	if (nChar == VK_DELETE && !kb.ctrl && !kb.shift && !kb.alt)
	{
		SetWindowText(_T("")); // 空欄 = 既定のキー
		return;
	}

	// 修飾キー無し(Shift のみも含む)の文字キーは、検索欄に入力できなくなるので不可。
	if (!kb.ctrl && !kb.alt && KeyBinding::IsTypingKey(nChar))
	{
		::MessageBeep(MB_ICONWARNING);
		return;
	}

	SetWindowText(KeyBindingText::Format(kb));
	SetSel(-1, -1); // カーソルを末尾へ(選択表示を消す)
}

void CKeyCaptureEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	UNREFERENCED_PARAMETER(nRepCnt);
	UNREFERENCED_PARAMETER(nFlags);
	CaptureKey(nChar); // 既定の編集動作(文字削除・カーソル移動等)は行わない
}

void CKeyCaptureEdit::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	UNREFERENCED_PARAMETER(nRepCnt);
	UNREFERENCED_PARAMETER(nFlags);
	CaptureKey(nChar); // Alt 同時押し(Alt+F 等)もメニュー操作にせず割り当てとして受け取る
}

void CKeyCaptureEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// 文字の直接入力は受け付けない(表記は CaptureKey が書き込む)。
	UNREFERENCED_PARAMETER(nChar);
	UNREFERENCED_PARAMETER(nRepCnt);
	UNREFERENCED_PARAMETER(nFlags);
}

void CKeyCaptureEdit::OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	UNREFERENCED_PARAMETER(nChar);
	UNREFERENCED_PARAMETER(nRepCnt);
	UNREFERENCED_PARAMETER(nFlags);
}

LRESULT CKeyCaptureEdit::OnPaste(WPARAM wParam, LPARAM lParam)
{
	// 右クリックメニュー等からの貼り付けも無効にする。
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	return 0;
}
