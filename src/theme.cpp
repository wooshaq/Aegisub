// Copyright (c) 2026, wooshaq
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

/// @file theme.cpp
/// @brief WsQ dark theme: colour palette and native dark mode

#include "theme.h"

#include "options.h"
#include "libresrc/libresrc.h"

#include <libaegisub/color.h>

#include <algorithm>

#include <wx/app.h>
#include <wx/font.h>
#include <wx/settings.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/version.h>

namespace {
struct ThemeColour {
	const char *option;
	const char *light; ///< Aegisub's default
	const char *dark;  ///< WsQ Dark
};

const ThemeColour palette[] = {
	// Subtitle grid
	{"Colour/Subtitle Grid/Active Border",                 "rgb(255, 91, 239)",  "rgb(76, 141, 255)"},
	{"Colour/Subtitle Grid/Background/Background",         "rgb(255,255,255)",   "rgb(27, 29, 33)"},
	{"Colour/Subtitle Grid/Background/Comment",            "rgb(216, 222, 245)", "rgb(38, 35, 49)"},
	{"Colour/Subtitle Grid/Background/Inframe",            "rgb(255, 253, 234)", "rgb(34, 39, 46)"},
	{"Colour/Subtitle Grid/Background/Selected Comment",   "rgb(211, 238, 238)", "rgb(43, 42, 69)"},
	{"Colour/Subtitle Grid/Background/Selection",          "rgb(206, 255, 231)", "rgb(29, 53, 87)"},
	{"Colour/Subtitle Grid/Background/Ad",                 "rgb(255, 186, 186)", "rgb(61, 31, 36)"},
	{"Colour/Subtitle Grid/Background/Selected Ad",        "rgb(229, 115, 115)", "rgb(92, 38, 48)"},
	{"Colour/Subtitle Grid/Collision",                     "rgb(255,0,0)",       "rgb(245, 184, 75)"},
	{"Colour/Subtitle Grid/CPS Error",                     "rgb(255,0,0)",       "rgb(74, 34, 25)"},
	{"Colour/Subtitle Grid/Header",                        "rgb(165, 207, 231)", "rgb(35, 38, 43)"},
	{"Colour/Subtitle Grid/Left Column",                   "rgb(196, 236, 201)", "rgb(27, 29, 33)"},
	{"Colour/Subtitle Grid/Lines",                         "rgb(190,190,190)",   "rgb(38, 42, 48)"},
	{"Colour/Subtitle Grid/Selection",                     "rgb(0,0,0)",         "rgb(255, 255, 255)"},
	{"Colour/Subtitle Grid/Standard",                      "rgb(0,0,0)",         "rgb(230, 232, 235)"},

	// Edit box text
	{"Colour/Subtitle/Background",                         "rgb(255, 255, 255)", "rgb(22, 24, 28)"},
	{"Colour/Subtitle/Syntax/Background/Error",            "rgb(255, 200, 200)", "rgb(74, 34, 25)"},
	{"Colour/Subtitle/Syntax/Brackets",                    "rgb(20, 50, 255)",   "rgb(138, 180, 255)"},
	{"Colour/Subtitle/Syntax/Comment",                     "rgb(0,0,0)",         "rgb(155, 163, 174)"},
	{"Colour/Subtitle/Syntax/Drawing Command",             "rgb(0,0,0)",         "rgb(230, 232, 235)"},
	{"Colour/Subtitle/Syntax/Drawing X",                   "rgb(90,40,40)",      "rgb(224, 168, 168)"},
	{"Colour/Subtitle/Syntax/Drawing Y",                   "rgb(40,90,40)",      "rgb(168, 224, 168)"},
	{"Colour/Subtitle/Syntax/Error",                       "rgb(200, 0, 0)",     "rgb(255, 123, 114)"},
	{"Colour/Subtitle/Syntax/Karaoke Template",            "rgb(128, 0, 192)",   "rgb(196, 155, 255)"},
	{"Colour/Subtitle/Syntax/Karaoke Variable",            "rgb(128, 0, 192)",   "rgb(196, 155, 255)"},
	{"Colour/Subtitle/Syntax/Line Break",                  "rgb(160, 160, 160)", "rgb(110, 118, 129)"},
	{"Colour/Subtitle/Syntax/Normal",                      "rgb(0,0,0)",         "rgb(230, 232, 235)"},
	{"Colour/Subtitle/Syntax/Parameters",                  "rgb(40, 90, 40)",    "rgb(127, 209, 168)"},
	{"Colour/Subtitle/Syntax/Slashes",                     "rgb(255, 0, 200)",   "rgb(209, 107, 217)"},
	{"Colour/Subtitle/Syntax/Tags",                        "rgb(90, 90, 90)",    "rgb(155, 163, 174)"},

	// Audio display
	{"Colour/Audio Display/Keyframe",                      "rgb(255,0,255)",     "rgb(209, 107, 217)"},
	{"Colour/Audio Display/Line Boundary Inactive Line",   "rgb(190,190,190)",   "rgb(90, 97, 107)"},
	{"Colour/Audio Display/Line boundary End",             "rgb(0, 0, 216)",     "rgb(76, 141, 255)"},
	{"Colour/Audio Display/Line boundary Start",           "rgb(216, 0, 0)",     "rgb(255, 90, 95)"},
	{"Colour/Audio Display/Line Info",                     "rgb(176,184,194)",   "rgb(201, 206, 214)"},
	{"Colour/Audio Display/Play Cursor",                   "rgb(255,255,255)",   "rgb(61, 220, 151)"},
	{"Colour/Audio Display/Seconds Line",                  "rgb(0,100,255)",     "rgb(42, 46, 52)"},
};

agi::signal::Connection theme_connection;

// The "WsQ Blue" waveform scheme: near-neutral dark background (lighter
// inside lines, blue inside the active line), blue waveform. Set here too
// because configs saved by older builds keep the old values.
struct SchemeValue { const char *option; double value; };
const SchemeValue wsq_blue[] = {
	{"Normal/Saturation Offset", 30},     {"Normal/Saturation Scale", 140},
	{"Normal/Lightness Offset", 18},      {"Normal/Lightness Scale", 122},
	{"Inactive/Saturation Offset", 30},   {"Inactive/Saturation Scale", 90},
	{"Inactive/Lightness Offset", 26},    {"Inactive/Lightness Scale", 90},
	{"Selection/Saturation Offset", 115}, {"Selection/Saturation Scale", 90},
	{"Selection/Lightness Offset", 44},   {"Selection/Lightness Scale", 122},
};
const ThemeColour wsq_blue_ui[] = {
	{"Colour/Schemes/WsQ Blue/UI/Light",                   "", "rgb(122, 130, 141)"},
	{"Colour/Schemes/WsQ Blue/UI/Dark",                    "", "rgb(24, 26, 30)"},
	{"Colour/Schemes/WsQ Blue/UI/Selection",               "", "rgb(29, 53, 87)"},
	{"Colour/Schemes/WsQ Blue/UI Focused/Light",           "", "rgb(155, 163, 174)"},
	{"Colour/Schemes/WsQ Blue/UI Focused/Dark",            "", "rgb(24, 26, 30)"},
	{"Colour/Schemes/WsQ Blue/UI Focused/Selection",       "", "rgb(29, 53, 87)"},
};

void ApplyPalette(bool dark) {
	for (auto const& colour : palette)
		OPT_SET(colour.option)->SetColor(agi::Color(dark ? colour.dark : colour.light));
	if (dark) {
		for (auto const& v : wsq_blue)
			OPT_SET(std::string("Colour/Schemes/WsQ Blue/") + v.option)->SetDouble(v.value);
		for (auto const& colour : wsq_blue_ui)
			OPT_SET(colour.option)->SetColor(agi::Color(colour.dark));
	}
	// Older configs keep their own copy of the scheme list
	auto schemes = OPT_GET("Audio/Colour Schemes")->GetListString();
	if (std::find(schemes.begin(), schemes.end(), "WsQ Blue") == schemes.end()) {
		schemes.emplace_back("WsQ Blue");
		OPT_SET("Audio/Colour Schemes")->SetListString(std::move(schemes));
	}
	OPT_SET("Colour/Audio Display/Waveform")->SetString(dark ? "WsQ Blue" : "Green");
	OPT_SET("App/Theme Applied")->SetString(dark ? "dark4" : "light");
}

void OnThemeChanged(agi::OptionValue const& opt) {
	ApplyPalette(opt.GetBool());
	wxMessageBox(_("The colours have been changed. Restart Aegisub to also switch the menus, buttons and dialogs."),
		_("Theme"), wxOK | wxICON_INFORMATION | wxCENTER);
}
}

