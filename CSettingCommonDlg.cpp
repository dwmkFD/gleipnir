// CSettingCommonDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "gleipnir.h"
#include "afxdialogex.h"
#include "CSettingCommonDlg.h"
#include "gleipnirDlg.h"   // SearchMethod と config.ini 値の変換

namespace
{
	// コンボの並び(番号)と検索方式の対応
	const SearchMethod kSearchMethodsByIndex[] = {
		SearchMethod::Prefix,     // 0: 先頭部分一致
		SearchMethod::Substring,  // 1: 中間部分一致
		SearchMethod::Skip,       // 2: スキップマッチング
	};

	int SearchMethodToIndex(SearchMethod method)
	{
		for (int i = 0; i < static_cast<int>(_countof(kSearchMethodsByIndex)); ++i)
		{
			if (kSearchMethodsByIndex[i] == method)
			{
				return i;
			}
		}
		return 1;
	}

	SearchMethod IndexToSearchMethod(int index, SearchMethod defaultMethod)
	{
		return (index >= 0 && index < static_cast<int>(_countof(kSearchMethodsByIndex))) ? kSearchMethodsByIndex[index] : defaultMethod;
	}
}


// CSettingCommonDlg ダイアログ

IMPLEMENT_DYNAMIC(CSettingCommonDlg, CDialogEx)

CSettingCommonDlg::CSettingCommonDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTING_DIALOG_COMMON, pParent)
	, m_maxEnumCount(kDefaultMaxEnumCount)
	, m_autoDeleteBrokenPath(FALSE)
	, m_candidateIconSel(0)
	, m_searchMethodNormalSel(1)
	, m_searchMethodFolderExpandSel(2)
	, m_clearInputOnHide(TRUE)
{
	for (COLORREF& c : m_customColors)
	{
		c = RGB(255, 255, 255);
	}

}

CSettingCommonDlg::~CSettingCommonDlg()
{
}

void CSettingCommonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT2, m_folderExpandKey);
	DDX_Text(pDX, IDC_EDIT3, m_parentFolderExpandKey);
	DDX_Text(pDX, IDC_EDIT4, m_selectDownKey);
	DDX_Text(pDX, IDC_EDIT5, m_selectUpKey);
	DDX_CBIndex(pDX, IDC_COMBO1, m_searchMethodNormalSel);
	DDX_CBIndex(pDX, IDC_COMBO2, m_searchMethodFolderExpandSel);
	DDX_CBIndex(pDX, IDC_COMBO3, m_candidateIconSel);
	DDX_Text(pDX, IDC_EDIT6, m_maxEnumCount);
	DDV_MinMaxInt(pDX, m_maxEnumCount, kMinMaxEnumCount, kMaxMaxEnumCount);
	DDX_Check(pDX, IDC_CHECK1, m_autoDeleteBrokenPath);
	DDX_Text(pDX, IDC_EDIT7, m_textColor);
	DDX_Text(pDX, IDC_EDIT8, m_backgroundColor);
	DDX_Text(pDX, IDC_EDIT9, m_fontName);
}


BEGIN_MESSAGE_MAP(CSettingCommonDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_TEXTCOLOR, &CSettingCommonDlg::OnBnClickedTextColor)
	ON_BN_CLICKED(IDC_BUTTON_BGCOLOR, &CSettingCommonDlg::OnBnClickedBackgroundColor)
	ON_BN_CLICKED(IDC_BUTTON_FONT, &CSettingCommonDlg::OnBnClickedFont)
	ON_WM_CTLCOLOR()
	ON_EN_CHANGE(IDC_EDIT7, &CSettingCommonDlg::OnEnChangeVisualEdit)
	ON_EN_CHANGE(IDC_EDIT8, &CSettingCommonDlg::OnEnChangeVisualEdit)
	ON_EN_CHANGE(IDC_EDIT9, &CSettingCommonDlg::OnEnChangeVisualEdit)
