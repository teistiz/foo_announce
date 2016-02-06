#include "stdafx.h"
#include "resource.h"

#define MAX_STR_LENGTH 256

// This identifies our config record. It's supposed to be unique so pls no copypasta.
static const GUID guid_announcerPreferences = {
	0xe067a0db, 0x6d16, 0x4bdb,{ 0x92, 0x4f, 0x9b, 0x55, 0x61, 0x48, 0x2f, 0x7f } };

// Identifiers for config fields.
static const GUID guid_configAddress = {
	0x3044cd35, 0x5b74, 0x4a81, { 0x97, 0x26, 0x0a, 0x45, 0xef, 0x41, 0x78, 0x1a } };
static const GUID guid_configAPIKey = {
	0x6de74304, 0xa809, 0x489c, { 0x89, 0xe4, 0xf2, 0xe0, 0x1e, 0xab, 0xa0, 0x6b } };
static const GUID guid_configEventID = {
	0xb294e8ac, 0x049d, 0x4c48, { 0xa2, 0x6c, 0x21, 0x74, 0x2f, 0x93, 0x19, 0x1a } };

// Define config fields.
cfg_string cfg_address(guid_configAddress, "hostname:port/path/to/api");
cfg_string cfg_eventid(guid_configEventID, "");
cfg_string cfg_apikey(guid_configAPIKey, "");

// This is shown inside foobar.
class AnnouncerPreferences
: public CDialogImpl <AnnouncerPreferences>,
  public preferences_page_instance
{
public:
	AnnouncerPreferences(preferences_page_callback::ptr callback)
		: mCallback(callback) { }
	
	enum { IDD = IDD_MYPREFERENCES };

	// preferences page overrides
	virtual t_uint32 get_state() override;
	virtual void apply() override;
	virtual void reset() override;
	virtual HWND get_wnd() override;

	// WTL message mapping
	BEGIN_MSG_MAP(AnnouncerPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_ADDRESS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_APIKEY , EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_EVENTID, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
protected:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	HWND mWindowHandle;
	const preferences_page_callback::ptr mCallback;
};

BOOL AnnouncerPreferences::OnInitDialog(CWindow window, LPARAM param) {
	mWindowHandle = window.m_hWnd;
	// should we init them to config values?
	SetDlgItemTextA(get_wnd(), IDC_ADDRESS, "");
	SetDlgItemTextA(get_wnd(), IDC_APIKEY, "");
	SetDlgItemTextA(get_wnd(), IDC_EVENTID, "");
	return FALSE;
}

HWND AnnouncerPreferences::get_wnd() {
	return mWindowHandle;
}

void AnnouncerPreferences::apply() {
	HWND hwnd = get_wnd();
	unsigned int len;
	char tmpstr[MAX_STR_LENGTH];
	len = GetDlgItemTextA(hwnd, IDC_ADDRESS, tmpstr, sizeof(tmpstr));
	cfg_address.set_string(tmpstr, len);
	len = GetDlgItemTextA(hwnd, IDC_APIKEY, tmpstr, sizeof(tmpstr));
	cfg_apikey.set_string(tmpstr, len);
	len = GetDlgItemTextA(hwnd, IDC_EVENTID, tmpstr, sizeof(tmpstr));
	cfg_eventid.set_string(tmpstr, len);
	OnChanged();
}

bool AnnouncerPreferences::HasChanged() {
	HWND hwnd = get_wnd();
	bool changed = false;
	char tmpstr[MAX_STR_LENGTH];
	GetDlgItemTextA(hwnd, IDC_ADDRESS, tmpstr, sizeof(tmpstr));
	changed |= (strcmp(cfg_address.toString(), tmpstr) != 0);
	GetDlgItemTextA(hwnd, IDC_APIKEY, tmpstr, sizeof(tmpstr));
	changed |= (strcmp(cfg_apikey.toString(), tmpstr) != 0);
	GetDlgItemTextA(hwnd, IDC_EVENTID, tmpstr, sizeof(tmpstr));
	changed |= (strcmp(cfg_eventid.toString(), tmpstr) != 0);
	return changed;
}

void AnnouncerPreferences::reset() {
	// set dialog items back to defaults
	SetDlgItemTextA(get_wnd(), IDC_ADDRESS, "");
	SetDlgItemTextA(get_wnd(), IDC_APIKEY, "");
	SetDlgItemTextA(get_wnd(), IDC_EVENTID, "");
}

t_uint32 AnnouncerPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) {
		state |= preferences_state::changed;
	}
	return state;
}

void AnnouncerPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}

void AnnouncerPreferences::OnChanged() {
	mCallback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<AnnouncerPreferences> {
public:
	const char* get_name() {
		return "foo_announce";
	}
	GUID get_guid() {
		return guid_announcerPreferences;
	}
	GUID get_parent_guid() {
		return guid_tools;
	}
};


// This is defined somewhere under foobar2000_sdk_helpers, but building
// it and linking it in does not banish the undefined external dep errors.
// Copypasting is easier than reverse engineering awful build systems.
PFC_NORETURN PFC_NOINLINE void WIN32_OP_FAIL() {
	const DWORD code = GetLastError();
	PFC_ASSERT(code != NO_ERROR);
	throw exception_win32(code);
}

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;
