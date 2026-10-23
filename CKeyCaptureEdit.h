#pragma once

#include "KeyBinding.h"

// キー割り当て入力欄。文字を打つのではなく、割り当てたいキー(修飾キーとの同時押し可)を
// 実際に押すと "Ctrl+F" のような表記が入る。
//   - Delete(修飾キー無し): 欄を空にする(= 既定のキーに戻す)
//   - Enter / Esc: 設定ダイアログの OK / キャンセルとして通常どおり働く
//   - 修飾キー無しの文字キー(A〜Z, 0〜9, 記号, Space 等)は、検索欄に文字を
//     入力できなくなるため受け付けない(Ctrl/Alt との同時押しなら可)
class CKeyCaptureEdit : public CEdit
{
public:
	CKeyCaptureEdit() = default;

protected:
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg LRESULT OnPaste(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	void CaptureKey(UINT nChar);
};