END_MESSAGE_MAP()


// CSettingCommonDlg メッセージ ハンドラー

BOOL CSettingCommonDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 呼び出しキーは画面からは変更不可(無効化)とし、config.ini の値を表示するだけにする
	// (保存対象にも含めない。値は LoadFromConfig で入れる。変更は config.ini の直接編集のみ)。
	CWnd* pHotKeyWnd = GetDlgItem(IDC_EDIT_HOTKEY);
	if (pHotKeyWnd != nullptr)
	{
		pHotKeyWnd->EnableWindow(FALSE);
	}

	// 検索方式: 3択(並びは kSearchMethodsByIndex と対応させる)。
	const UINT searchCombos[] = { IDC_COMBO1, IDC_COMBO2 };
	for (UINT id : searchCombos)
	{
		CComboBox* pCombo = static_cast<CComboBox*>(GetDlgItem(id));
		if (pCombo != nullptr)
		{
			pCombo->ResetContent();
			pCombo->AddString(_T("先頭部分一致"));       // 0
			pCombo->AddString(_T("中間部分一致"));       // 1
			pCombo->AddString(_T("スキップマッチング")); // 2
		}
	}

	// 候補リストのアイコン: 表示/非表示の二択(並びは CBIndex と対応させる)。
	CComboBox* pIconCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO3));
	if (pIconCombo != nullptr)
	{
		pIconCombo->ResetContent();
		pIconCombo->AddString(_T("表示"));   // 0
		pIconCombo->AddString(_T("非表示")); // 1
		pIconCombo->SetCurSel(0);
	}

	// 最大列挙数は数字だけ入力できるようにする。
	CWnd* pMaxEnum = GetDlgItem(IDC_EDIT6);
	if (pMaxEnum != nullptr)
	{
		pMaxEnum->ModifyStyle(0, ES_NUMBER);
	}

	// フォルダ展開キー・親フォルダ展開キー: 押したキーをそのまま割り当てとして記録する欄。
	m_editFolderExpandKey.SubclassDlgItem(IDC_EDIT2, this);
	m_editParentFolderExpandKey.SubclassDlgItem(IDC_EDIT3, this);
	m_editSelectDownKey.SubclassDlgItem(IDC_EDIT4, this);
	m_editSelectUpKey.SubclassDlgItem(IDC_EDIT5, this);

	// 書式や既定値を、空欄時の薄字ヒント(キューバナー)で示す。
	// 空欄のままなら既定値(Ctrl+F / Ctrl+B、緑文字・黒背景・メイリオ太字)が使われる。
	struct CueBanner { UINT id; LPCWSTR text; };
	const CueBanner cues[] = {
		{ IDC_EDIT2, L"キーを押して設定(空欄=Ctrl+F)" },
		{ IDC_EDIT3, L"キーを押して設定(空欄=Ctrl+B)" },
		{ IDC_EDIT4, L"空欄=Ctrl+N" },
		{ IDC_EDIT5, L"空欄=Ctrl+P" },
		{ IDC_EDIT7, L"#00FF00 または 0,255,0" },
		{ IDC_EDIT8, L"#000000 または 0,0,0" },
		{ IDC_EDIT9, L"Meiryo,10,bold" },
	};
	for (const CueBanner& cue : cues)
	{
		CEdit* pEdit = static_cast<CEdit*>(GetDlgItem(cue.id));
		if (pEdit != nullptr)
		{
			pEdit->SetCueBanner(cue.text, TRUE);
		}
	}

	return TRUE;
}

