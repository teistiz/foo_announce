#include "stdafx.h"
#include "resource1.h"
#include <vector>

#define MAX_STR_LENGTH 256

// This identifies our config record. It's supposed to be unique so pls no copypasta.
static const GUID guid_announcerPreferences = {
	0xe067a0db, 0x6d16, 0x4bdb,{ 0x92, 0x4f, 0x9b, 0x55, 0x61, 0x48, 0x2f, 0x7f } };

// Identifiers for config fields.
static const GUID guid_configEnabled = {
	0xe774deda, 0xbdf3, 0x474a, { 0x85, 0xbb, 0xa2, 0x38, 0x2b, 0xe3, 0x81, 0x43 } };
static const GUID guid_configAddress = {
	0x3044cd35, 0x5b74, 0x4a81, { 0x97, 0x26, 0x0a, 0x45, 0xef, 0x41, 0x78, 0x1a } };
static const GUID guid_configAPIKey = {
	0x6de74304, 0xa809, 0x489c, { 0x89, 0xe4, 0xf2, 0xe0, 0x1e, 0xab, 0xa0, 0x6b } };
static const GUID guid_configEventID = {
	0xb294e8ac, 0x049d, 0x4c48, { 0xa2, 0x6c, 0x21, 0x74, 0x2f, 0x93, 0x19, 0x1a } };

// Define config fields.
cfg_bool   cfg_enabled(guid_configEnabled, false);
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
		: m_callback(callback)
	{
	}
	
	enum { IDD = IDD_DIALOG1 };
	// WTL message mapping
	BEGIN_MSG_MAP(AnnouncerPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_SERVERADDRESS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_APIKEY, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_EVENTID, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_ENABLED, EN_CHANGE, OnEditChange)
	END_MSG_MAP()

	// preferences page overrides
	t_uint32 get_state() override;
	void apply() override;
	void reset() override;
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	// this is used to tell Foobar we might have something
	// it will call get_state() in response
	const preferences_page_callback::ptr m_callback;
public:
};

BOOL AnnouncerPreferences::OnInitDialog(CWindow window, LPARAM) {
	SetDlgItemTextA(get_wnd(), IDC_SERVERADDRESS, cfg_address.toString());
	SetDlgItemTextA(get_wnd(), IDC_APIKEY, cfg_apikey.toString());
	SetDlgItemTextA(get_wnd(), IDC_EVENTID, cfg_eventid.toString());
	CheckDlgButton(IDC_ENABLED, cfg_enabled ? BST_CHECKED : BST_UNCHECKED);
	return TRUE;
}

void AnnouncerPreferences::apply() {
	unsigned int len;
	char tmpstr[MAX_STR_LENGTH];
	len = GetDlgItemTextA(get_wnd(), IDC_SERVERADDRESS, tmpstr, sizeof(tmpstr));
	cfg_address.set_string(tmpstr, len);
	len = GetDlgItemTextA(get_wnd(), IDC_APIKEY, tmpstr, sizeof(tmpstr));
	cfg_apikey.set_string(tmpstr, len);
	len = GetDlgItemTextA(get_wnd(), IDC_EVENTID, tmpstr, sizeof(tmpstr));
	cfg_eventid.set_string(tmpstr, len);
	cfg_enabled = IsDlgButtonChecked(IDC_ENABLED) == BST_CHECKED;
	OnChanged();
}

bool AnnouncerPreferences::HasChanged() {
	bool changed = false;
	char tmpstr[MAX_STR_LENGTH];

	GetDlgItemTextA(get_wnd(), IDC_SERVERADDRESS, tmpstr, sizeof(tmpstr));
	changed |= (strcmp(cfg_address.toString(), tmpstr) != 0);
	GetDlgItemTextA(get_wnd(), IDC_APIKEY, tmpstr, sizeof(tmpstr));
	changed |= (strcmp(cfg_apikey.toString(), tmpstr) != 0);
	GetDlgItemTextA(get_wnd(), IDC_EVENTID, tmpstr, sizeof(tmpstr));
	changed |= (strcmp(cfg_eventid.toString(), tmpstr) != 0);
	changed |= (IsDlgButtonChecked(IDC_ENABLED) == BST_CHECKED) != cfg_enabled;
	return changed;
}

void AnnouncerPreferences::reset() {
	// set dialog items back to defaults
	SetDlgItemTextA(get_wnd(), IDC_SERVERADDRESS, "");
	SetDlgItemTextA(get_wnd(), IDC_APIKEY, "");
	SetDlgItemTextA(get_wnd(), IDC_EVENTID, "");
	CheckDlgButton(IDC_ENABLED, false);
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
	// let foobar know stuff changed and the Apply button may need updating
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<AnnouncerPreferences> {
public:
	virtual const char* get_name() override {
		return "foo_announce";
	}
	virtual GUID get_guid() override {
		return guid_announcerPreferences;
	}
	virtual GUID get_parent_guid() override {
		return guid_tools;
	}
};

static preferences_page_factory_t<preferences_page_myimpl>
g_preferences_page_myimpl_factory;