namespace theme {

void Init() {
	const bool dark = OPT_GET("App/Dark Theme")->GetBool();
	// "dark4": bump when the dark palette changes so existing users get it
	if (OPT_GET("App/Theme Applied")->GetString() != (dark ? "dark4" : "light"))
		ApplyPalette(dark);

	// White icons for dark backgrounds; like the native controls this is
	// decided once, before any icon is loaded
	libresrc_set_dark_icons(dark);

	// Native controls follow the theme too. This must happen before any
	// window is created, so changing it takes a restart.
#if defined(__WXMSW__) && wxCHECK_VERSION(3, 3, 0)
	if (dark)
		wxTheApp->MSWEnableDarkMode(wxApp::DarkMode_Always);
#elif wxCHECK_VERSION(3, 3, 0)
	if (dark)
		wxTheApp->SetAppearance(wxApp::Appearance::Dark);
#endif

	// A lambda rather than the function itself: passing the function pointer
	// crashes MSVC (internal compiler error in the signal adder template)
	theme_connection = OPT_SUB("App/Dark Theme", [](agi::OptionValue const& opt) { OnThemeChanged(opt); });
}

bool IsDark() {
	return OPT_GET("App/Dark Theme")->GetBool();
}

wxFont MonoFont(wxFont const& base) {
	wxFont font(base);
#ifdef __WXMSW__
	font.SetFaceName("Consolas");
#else
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	font.SetFaceName("Monospace");
#endif
	return font;
}

static wxColour Sys(wxSystemColour c) { return wxSystemSettings::GetColour(c); }

wxColour Window()      { return IsDark() ? wxColour(27, 29, 33)    : Sys(wxSYS_COLOUR_BTNFACE); }
wxColour Bar()         { return IsDark() ? wxColour(31, 34, 38)    : Sys(wxSYS_COLOUR_BTNFACE); }
wxColour Border()      { return IsDark() ? wxColour(46, 50, 56)    : Sys(wxSYS_COLOUR_BTNSHADOW); }
wxColour Field()       { return IsDark() ? wxColour(43, 47, 53)    : Sys(wxSYS_COLOUR_WINDOW); }
wxColour FieldBorder() { return IsDark() ? wxColour(58, 64, 72)    : Sys(wxSYS_COLOUR_BTNSHADOW); }
wxColour Text()        { return IsDark() ? wxColour(230, 232, 235) : Sys(wxSYS_COLOUR_WINDOWTEXT); }
wxColour TextDim()     { return IsDark() ? wxColour(155, 163, 174) : Sys(wxSYS_COLOUR_GRAYTEXT); }
wxColour Accent()      { return wxColour(76, 141, 255); }

}