void CSettingCommonDlg::LoadFromConfig(const CString& configPath)
{
	TCHAR buf[512] = { 0 };

	m_configPath = configPath;
	LoadCustomColors();

	// 呼び出しキー(表示のみ)。メイン画面と同じ解釈で、実際に使われているキーを表示する。
	::GetPrivateProfileString(_T("Common"), _T("HotKey"), _T(""), buf, _countof(buf), configPath);
	{
		const KeyBinding hotKey = KeyBindingText::Parse(buf);
		m_hotKey = KeyBindingText::Format(IsUsableHotKey(hotKey) ? hotKey : DefaultHotKey());
		SetDlgItemText(IDC_EDIT_HOTKEY, m_hotKey);
	}

	// 未設定・解釈できない値なら既定のキーを表示する(実際に使われるキーと一致させる)。
	::GetPrivateProfileString(_T("Common"), _T("FolderExpandKey"), _T(""), buf, _countof(buf), configPath);
	m_folderExpandKey = KeyBindingText::Format(KeyBindingText::ParseOr(buf, DefaultFolderExpandKey()));

	::GetPrivateProfileString(_T("Common"), _T("ParentFolderExpandKey"), _T(""), buf, _countof(buf), configPath);
	m_parentFolderExpandKey = KeyBindingText::Format(KeyBindingText::ParseOr(buf, DefaultParentFolderExpandKey()));

	::GetPrivateProfileString(_T("Common"), _T("SelectDownKey"), _T(""), buf, _countof(buf), configPath);
	m_selectDownKey = KeyBindingText::Format(KeyBindingText::ParseOr(buf, DefaultSelectDownKey()));

	::GetPrivateProfileString(_T("Common"), _T("SelectUpKey"), _T(""), buf, _countof(buf), configPath);
	m_selectUpKey = KeyBindingText::Format(KeyBindingText::ParseOr(buf, DefaultSelectUpKey()));

	::GetPrivateProfileString(_T("Common"), _T("SearchMethodNormal"), _T(""), buf, _countof(buf), configPath);
	m_searchMethodNormalSel = SearchMethodToIndex(CgleipnirDlg::ParseSearchMethod(buf, SearchMethod::Substring));

	::GetPrivateProfileString(_T("Common"), _T("SearchMethodFolderExpand"), _T(""), buf, _countof(buf), configPath);
	m_searchMethodFolderExpandSel = SearchMethodToIndex(CgleipnirDlg::ParseSearchMethod(buf, SearchMethod::Skip));

	// "0"/"非表示" のときだけ非表示。未設定・その他の値は既定の「表示」。
	::GetPrivateProfileString(_T("Common"), _T("CandidateIcon"), _T("1"), buf, _countof(buf), configPath);
	{
		CString v = buf;
		v.Trim();
		m_candidateIconSel = (v == _T("0") || v == _T("非表示")) ? 1 : 0;
	}

	// 未設定・範囲外は既定の200件(メイン画面と同じ解釈)。
	m_maxEnumCount = static_cast<int>(::GetPrivateProfileInt(_T("Common"), _T("MaxEnumCount"), kDefaultMaxEnumCount, configPath));
	if (m_maxEnumCount < kMinMaxEnumCount || m_maxEnumCount > kMaxMaxEnumCount)
	{
		m_maxEnumCount = kDefaultMaxEnumCount;
	}
	m_autoDeleteBrokenPath = ::GetPrivateProfileInt(_T("Common"), _T("AutoDeleteBrokenPath"), 0, configPath) ? TRUE : FALSE;

	::GetPrivateProfileString(_T("Common"), _T("TextColor"), _T(""), buf, _countof(buf), configPath);
	m_textColor = buf;

	::GetPrivateProfileString(_T("Common"), _T("BackgroundColor"), _T(""), buf, _countof(buf), configPath);
	m_backgroundColor = buf;

	::GetPrivateProfileString(_T("Common"), _T("Font"), _T(""), buf, _countof(buf), configPath);
	m_fontName = buf;

	m_clearInputOnHide = ::GetPrivateProfileInt(_T("Common"), _T("ClearInputOnHide"), 1, configPath) ? TRUE : FALSE;

	// 読み込んだ値をコントロールへ反映する。
	UpdateData(FALSE);

	// サンプル表示を読み込んだ値で更新する。
	UpdateSample();
}

