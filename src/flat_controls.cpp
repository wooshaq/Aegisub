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

/// @file flat_controls.cpp
/// @brief WsQ: combo box, number field and slider drawn by Aegisub itself

#include "flat_controls.h"

#include "theme.h"

#include <algorithm>

#include <wx/combobox.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/menu.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>

namespace {
const wxColour hover_fill(54, 59, 66);

wxColour ParentColour(wxWindow *w) {
	wxWindow *parent = w->GetParent();
	return parent && parent->UseBgCol() ? parent->GetBackgroundColour() : theme::Window();
}

wxColour TextColour(wxWindow *w) {
	return w->IsEnabled() ? theme::Text() : theme::TextDim();
}
}

// ---------------------------------------------------------------------------
// FlatField

FlatField::FlatField(wxWindow *parent, wxWindowID id, wxSize const& size)
: wxControl(parent, id, wxDefaultPosition, size, wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Bind(wxEVT_PAINT, &FlatField::OnPaint, this);
	Bind(wxEVT_SIZE, &FlatField::OnSize, this);
}

void FlatField::MakeText(wxString const& value, long style) {
	text = new wxTextCtrl(this, -1, value, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | style);
	text->SetBackgroundColour(theme::Field());
	text->SetForegroundColour(theme::Text());
	// The frame turns blue while typing
	text->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) { Refresh(false); e.Skip(); });
	text->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { Refresh(false); e.Skip(); });
	text->Bind(wxEVT_TEXT_ENTER, &FlatField::Forward, this);
}

void FlatField::Forward(wxCommandEvent &evt) {
	wxCommandEvent copy(evt.GetEventType(), GetId());
	copy.SetEventObject(this);
	copy.SetString(evt.GetString());
	copy.SetInt(evt.GetInt());
	ProcessWindowEvent(copy);
}

wxSize FlatField::DoGetBestSize() const {
	return wxSize(FromDIP(80), GetCharHeight() + FromDIP(12));
}

bool FlatField::Enable(bool enable) {
	if (!wxControl::Enable(enable)) return false;
	if (text) text->Enable(enable);
	Refresh(false);
	return true;
}

bool FlatField::SetFont(wxFont const& font) {
	bool changed = wxControl::SetFont(font);
	if (text) text->SetFont(font);
	return changed;
}

void FlatField::OnSize(wxSizeEvent &evt) {
	if (text) {
		const wxSize size = GetClientSize();
		const int left = FromDIP(8);
		const int th = std::min(size.GetHeight() - 2, text->GetBestSize().GetHeight());
		text->SetSize(left, (size.GetHeight() - th) / 2,
			std::max(1, size.GetWidth() - left - ButtonsWidth() - FromDIP(2)), th);
	}
	Refresh(false);
	evt.Skip();
}

void FlatField::OnPaint(wxPaintEvent&) {
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(ParentColour(this)));
	dc.Clear();

	wxGCDC gdc(dc);
	const wxSize size = GetClientSize();
	const bool focused = text && text->HasFocus();
	gdc.SetBrush(wxBrush(theme::Field()));
	gdc.SetPen(wxPen(focused ? theme::Accent() : theme::FieldBorder(), 1));
	gdc.DrawRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), FromDIP(5));

	gdc.SetFont(GetFont());
	PaintContents(gdc, wxRect(size));
}

// ---------------------------------------------------------------------------
// FlatComboBox

FlatComboBox::FlatComboBox(wxWindow *parent, wxString const& value, wxSize const& size, bool editable, wxString const& hint)
: FlatField(parent, -1, size)
, value(value)
, editable(editable)
{
	if (editable) {
		MakeText(value, wxTE_PROCESS_ENTER);
		if (!hint.empty()) text->SetHint(hint);
		text->Bind(wxEVT_TEXT, &FlatComboBox::Forward, this);
		// Alt+Down or F4 opens the list, like a real combo box
		text->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
			if ((e.GetKeyCode() == WXK_DOWN && e.AltDown()) || e.GetKeyCode() == WXK_F4)
				ShowList();
			else
				e.Skip();
		});
	}
	else
		SetCanFocus(false);

	for (auto evt : {wxEVT_ENTER_WINDOW, wxEVT_LEAVE_WINDOW, wxEVT_MOTION, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK, wxEVT_LEFT_UP})
		Bind(evt, &FlatComboBox::OnMouse, this);
	SetInitialSize(size);
}

