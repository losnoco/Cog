/*
 * libopenmpt_plugin_gui.cpp
 * -------------------------
 * Purpose: libopenmpt plugin GUI
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif // _MSC_VER

#define _WIN32_WINNT 0x0501 // _WIN32_WINNT_WINXP
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS // Avoid binary bloat from linking unused MFC controls

#include <afxwin.h>
#include <afxcmn.h>

#include "resource.h"

#include "libopenmpt_plugin_gui.hpp"


namespace libopenmpt {
namespace plugin {


class CSettingsApp : public CWinApp {

public:

	BOOL InitInstance() override {
		if ( !CWinApp::InitInstance() )
		{
			return FALSE;
		}
		DllMainAttach();
		return TRUE;
	}

	int ExitInstance() override {
		DllMainDetach();
		return CWinApp::ExitInstance();
	}

};


CSettingsApp theApp;


class CSettingsDialog : public CDialog {

protected:

	DECLARE_MESSAGE_MAP()

	libopenmpt_settings * s;

	CString m_Title;

	CComboBox m_ComboBoxSamplerate;
	CComboBox m_ComboBoxChannels;
	CSliderCtrl m_SliderCtrlGain;
	CComboBox m_ComboBoxInterpolation;
	CButton m_CheckBoxAmigaResampler;
	CComboBox m_ComboBoxRepeat;
	CSliderCtrl m_SliderCtrlStereoSeparation;
	CComboBox m_ComboBoxRamping;

public:

	CSettingsDialog( libopenmpt_settings * s_, CString title, CWnd * parent = NULL )
		: CDialog( IDD_SETTINGS, parent )
		, s( s_ )
		, m_Title( title )
	{
		return;
	}

protected:

	void DoDataExchange( CDataExchange * pDX ) override
	{
		CDialog::DoDataExchange( pDX );
		DDX_Control( pDX, IDC_COMBO_SAMPLERATE, m_ComboBoxSamplerate );
		DDX_Control( pDX, IDC_COMBO_CHANNELS, m_ComboBoxChannels );
		DDX_Control( pDX, IDC_SLIDER_GAIN, m_SliderCtrlGain );
		DDX_Control( pDX, IDC_COMBO_INTERPOLATION, m_ComboBoxInterpolation );
		DDX_Control( pDX, IDC_CHECK_AMIGA_RESAMPLER, m_CheckBoxAmigaResampler );
		DDX_Control( pDX, IDC_COMBO_REPEAT, m_ComboBoxRepeat );
		DDX_Control( pDX, IDC_SLIDER_STEREOSEPARATION, m_SliderCtrlStereoSeparation );
		DDX_Control( pDX, IDC_COMBO_RAMPING, m_ComboBoxRamping );
	}

	afx_msg BOOL OnInitDialog() override {

		if ( !CDialog::OnInitDialog() ) {
			return false;
		}

		SetWindowText( m_Title );
		EnableToolTips();

		bool selected = false;

		selected = false;
		if ( !s->no_default_format ) {
			m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"Default" ), 0 );
		}
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"6000" ), 6000 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"8000" ), 8000 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"11025" ), 11025 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"16000" ), 16000 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"22050" ), 22050 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"32000" ), 32000 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"44100" ), 44100 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"48000" ), 48000 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"88200" ), 88200 );
		m_ComboBoxSamplerate.SetItemData( m_ComboBoxSamplerate.AddString( L"96000" ), 96000 );
		if ( !s->no_default_format && s->samplerate == 0 ) {
			m_ComboBoxSamplerate.SelectString( 0, L"Default" );
		}
		for ( int index = 0; index < m_ComboBoxSamplerate.GetCount(); ++index ) {
			if ( m_ComboBoxSamplerate.GetItemData( index ) == s->samplerate ) {
				m_ComboBoxSamplerate.SetCurSel( index );
				selected = true;
			}
		}
		if ( !selected ) {
			m_ComboBoxSamplerate.SelectString( 0, L"48000" );
		}

		selected = false;
		if ( !s->no_default_format ) {
			m_ComboBoxChannels.SetItemData( m_ComboBoxChannels.AddString( L"Default" ), 0 );
		}
		m_ComboBoxChannels.SetItemData( m_ComboBoxChannels.AddString( L"Mono" ), 1 );
		m_ComboBoxChannels.SetItemData( m_ComboBoxChannels.AddString( L"Stereo" ), 2 );
		m_ComboBoxChannels.SetItemData( m_ComboBoxChannels.AddString( L"Quad" ), 4 );
		if ( !s->no_default_format && s->channels == 0 ) {
			m_ComboBoxChannels.SelectString( 0, L"Default" );
		}
		for ( int index = 0; index < m_ComboBoxChannels.GetCount(); ++index ) {
			if ( m_ComboBoxChannels.GetItemData( index ) == s->channels ) {
				m_ComboBoxChannels.SetCurSel( index );
				selected = true;
			}
		}
		if ( !selected ) {
			m_ComboBoxChannels.SelectString( 0, L"Stereo" );
		}

		m_SliderCtrlGain.SetRange( -1200, 1200 );
		m_SliderCtrlGain.SetTicFreq( 100 );
		m_SliderCtrlGain.SetPageSize( 300 );
		m_SliderCtrlGain.SetLineSize( 100 );
		m_SliderCtrlGain.SetPos( s->mastergain_millibel );

		selected = false;
		m_ComboBoxInterpolation.SetItemData( m_ComboBoxInterpolation.AddString( L"Off / 1 Tap (Nearest)" ), 1 );
		m_ComboBoxInterpolation.SetItemData( m_ComboBoxInterpolation.AddString( L"2 Tap (Linear)" ), 2 );
		m_ComboBoxInterpolation.SetItemData( m_ComboBoxInterpolation.AddString( L"4 Tap (Cubic)" ), 4 );
		m_ComboBoxInterpolation.SetItemData( m_ComboBoxInterpolation.AddString( L"8 Tap (Polyphase FIR)" ), 8 );
		for ( int index = 0; index < m_ComboBoxInterpolation.GetCount(); ++index ) {
			if ( m_ComboBoxInterpolation.GetItemData( index ) == s->interpolationfilterlength ) {
				m_ComboBoxInterpolation.SetCurSel( index );
				selected = true;
			}
		}
		if ( !selected ) {
			m_ComboBoxInterpolation.SelectString( 0, L"8 Tap (Polyphase FIR)" );
		}
		
		m_CheckBoxAmigaResampler.SetCheck( s->use_amiga_resampler ? BST_CHECKED : BST_UNCHECKED );

		selected = false;
		m_ComboBoxRepeat.SetItemData( m_ComboBoxRepeat.AddString( L"Forever" ), -1 );
		m_ComboBoxRepeat.SetItemData( m_ComboBoxRepeat.AddString( L"Never" ), 0 );
		m_ComboBoxRepeat.SetItemData( m_ComboBoxRepeat.AddString( L"Once" ), 1 );
		for ( int index = 0; index < m_ComboBoxRepeat.GetCount(); ++index ) {
			if ( m_ComboBoxRepeat.GetItemData( index ) == s->repeatcount ) {
				m_ComboBoxRepeat.SetCurSel( index );
				selected = true;
			}
		}
		if ( !selected ) {
			m_ComboBoxRepeat.SelectString( 0, L"Never" );
		}

		m_SliderCtrlStereoSeparation.SetRange( 0, 200 );
		m_SliderCtrlStereoSeparation.SetTicFreq( 100 );
		m_SliderCtrlStereoSeparation.SetPageSize( 25 );
		m_SliderCtrlStereoSeparation.SetLineSize( 5 );
		m_SliderCtrlStereoSeparation.SetPos( s->stereoseparation );

		selected = false;
		m_ComboBoxRamping.SetItemData( m_ComboBoxRamping.AddString( L"Default" ), -1 );
		m_ComboBoxRamping.SetItemData( m_ComboBoxRamping.AddString( L"Off" ), 0 );
		m_ComboBoxRamping.SetItemData( m_ComboBoxRamping.AddString( L"1 ms" ), 1 );
		m_ComboBoxRamping.SetItemData( m_ComboBoxRamping.AddString( L"2 ms" ), 2 );
		m_ComboBoxRamping.SetItemData( m_ComboBoxRamping.AddString( L"3 ms" ), 3 );
		m_ComboBoxRamping.SetItemData( m_ComboBoxRamping.AddString( L"5 ms" ), 5 );
		m_ComboBoxRamping.SetItemData( m_ComboBoxRamping.AddString( L"10 ms" ), 10 );
		for ( int index = 0; index < m_ComboBoxRamping.GetCount(); ++index ) {
			if ( m_ComboBoxRamping.GetItemData( index ) == s->ramping ) {
				m_ComboBoxRamping.SetCurSel( index );
				selected = true;
			}
		}
		if ( !selected ) {
			m_ComboBoxRamping.SelectString( 0, L"Default" );
		}

		return TRUE;

	}

	void OnOK() override {

		s->samplerate = m_ComboBoxSamplerate.GetItemData( m_ComboBoxSamplerate.GetCurSel() );

		s->channels = m_ComboBoxChannels.GetItemData( m_ComboBoxChannels.GetCurSel() );

		s->mastergain_millibel = m_SliderCtrlGain.GetPos();

		s->interpolationfilterlength = m_ComboBoxInterpolation.GetItemData( m_ComboBoxInterpolation.GetCurSel() );

		s->use_amiga_resampler = ( m_CheckBoxAmigaResampler.GetCheck() != BST_UNCHECKED ) ? 1 : 0;

		s->repeatcount = m_ComboBoxRepeat.GetItemData( m_ComboBoxRepeat.GetCurSel() );

		s->stereoseparation = m_SliderCtrlStereoSeparation.GetPos();

		s->ramping = m_ComboBoxRamping.GetItemData( m_ComboBoxRamping.GetCurSel() );

		s->changed();

		CDialog::OnOK();

	}

	BOOL OnToolTipText( UINT, NMHDR * pNMHDR, LRESULT * pResult ) {
		TOOLTIPTEXT * pTTT = reinterpret_cast<TOOLTIPTEXT *>( pNMHDR );

		UINT_PTR nID = pNMHDR->idFrom;
		if( pTTT->uFlags & TTF_IDISHWND )
		{
			// idFrom is actually the HWND of the tool
			nID = (UINT_PTR)::GetDlgCtrlID((HWND)nID);
		}

		switch ( nID ) {
		case IDC_SLIDER_GAIN:
			swprintf( pTTT->szText, _countof(pTTT->szText), L"%.02f dB", m_SliderCtrlGain.GetPos() * 0.01f );
			break;

		case IDC_SLIDER_STEREOSEPARATION:
			swprintf( pTTT->szText, _countof(pTTT->szText), L"%d %%", m_SliderCtrlStereoSeparation.GetPos());
			break;

		default:
			return FALSE;
		}

		*pResult = 0;
		return TRUE;
	}

};

BEGIN_MESSAGE_MAP(CSettingsDialog, CDialog)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, &CSettingsDialog::OnToolTipText)
END_MESSAGE_MAP()



class CInfoDialog : public CDialog {

protected:

	CString m_Title;
	CString m_FileInfo;
	CEdit m_EditFileInfo;

public:

	CInfoDialog( CString title, CString info, CWnd * parent = NULL )
		: CDialog( IDD_FILEINFO, parent )
		, m_Title( title )
		, m_FileInfo( info )
	{
		return;
	}

protected:

	void DoDataExchange( CDataExchange * pDX ) override
	{
		CDialog::DoDataExchange( pDX );
		DDX_Control( pDX, IDC_FILEINFO, m_EditFileInfo );
	}

	afx_msg BOOL OnInitDialog() override {

		if ( !CDialog::OnInitDialog() ) {
			return false;
		}

		SetWindowText( m_Title );

		m_EditFileInfo.SetWindowText( m_FileInfo );

		return TRUE;

	}

};


void gui_edit_settings( libopenmpt_settings * s, HWND parent, std::wstring title ) {
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	CSettingsDialog dlg( s, title.c_str(), parent ? CWnd::FromHandle( parent ) : nullptr );
	dlg.DoModal();
}


void gui_show_file_info( HWND parent, std::wstring title, std::wstring info ) {
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	CInfoDialog dlg( title.c_str(), info.c_str(), parent ? CWnd::FromHandle( parent ) : nullptr);
	dlg.DoModal();
}


} // namespace plugin
} // namespace libopenmpt