void CSettingCommonDlg::SaveToConfig(const CString& configPath)
{
	// コントロールの現在値をメンバー変数へ取り込む。
	UpdateData(TRUE);

	// 表記を正規化して保存する(空欄なら空 = 既定のキー)。
	::WritePrivateProfileString(_T("Common"), _T("FolderExpandKey"),
		KeyBindingText::Format(KeyBindingText::Parse(m_folderExpandKey)), configPath);
	::WritePrivateProfileString(_T("Common"), _T("ParentFolderExpandKey"),
		KeyBindingText::Format(KeyBindingText::Parse(m_parentFolderExpandKey)), configPath);
	::WritePrivateProfileString(_T("Common"), _T("SelectDownKey"),
		KeyBindingText::Format(KeyBindingText::Parse(m_selectDownKey)), configPath);
	::WritePrivateProfileString(_T("Common"), _T("SelectUpKey"),
		KeyBindingText::Format(KeyBindingText::Parse(m_selectUpKey)), configPath);
	::WritePrivateProfileString(_T("Common"), _T("SearchMethodNormal"),
		CgleipnirDlg::SearchMethodToConfigValue(IndexToSearchMethod(m_searchMethodNormalSel, SearchMethod::Substring)), configPath);
	::WritePrivateProfileString(_T("Common"), _T("SearchMethodFolderExpand"),
		CgleipnirDlg::SearchMethodToConfigValue(IndexToSearchMethod(m_searchMethodFolderExpandSel, SearchMethod::Skip)), configPath);
	::WritePrivateProfileString(_T("Common"), _T("CandidateIcon"), (m_candidateIconSel == 1) ? _T("0") : _T("1"), configPath);

	CString numBuf;
	numBuf.Format(_T("%d"), m_maxEnumCount);
	::WritePrivateProfileString(_T("Common"), _T("MaxEnumCount"), numBuf, configPath);

	numBuf.Format(_T("%d"), m_autoDeleteBrokenPath ? 1 : 0);
	::WritePrivateProfileString(_T("Common"), _T("AutoDeleteBrokenPath"), numBuf, configPath);

	::WritePrivateProfileString(_T("Common"), _T("TextColor"), m_textColor, configPath);
	::WritePrivateProfileString(_T("Common"), _T("BackgroundColor"), m_backgroundColor, configPath);
	::WritePrivateProfileString(_T("Common"), _T("Font"), m_fontName, configPath);

	numBuf.Format(_T("%d"), m_clearInputOnHide ? 1 : 0);
	::WritePrivateProfileString(_T("Common"), _T("ClearInputOnHide"), numBuf, configPath);
}