int FlatComboBox::ButtonsWidth() const {
	return FromDIP(22);
}

void FlatComboBox::PaintContents(wxDC &dc, wxRect const& field) {
	const int bw = ButtonsWidth();
	const bool hot = hover && IsEnabled() && (!editable || ScreenToClient(wxGetMousePosition()).x >= field.GetRight() - bw);

	if (!editable) {
		dc.SetTextForeground(TextColour(this));
		const int left = FromDIP(8);
		wxString shown = wxControl::Ellipsize(value, dc, wxELLIPSIZE_END, field.GetWidth() - left - bw);
		wxSize ext = dc.GetTextExtent(shown.empty() ? wxString("Ay") : shown);
		dc.DrawText(shown, left, (field.GetHeight() - ext.GetHeight()) / 2);
	}

	if (hot) {
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(wxBrush(hover_fill));
		dc.DrawRoundedRectangle(field.GetRight() - bw + 1, 3, bw - 4, field.GetHeight() - 6, FromDIP(4));
	}

	// Chevron
	const double cx = field.GetRight() - bw / 2.0, cy = field.GetHeight() / 2.0;
	const double s = FromDIP(4);
	dc.SetPen(wxPen(hot ? theme::Text() : theme::TextDim(), std::max(1, FromDIP(1))));
	wxPoint pts[] = {wxPoint(cx - s, cy - s / 2), wxPoint(cx, cy + s / 2), wxPoint(cx + s, cy - s / 2)};
	dc.DrawLines(3, pts);
}

void FlatComboBox::OnMouse(wxMouseEvent &evt) {
	if (evt.Entering()) hover = true;
	else if (evt.Leaving()) hover = false;
	else if ((evt.LeftDown() || evt.LeftDClick()) && IsEnabled()) {
		if (!editable || evt.GetX() >= GetClientSize().GetWidth() - ButtonsWidth()) {
			Refresh(false);
			ShowList();
			return;
		}
		// A click on the padding around the text still starts typing
		text->SetFocus();
	}
	Refresh(false);
	evt.Skip();
}

void FlatComboBox::ShowList() {
	if (items.empty()) return;

	const wxString current = GetValue();
	wxMenu menu;
	for (size_t i = 0; i < items.size(); ++i) {
		wxString label = items[i];
		label.Replace("&", "&&");
		label.Replace("\t", " ");
		auto item = menu.AppendCheckItem(1000 + i, label.empty() ? wxString(" ") : label);
		if (items[i] == current) item->Check(true);
	}

	int id = GetPopupMenuSelectionFromUser(menu, wxPoint(0, GetClientSize().GetHeight()));
	hover = false;
	Refresh(false);
	if (id < 1000 || id >= 1000 + (int)items.size()) return;

	const int index = id - 1000;
	ChangeValue(items[index]);
	if (text) text->SetInsertionPointEnd();

	wxCommandEvent sel(wxEVT_COMBOBOX, GetId());
	sel.SetEventObject(this);
	sel.SetString(items[index]);
	sel.SetInt(index);
	ProcessWindowEvent(sel);
}

wxString FlatComboBox::GetValue() const {
	return editable ? text->GetValue() : value;
}

void FlatComboBox::ChangeValue(wxString const& v) {
	if (editable) {
		if (text->GetValue() != v) text->ChangeValue(v);
	}
	else {
		value = v;
		Refresh(false);
	}
}

void FlatComboBox::Set(wxArrayString const& new_items) {
	items = new_items;
}

int FlatComboBox::FindString(wxString const& s) const {
	for (size_t i = 0; i < items.size(); ++i) {
		if (items[i].IsSameAs(s, false)) return (int)i;
	}
	return wxNOT_FOUND;
}

void FlatComboBox::Select(int n) {
	if (n >= 0 && n < (int)items.size())
		ChangeValue(items[n]);
	else if (!editable)
		ChangeValue(wxString());
}

bool FlatComboBox::SetStringSelection(wxString const& s) {
	int n = FindString(s);
	if (n == wxNOT_FOUND) return false;
	Select(n);
	return true;
}

long FlatComboBox::GetInsertionPoint() const {
	return text ? text->GetInsertionPoint() : 0;
}

void FlatComboBox::SetInsertionPoint(long pos) {
	if (text) text->SetInsertionPoint(std::min(pos, text->GetLastPosition()));
}

// ---------------------------------------------------------------------------
// FlatSpinCtrl

