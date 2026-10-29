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

/// @file flat_button.h
/// @brief WsQ: an icon button drawn by Aegisub itself, so it looks the same on every platform

#pragma once

#include <wx/bmpbndl.h>
#include <wx/control.h>

/// Rounded icon button in the style of the GTK Adwaita theme. Sends
/// wxEVT_BUTTON, or wxEVT_TOGGLEBUTTON when created as a toggle.
class FlatButton final : public wxControl {
	wxBitmapBundle bitmap;
	wxString label;
	bool toggle;
	bool value = false;
	bool hover = false;
	bool pressed = false;

	void OnPaint(wxPaintEvent&);
	void OnMouse(wxMouseEvent&);

	wxSize DoGetBestSize() const override;

public:
	FlatButton(wxWindow *parent, wxBitmapBundle const& bitmap, bool toggle = false);
	/// A text button
	FlatButton(wxWindow *parent, wxString const& label);

	bool AcceptsFocus() const override { return false; }
	bool Enable(bool enable = true) override;

	bool GetValue() const { return value; }
	void SetValue(bool v);

	/// Window background colour used by the themed panels
	static wxColour PanelColour();
};