bool CSettingCommonDlg::ValidateBeforeSave()
{
	// 数値欄(最大列挙数 1〜10000)などの DDV チェック。範囲外なら MFC が
	// メッセージを出して該当欄にフォーカスを移すので、保存せずに戻る。
	if (!UpdateData(TRUE))
	{
		return false;
	}

	// 4つのキー割り当て(空欄は既定値として扱う)に重複が無いか確認する。
	struct KeyField
	{
		UINT id;
		LPCTSTR label;
		KeyBinding (*defaultKey)();
		KeyBinding key;
	};
	KeyField fields[] = {
		{ IDC_EDIT2, _T("フォルダ展開"), &DefaultFolderExpandKey, KeyBinding() },
		{ IDC_EDIT3, _T("親フォルダ展開"), &DefaultParentFolderExpandKey, KeyBinding() },
		{ IDC_EDIT4, _T("下選択"), &DefaultSelectDownKey, KeyBinding() },
		{ IDC_EDIT5, _T("上選択"), &DefaultSelectUpKey, KeyBinding() },
	};
	for (KeyField& f : fields)
	{
		CString text;
		GetDlgItemText(f.id, text);
		f.key = KeyBindingText::ParseOr(text, f.defaultKey());
	}

	// 呼び出しキー(config.ini の値。画面では変更不可)と重なっていると、そのキーは
	// 全アプリで横取りされてランチャー内に届かないため、これも確認する。
	const KeyBinding hotKey = KeyBindingText::Parse(m_hotKey);
	for (const KeyField& f : fields)
	{
		if (f.key == hotKey)
		{
			CString msg;
			msg.Format(_T("「%s」のキー(%s)が呼び出しキーと同じです。\n別のキーを設定してください。"),
				f.label, static_cast<LPCTSTR>(KeyBindingText::Format(f.key)));
			MessageBox(msg, _T("gleipnir"), MB_OK | MB_ICONWARNING);
			GetDlgItem(f.id)->SetFocus();
			return false;
		}
	}

	for (size_t i = 0; i < _countof(fields); ++i)
	{
		for (size_t j = i + 1; j < _countof(fields); ++j)
		{
			if (fields[i].key == fields[j].key)
			{
				CString msg;
				msg.Format(_T("「%s」と「%s」のキーが同じ(%s)です。\n別のキーを設定してください。"),
					fields[i].label, fields[j].label,
					static_cast<LPCTSTR>(KeyBindingText::Format(fields[i].key)));
				MessageBox(msg, _T("gleipnir"), MB_OK | MB_ICONWARNING);
				GetDlgItem(fields[j].id)->SetFocus();
				return false;
			}
		}
	}
	return true;
}

namespace
{
	// ビジュアル欄が空欄のときに使われる既定の色(メイン画面の既定と同じ)
	const COLORREF kDefaultTextColor = RGB(0, 255, 0);
	const COLORREF kDefaultBackgroundColor = RGB(0, 0, 0);
}

COLORREF CSettingCommonDlg::GetEditColor(UINT editId, COLORREF defaultColor)
{
	CString text;
	GetDlgItemText(editId, text);
	COLORREF color = defaultColor;
	CgleipnirDlg::ParseColorString(text, color); // 不正なら defaultColor のまま
	return color;
}

void CSettingCommonDlg::LoadCustomColors()
{
	TCHAR buf[512] = { 0 };
	::GetPrivateProfileString(_T("Common"), _T("CustomColors"), _T(""), buf, _countof(buf), m_configPath);

	const CString value = buf;
	int pos = 0;
	int index = 0;
	CString token = value.Tokenize(_T(", "), pos);
	while (!token.IsEmpty() && index < static_cast<int>(_countof(m_customColors)))
	{
		COLORREF c = RGB(255, 255, 255);
		CgleipnirDlg::ParseColorString(token, c);
		m_customColors[index++] = c;
		token = value.Tokenize(_T(", "), pos);
	}
}

void CSettingCommonDlg::SaveCustomColors() const
{
	if (m_configPath.IsEmpty())
	{
		return;
	}
	CString value;
	for (int i = 0; i < static_cast<int>(_countof(m_customColors)); ++i)
	{
		if (i > 0)
		{
			value += _T(",");
		}
		value += CgleipnirDlg::FormatColorString(m_customColors[i]);
	}
	::WritePrivateProfileString(_T("Common"), _T("CustomColors"), value, m_configPath);
}

bool CSettingCommonDlg::ChooseColorFor(UINT editId, COLORREF defaultColor)
{
	CColorDialog dlg(GetEditColor(editId, defaultColor), 0, this);
	dlg.m_cc.Flags |= CC_RGBINIT | CC_ANYCOLOR; // 黒(0)でも初期選択されるよう明示する
	dlg.m_cc.lpCustColors = m_customColors;

	const INT_PTR result = dlg.DoModal();

	// 「作成した色」はキャンセルで閉じても変わっていることがあるので、毎回保存する。
	SaveCustomColors();

	if (result != IDOK)
	{
		return false;
	}
	SetDlgItemText(editId, CgleipnirDlg::FormatColorString(dlg.GetColor()));
	return true;
}

