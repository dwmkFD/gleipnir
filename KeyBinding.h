#pragma once

// キー割り当て(フォルダ展開キー等)の表現と、config.ini 用の文字列との相互変換。
// 文字列の書式: "Ctrl+Shift+Alt+キー名"(修飾キーは任意・順不同、大文字小文字区別なし)。
//   例: "Ctrl+F", "Ctrl+B", "Tab", "BackSpace", "F1", "Shift+Tab", "Ctrl+Alt+Left"
// ヘッダーのみで完結させ、設定画面(CSettingCommonDlg)とメイン画面(CgleipnirDlg)の
// 両方から使う。

struct KeyBinding
{
	UINT vk = 0;        // 仮想キーコード(0 = 未設定)
	bool ctrl = false;
	bool shift = false;
	bool alt = false;

	bool IsValid() const { return vk != 0; }

	bool operator==(const KeyBinding& o) const
	{
		return vk == o.vk && ctrl == o.ctrl && shift == o.shift && alt == o.alt;
	}

	// 押されたキー(vk)と、その時点の修飾キーの状態が一致するか。
	bool Matches(UINT pressedVk) const
	{
		if (vk == 0 || pressedVk != vk)
		{
			return false;
		}
		const bool c = ::GetKeyState(VK_CONTROL) < 0;
		const bool s = ::GetKeyState(VK_SHIFT) < 0;
		const bool a = ::GetKeyState(VK_MENU) < 0;
		return c == ctrl && s == shift && a == alt;
	}

	// 修飾キー無しで押すと文字入力や入力欄の編集に使われるキーか。
	// (この種のキーは割り当てると入力できなくなるため、設定画面で受け付けない)
	static bool IsTypingKey(UINT key)
	{
		return (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')
			|| (key >= VK_NUMPAD0 && key <= VK_DIVIDE) || key == VK_SPACE
			|| (key >= VK_OEM_1 && key <= VK_OEM_3) || (key >= VK_OEM_4 && key <= VK_OEM_8)
			|| key == VK_OEM_102;
	}

	// 入力欄の文字・カーソル操作に使うキー(BackSpace, Delete, ←→, Home, End)を
	// 修飾キー無しで割り当てているか。親フォルダ展開キーでは「入力が空のときだけ」動かす。
	bool IsTextEditingKey() const
	{
		if (ctrl || alt)
		{
			return false;
		}
		return vk == VK_BACK || vk == VK_DELETE || vk == VK_LEFT || vk == VK_RIGHT
			|| vk == VK_HOME || vk == VK_END;
	}
};

namespace KeyBindingText
{
	struct NamedKey
	{
		UINT vk;
		LPCTSTR name;
	};

	// 表示・保存に使う名前(先頭の名前が正式名。別名は読み込み時のみ受け付ける)
	inline const NamedKey* GetNamedKeys(size_t& count)
	{
		static const NamedKey keys[] = {
			{ VK_TAB, _T("Tab") },
			{ VK_BACK, _T("BackSpace") }, { VK_BACK, _T("BS") },
			{ VK_RETURN, _T("Enter") },
			{ VK_ESCAPE, _T("Esc") }, { VK_ESCAPE, _T("Escape") },
			{ VK_SPACE, _T("Space") },
			{ VK_INSERT, _T("Insert") }, { VK_INSERT, _T("Ins") },
			{ VK_DELETE, _T("Delete") }, { VK_DELETE, _T("Del") },
			{ VK_HOME, _T("Home") },
			{ VK_END, _T("End") },
			{ VK_PRIOR, _T("PageUp") }, { VK_PRIOR, _T("PgUp") },
			{ VK_NEXT, _T("PageDown") }, { VK_NEXT, _T("PgDn") },
			{ VK_LEFT, _T("Left") },
			{ VK_RIGHT, _T("Right") },
			{ VK_UP, _T("Up") },
			{ VK_DOWN, _T("Down") },
			{ VK_APPS, _T("Apps") },
			{ VK_PAUSE, _T("Pause") },
		};
		count = _countof(keys);
		return keys;
	}