FlatSpinCtrl::FlatSpinCtrl(wxWindow *parent, int value, int min, int max, wxSize const& size)
: FlatField(parent, -1, size)
, min(min)
, max(max)
{
	MakeText(std::to_wstring(std::clamp(value, min, max)), wxTE_PROCESS_ENTER);
	text->SetValidator(wxTextValidator(min < 0 ? wxFILTER_NUMERIC : wxFILTER_DIGITS));
	text->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Notify(); });
	text->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
		if (e.GetKeyCode() == WXK_UP) Step(1);
		else if (e.GetKeyCode() == WXK_DOWN) Step(-1);
		else if (e.GetKeyCode() == WXK_PAGEUP) Step(10);
		else if (e.GetKeyCode() == WXK_PAGEDOWN) Step(-10);
		else e.Skip();
	});
	text->Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) { Step(e.GetWheelRotation() > 0 ? 1 : -1); });

	for (auto evt : {wxEVT_ENTER_WINDOW, wxEVT_LEAVE_WINDOW, wxEVT_MOTION, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK, wxEVT_MOUSEWHEEL})
		Bind(evt, &FlatSpinCtrl::OnMouse, this);

	if (size == wxDefaultSize)
		SetInitialSize(wxSize(GetTextExtent(wxS("0000")).GetWidth() + FromDIP(16) + ButtonsWidth(), -1));
	else
		SetInitialSize(size);
}

int FlatSpinCtrl::ButtonsWidth() const {
	return 2 * FromDIP(18);
}

void FlatSpinCtrl::PaintContents(wxDC &dc, wxRect const& field) {
	const int bw = FromDIP(18);
	const int h = field.GetHeight();
	for (int dir : {-1, 1}) {
		const int x0 = dir < 0 ? field.GetRight() - 2 * bw : field.GetRight() - bw;
		const bool is_hot = hover && hot == dir && IsEnabled();
		if (is_hot) {
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(wxBrush(hover_fill));
			dc.DrawRoundedRectangle(x0 + 1, 3, bw - 3, h - 6, FromDIP(4));
		}
		const int cx = x0 + bw / 2, cy = h / 2, s = FromDIP(4);
		dc.SetPen(wxPen(is_hot ? theme::Text() : theme::TextDim(), std::max(1, FromDIP(1))));
		dc.DrawLine(cx - s, cy, cx + s + 1, cy);
		if (dir > 0)
			dc.DrawLine(cx, cy - s, cx, cy + s + 1);
	}
}

void FlatSpinCtrl::OnMouse(wxMouseEvent &evt) {
	const int w = GetClientSize().GetWidth(), bw = FromDIP(18);
	if (evt.Leaving()) {
		hover = false;
		hot = 0;
	}
	else {
		hover = true;
		const int x = evt.GetX();
		hot = x >= w - bw ? 1 : x >= w - 2 * bw ? -1 : 0;
	}

	if (evt.GetEventType() == wxEVT_MOUSEWHEEL && IsEnabled())
		Step(evt.GetWheelRotation() > 0 ? 1 : -1);
	else if ((evt.LeftDown() || evt.LeftDClick()) && IsEnabled()) {
		if (hot) Step(hot);
		else text->SetFocus();
	}
	Refresh(false);
}

int FlatSpinCtrl::GetValue() const {
	long v = 0;
	if (!text->GetValue().ToLong(&v)) v = std::max(min, 0);
	return std::clamp<int>(v, min, max);
}

void FlatSpinCtrl::SetValue(int v) {
	wxString s = std::to_wstring(std::clamp(v, min, max));
	if (text->GetValue() != s) text->ChangeValue(s);
}

void FlatSpinCtrl::Step(int delta) {
	const int before = GetValue();
	SetValue(before + delta);
	if (GetValue() != before) Notify();
}

void FlatSpinCtrl::Notify() {
	wxCommandEvent evt(wxEVT_TEXT, GetId());
	evt.SetEventObject(this);
	evt.SetString(text->GetValue());
	evt.SetInt(GetValue());
	ProcessWindowEvent(evt);
}

// ---------------------------------------------------------------------------
// FlatSlider

FlatSlider::FlatSlider(wxWindow *parent, wxWindowID id, int value, int min, int max, bool inverse)
: wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE)
, value(std::clamp(value, min, max))
, min(min)
, max(std::max(max, min + 1))
, inverse(inverse)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetInitialSize(GetBestSize());
	Bind(wxEVT_PAINT, &FlatSlider::OnPaint, this);
	for (auto evt : {wxEVT_ENTER_WINDOW, wxEVT_LEAVE_WINDOW, wxEVT_MOTION, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK, wxEVT_LEFT_UP, wxEVT_MOUSEWHEEL})
		Bind(evt, &FlatSlider::OnMouse, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) { dragging = false; Refresh(false); });
	Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(false); e.Skip(); });
}