void CSettingCommonDlg::OnBnClickedTextColor()
{
	ChooseColorFor(IDC_EDIT7, kDefaultTextColor);
}

void CSettingCommonDlg::OnBnClickedBackgroundColor()
{
	ChooseColorFor(IDC_EDIT8, kDefaultBackgroundColor);
}

void CSettingCommonDlg::OnBnClickedFont()
{
	// 初期表示: 欄の文字列をメイン画面と同じ規則で解釈したフォント。
	CString fontText;
	GetDlgItemText(IDC_EDIT9, fontText);
	LOGFONT lf = {};
	BuildFontFromText(fontText, lf);

	// 「文字飾り」(取り消し線・下線・色)も表示する。色は文字色の欄と連動させる。
	CFontDialog dlg(&lf, CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS, nullptr, this);
	const COLORREF textColor = GetEditColor(IDC_EDIT7, kDefaultTextColor);
	dlg.m_cf.rgbColors = textColor;

	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	const int pointSize = (dlg.GetSize() + 5) / 10; // GetSize は 1/10 ポイント単位
	const CString newFont = CgleipnirDlg::FormatFontString(dlg.GetFaceName(), pointSize,
		dlg.GetWeight() >= FW_SEMIBOLD, dlg.IsItalic() != FALSE, dlg.IsUnderline() != FALSE,
		dlg.IsStrikeOut() != FALSE);
	SetDlgItemText(IDC_EDIT9, newFont);

	// フォントダイアログで色を変えた場合だけ、文字色の欄にも反映する。
	if (dlg.GetColor() != textColor)
	{
		SetDlgItemText(IDC_EDIT7, CgleipnirDlg::FormatColorString(dlg.GetColor()));
	}
}

void CSettingCommonDlg::BuildFontFromText(const CString& fontText, LOGFONT& lf)
{
	lf = LOGFONT();
	CWnd* pMain = AfxGetMainWnd();
	CFont* pBase = (pMain != nullptr) ? pMain->GetFont() : nullptr;
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

	CClientDC dc(this);
	CgleipnirDlg::ParseFontString(fontText, lf, dc.GetSafeHdc());
}

void CSettingCommonDlg::UpdateSample()
{
	CWnd* pSample = GetDlgItem(IDC_STATIC_SAMPLE);
	if (pSample == nullptr)
	{
		return;
	}

	// 空欄・不正値はメイン画面と同じく既定値(緑文字・黒背景・メイリオ太字)。
	m_sampleTextColor = GetEditColor(IDC_EDIT7, kDefaultTextColor);
	m_sampleBackColor = GetEditColor(IDC_EDIT8, kDefaultBackgroundColor);
	m_sampleBrush.DeleteObject();
	m_sampleBrush.CreateSolidBrush(m_sampleBackColor);

	CString fontText;
	GetDlgItemText(IDC_EDIT9, fontText);
	LOGFONT lf = {};
	BuildFontFromText(fontText, lf);

	// コントロールに新しいフォントを渡してから古いフォントを破棄する。
	CFont newFont;
	newFont.CreateFontIndirect(&lf);
	pSample->SetFont(&newFont, FALSE);
	m_sampleFont.DeleteObject();
	m_sampleFont.Attach(newFont.Detach());

	pSample->Invalidate();
}

HBRUSH CSettingCommonDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd != nullptr && pWnd->GetDlgCtrlID() == IDC_STATIC_SAMPLE
		&& m_sampleBrush.GetSafeHandle() != nullptr)
	{
		pDC->SetTextColor(m_sampleTextColor);
		pDC->SetBkColor(m_sampleBackColor);
		return static_cast<HBRUSH>(m_sampleBrush.GetSafeHandle());
	}
	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CSettingCommonDlg::OnEnChangeVisualEdit()
{
	// 文字色・背景色・フォントのいずれかが変わったらサンプルを更新する
	// (手入力・「選択...」ボタンのどちらでも)。
	UpdateSample();
}