	inline CString KeyName(UINT vk)
	{
		if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9'))
		{
			return CString(static_cast<TCHAR>(vk));
		}
		if (vk >= VK_F1 && vk <= VK_F24)
		{
			CString s;
			s.Format(_T("F%u"), vk - VK_F1 + 1);
			return s;
		}
		size_t count = 0;
		const NamedKey* keys = GetNamedKeys(count);
		for (size_t i = 0; i < count; ++i)
		{
			if (keys[i].vk == vk)
			{
				return keys[i].name;
			}
		}
		// 記号キー等は仮想キーコードの16進表記で保存する(例: 0xBA)
		CString s;
		s.Format(_T("0x%02X"), vk);
		return s;
	}

	inline UINT ParseKeyName(const CString& name)
	{
		if (name.GetLength() == 1)
		{
			const TCHAR ch = static_cast<TCHAR>(_totupper(name[0]));
			if ((ch >= _T('A') && ch <= _T('Z')) || (ch >= _T('0') && ch <= _T('9')))
			{
				return static_cast<UINT>(ch);
			}
		}
		if ((name[0] == _T('F') || name[0] == _T('f')) && name.GetLength() >= 2
			&& name.Mid(1).SpanIncluding(_T("0123456789")) == name.Mid(1))
		{
			const int n = _ttoi(name.Mid(1));
			if (n >= 1 && n <= 24)
			{
				return VK_F1 + n - 1;
			}
		}
		if (name.Left(2).CompareNoCase(_T("0x")) == 0 && name.GetLength() > 2)
		{
			const unsigned long v = _tcstoul(name.Mid(2), nullptr, 16);
			if (v > 0 && v < 0xFF)
			{
				return static_cast<UINT>(v);
			}
		}
		size_t count = 0;
		const NamedKey* keys = GetNamedKeys(count);
		for (size_t i = 0; i < count; ++i)
		{
			if (name.CompareNoCase(keys[i].name) == 0)
			{
				return keys[i].vk;
			}
		}
		return 0;
	}

	inline CString Format(const KeyBinding& kb)
	{
		if (!kb.IsValid())
		{
			return CString();
		}
		CString s;
		if (kb.ctrl) s += _T("Ctrl+");
		if (kb.shift) s += _T("Shift+");
		if (kb.alt) s += _T("Alt+");
		s += KeyName(kb.vk);
		return s;
	}

	// 解釈できなければ IsValid() == false の KeyBinding を返す。
	inline KeyBinding Parse(const CString& text)
	{
		KeyBinding kb;
		CString s = text;
		s.Trim();
		if (s.IsEmpty())
		{
			return kb;
		}

		// "Ctrl++" のように '+' キー自体は扱わない(記号キーは 0xBB 等で指定)
		int pos = 0;
		CString token = s.Tokenize(_T("+"), pos);
		CString keyPart;
		while (!token.IsEmpty())
		{
			token.Trim();
			if (token.CompareNoCase(_T("Ctrl")) == 0 || token.CompareNoCase(_T("Control")) == 0)
			{
				kb.ctrl = true;
			}
			else if (token.CompareNoCase(_T("Shift")) == 0)
			{
				kb.shift = true;
			}
			else if (token.CompareNoCase(_T("Alt")) == 0)
			{
				kb.alt = true;
			}
			else
			{
				if (!keyPart.IsEmpty())
				{
					return KeyBinding(); // キーが2つ以上ある
				}
				keyPart = token;
			}
			token = s.Tokenize(_T("+"), pos);
		}

		if (keyPart.IsEmpty())
		{
			return KeyBinding(); // 修飾キーだけ
		}
		kb.vk = ParseKeyName(keyPart);
		if (kb.vk == 0)
		{
			return KeyBinding();
		}
		return kb;
	}

	// 既定値付きで解釈する(空欄・不正値なら既定値)。
	inline KeyBinding ParseOr(const CString& text, const KeyBinding& defaultBinding)
	{
		const KeyBinding kb = Parse(text);
		return kb.IsValid() ? kb : defaultBinding;
	}

	inline KeyBinding Make(UINT vk, bool ctrl, bool shift = false, bool alt = false)
	{
		KeyBinding kb;
		kb.vk = vk;
		kb.ctrl = ctrl;
		kb.shift = shift;
		kb.alt = alt;
		return kb;
	}
}

// 既定のキー割り当て
inline KeyBinding DefaultFolderExpandKey() { return KeyBindingText::Make('F', true); }       // Ctrl+F
inline KeyBinding DefaultParentFolderExpandKey() { return KeyBindingText::Make('B', true); } // Ctrl+B