wxSize FlatSlider::DoGetBestSize() const {
	return FromDIP(wxSize(22, 60));
}

int FlatSlider::Margin() const {
	return FromDIP(9);
}

void FlatSlider::SetValue(int v) {
	v = std::clamp(v, min, max);
	if (v == value) return;
	value = v;
	Refresh(false);
}

bool FlatSlider::Enable(bool enable) {
	if (!wxControl::Enable(enable)) return false;
	Refresh(false);
	return true;
}

void FlatSlider::OnPaint(wxPaintEvent&) {
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(ParentColour(this)));
	dc.Clear();

	wxGCDC gdc(dc);
	const wxSize size = GetClientSize();
	const int m = Margin();
	const int len = std::max(1, size.GetHeight() - 2 * m);
	const double frac = double(value - min) / (max - min);
	const int knob_y = inverse ? size.GetHeight() - m - int(frac * len) : m + int(frac * len);
	const int track_w = FromDIP(4);
	const int cx = size.GetWidth() / 2;

	// Track, filled from the minimum end up to the knob
	gdc.SetPen(*wxTRANSPARENT_PEN);
	gdc.SetBrush(wxBrush(theme::Field()));
	gdc.DrawRoundedRectangle(cx - track_w / 2, m, track_w, len, track_w / 2.0);
	gdc.SetBrush(wxBrush(IsEnabled() ? theme::Accent() : theme::FieldBorder()));
	if (inverse)
		gdc.DrawRoundedRectangle(cx - track_w / 2, knob_y, track_w, size.GetHeight() - m - knob_y, track_w / 2.0);
	else
		gdc.DrawRoundedRectangle(cx - track_w / 2, m, track_w, knob_y - m, track_w / 2.0);

	// Knob
	const int r = FromDIP(6);
	wxColour knob = !IsEnabled() ? wxColour(90, 97, 107) : (hover || dragging) ? *wxWHITE : theme::Text();
	gdc.SetBrush(wxBrush(knob));
	gdc.SetPen(wxPen(ParentColour(this), std::max(1, FromDIP(2))));
	gdc.DrawCircle(cx, knob_y, r);
}

void FlatSlider::SetFromY(int y, wxEventType type) {
	const int m = Margin();
	const int len = std::max(1, GetClientSize().GetHeight() - 2 * m);
	double frac = std::clamp(double(y - m) / len, 0.0, 1.0);
	if (inverse) frac = 1.0 - frac;
	const int v = min + int(frac * (max - min) + 0.5);
	if (v != value || type != wxEVT_SCROLL_THUMBTRACK) {
		SetValue(v);
		Send(type);
	}
}

void FlatSlider::Send(wxEventType type) {
	wxScrollEvent evt(type, GetId(), value, wxVERTICAL);
	evt.SetEventObject(this);
	ProcessWindowEvent(evt);
}

void FlatSlider::OnMouse(wxMouseEvent &evt) {
	if (evt.Entering()) hover = true;
	else if (evt.Leaving()) hover = false;

	if (!IsEnabled()) {
		Refresh(false);
		return;
	}

	if (evt.GetEventType() == wxEVT_MOUSEWHEEL) {
		// Wheel up moves the knob up, like the native slider
		int delta = evt.GetWheelRotation() > 0 ? 1 : -1;
		const int before = value;
		SetValue(value + (inverse ? delta : -delta));
		if (value != before) Send(wxEVT_SCROLL_CHANGED);
		return;
	}

	if (evt.LeftDown() || evt.LeftDClick()) {
		dragging = true;
		if (!HasCapture()) CaptureMouse();
		SetFromY(evt.GetY(), wxEVT_SCROLL_THUMBTRACK);
	}
	else if (evt.Dragging() && dragging)
		SetFromY(evt.GetY(), wxEVT_SCROLL_THUMBTRACK);
	else if (evt.LeftUp() && dragging) {
		dragging = false;
		if (HasCapture()) ReleaseMouse();
		Send(wxEVT_SCROLL_THUMBRELEASE);
		Send(wxEVT_SCROLL_CHANGED);
	}
	Refresh(false);
}
