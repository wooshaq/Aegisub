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

/// @file flat_controls.h
/// @brief WsQ: combo box, number field and slider drawn by Aegisub itself,
///        so they can look like the rest of the dark layout on Windows too

#pragma once

#include <wx/arrstr.h>
#include <wx/control.h>

class wxTextCtrl;

/// Base for the field-like controls: a rounded field, optionally with a
/// borderless text control inside it
class FlatField : public wxControl {
protected:
	wxTextCtrl *text = nullptr;
	bool hover = false;

	void OnPaint(wxPaintEvent&);
	void OnSize(wxSizeEvent&);

	/// Width of the buttons area on the right
	virtual int ButtonsWidth() const = 0;
	/// Draw the buttons area and, for read-only fields, the value
	virtual void PaintContents(wxDC &dc, wxRect const& field) = 0;
	/// Create the inner text control
	void MakeText(wxString const& value, long style);
	/// Resend an event from the inner text control as this control's own
	void Forward(wxCommandEvent &evt);

	wxSize DoGetBestSize() const override;

public:
	FlatField(wxWindow *parent, wxWindowID id, wxSize const& size);

	bool Enable(bool enable = true) override;
	bool SetFont(wxFont const& font) override;
	bool SetBackgroundColour(wxColour const&) override { return false; }
};

/// Combo box: editable (text field plus a list button) or read-only
/// (the whole field opens the list). Sends wxEVT_TEXT while typing and
/// wxEVT_COMBOBOX when an entry is picked from the list.
class FlatComboBox final : public FlatField {
	wxArrayString items;
	wxString value; ///< Value of a read-only combo box
	bool editable;
	bool pressed = false;

	int ButtonsWidth() const override;
	void PaintContents(wxDC &dc, wxRect const& field) override;
	void OnMouse(wxMouseEvent &evt);
	void ShowList();

public:
	FlatComboBox(wxWindow *parent, wxString const& value, wxSize const& size, bool editable, wxString const& hint = wxString());

	wxString GetValue() const;
	void ChangeValue(wxString const& value);

	void Set(wxArrayString const& items);
	void Clear() { items.clear(); }
	void Append(wxArrayString const& more) { for (auto const& s : more) items.push_back(s); }
	int FindString(wxString const& s) const;
	void Select(int n);
	bool SetStringSelection(wxString const& s);

	long GetInsertionPoint() const;
	void SetInsertionPoint(long pos);
};

/// Number field with − and + buttons. Sends wxEVT_TEXT with the value in
/// GetInt() whenever the value changes because of the user.
class FlatSpinCtrl final : public FlatField {
	int min, max;
	int hot = 0; ///< -1: − button under the mouse, 1: + button, 0: none

	int ButtonsWidth() const override;
	void PaintContents(wxDC &dc, wxRect const& field) override;
	void OnMouse(wxMouseEvent &evt);
	void Step(int delta);
	void Notify();

public:
	FlatSpinCtrl(wxWindow *parent, int value, int min, int max, wxSize const& size = wxDefaultSize);

	int GetValue() const;
	/// Set the value without sending an event
	void SetValue(int value);
};

/// Vertical slider with a thin rounded track and a round knob. Sends
/// wxScrollEvents like wxSlider (catch them with EVT_COMMAND_SCROLL).
class FlatSlider final : public wxControl {
	int value, min, max;
	bool inverse; ///< Minimum at the bottom
	bool hover = false;
	bool dragging = false;

	void OnPaint(wxPaintEvent&);
	void OnMouse(wxMouseEvent &evt);
	void SetFromY(int y, wxEventType type);
	void Send(wxEventType type);
	int Margin() const;

	wxSize DoGetBestSize() const override;

public:
	FlatSlider(wxWindow *parent, wxWindowID id, int value, int min, int max, bool inverse);

	int GetValue() const { return value; }
	void SetValue(int v);
	bool AcceptsFocus() const override { return false; }
	bool Enable(bool enable = true) override;
};
