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

/// @file flat_button.cpp
/// @brief WsQ: an icon button drawn by Aegisub itself

#include "flat_button.h"

#include "theme.h"

#include <memory>
#include <unordered_map>

#include <wx/dcbuffer.h>
#include <wx/sizer.h>
#include <wx/graphics.h>
#include <wx/tglbtn.h>

namespace {
// Colours measured from GTK's Adwaita-dark / Adwaita buttons
struct Palette {
	wxColour top, bottom, border, highlight, hover_top, hover_bottom, down;
};

Palette const& palette() {
	static const Palette dark{
		{53, 53, 53}, {50, 50, 50}, {27, 27, 27}, {60, 60, 60},
		{62, 62, 62}, {58, 58, 58}, {33, 33, 33},
	};
	static const Palette light{
		{246, 245, 244}, {237, 235, 233}, {205, 199, 194}, {255, 255, 255},
		{248, 248, 247}, {242, 241, 240}, {214, 209, 205},
	};
	return theme::IsDark() ? dark : light;
}
}

wxColour FlatButton::PanelColour() {
	return theme::IsDark() ? theme::Window() : wxColour(246, 245, 244);
}

namespace {
// Fields framed by their card, and the frame colour of each
std::unordered_map<wxWindow*, wxColour> framed_fields;

void DrawCard(wxWindow *card, wxAutoBufferedPaintDC &dc) {
	wxWindow *parent = card->GetParent();
	dc.SetBackground(wxBrush(parent && parent->UseBgCol() ? parent->GetBackgroundColour() : theme::Window()));
	dc.Clear();

	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
	if (!gc) return;
	const wxSize size = card->GetClientSize();
	gc->SetBrush(wxBrush(card->GetBackgroundColour()));
	gc->SetPen(wxPen(theme::Border(), 1));
	gc->DrawRoundedRectangle(0.5, 0.5, size.GetWidth() - 1, size.GetHeight() - 1, card->FromDIP(6));

	// Rounded frames around the borderless fields
	const int fx = card->FromDIP(4), fy = card->FromDIP(3);
	for (wxWindow *child : card->GetChildren()) {
		auto it = framed_fields.find(child);
		if (it == framed_fields.end() || !child->IsShown()) continue;
		wxRect r = child->GetRect().Inflate(fx, fy);
		gc->SetBrush(wxBrush(child->GetBackgroundColour()));
		gc->SetPen(wxPen(it->second.IsOk() ? it->second : theme::FieldBorder(), 1));
		gc->DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, card->FromDIP(5));
	}
}
}

namespace theme {
void MakeCard(wxWindow *card, int inset) {
	if (!IsDark()) return;
	card->SetBackgroundStyle(wxBG_STYLE_PAINT);
	card->SetBackgroundColour(Bar());
	card->Bind(wxEVT_PAINT, [card](wxPaintEvent&) {
		wxAutoBufferedPaintDC dc(card);
		DrawCard(card, dc);
	});
	card->Bind(wxEVT_SIZE, [card](wxSizeEvent& e) { card->Refresh(false); e.Skip(); });

	if (wxSizer *inner = card->GetSizer()) {
		auto outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(inner, 1, wxEXPAND | wxALL, card->FromDIP(inset));
		card->SetSizer(outer, false);
	}
}

void FrameField(wxWindow *field, wxColour const& border) {
	if (!IsDark()) return;
	framed_fields[field] = border;
	field->Bind(wxEVT_DESTROY, [field](wxWindowDestroyEvent& e) {
		if (e.GetEventObject() == field) framed_fields.erase(field);
		e.Skip();
	});
	// Repaint the frame when the field moves or is shown
	field->Bind(wxEVT_SIZE, [field](wxSizeEvent& e) { if (field->GetParent()) field->GetParent()->Refresh(false); e.Skip(); });
	field->Bind(wxEVT_SHOW, [field](wxShowEvent& e) { if (field->GetParent()) field->GetParent()->Refresh(false); e.Skip(); });

	if (wxSizer *sizer = field->GetContainingSizer()) {
		if (wxSizerItem *item = sizer->GetItem(field)) {
			item->SetFlag((item->GetFlag() & ~wxALL) | wxALL);
			item->SetBorder(std::max(item->GetBorder(), field->FromDIP(5)));
		}
	}
}
}

FlatButton::FlatButton(wxWindow *parent, wxString const& label)
: FlatButton(parent, wxBitmapBundle(), false)
{
	this->label = label;
	SetInitialSize(GetBestSize());
}

FlatButton::FlatButton(wxWindow *parent, wxBitmapBundle const& bitmap, bool toggle)
: wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE)
, bitmap(bitmap)
, toggle(toggle)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetInitialSize(GetBestSize());

	Bind(wxEVT_PAINT, &FlatButton::OnPaint, this);
	for (auto evt : {wxEVT_ENTER_WINDOW, wxEVT_LEAVE_WINDOW, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK, wxEVT_LEFT_UP})
		Bind(evt, &FlatButton::OnMouse, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) { pressed = false; Refresh(false); });
}

wxSize FlatButton::DoGetBestSize() const {
	if (!label.empty()) {
		wxSize text = GetTextExtent(label);
		wxSize size = text + FromDIP(wxSize(20, 10));
		size.IncTo(FromDIP(wxSize(26, 26)));
		return size;
	}
	wxSize icon = bitmap.IsOk() ? bitmap.GetPreferredLogicalSizeFor(this) : wxSize(16, 16);
	wxSize size = icon + FromDIP(wxSize(10, 10));
	size.IncTo(FromDIP(wxSize(26, 26)));
	return size;
}

bool FlatButton::Enable(bool enable) {
	if (!wxControl::Enable(enable)) return false;
	Refresh(false);
	return true;
}

void FlatButton::SetValue(bool v) {
	if (value == v) return;
	value = v;
	Refresh(false);
}

void FlatButton::OnMouse(wxMouseEvent &evt) {
	if (evt.Entering()) hover = true;
	else if (evt.Leaving()) hover = false;
	else if ((evt.LeftDown() || evt.LeftDClick()) && IsEnabled()) {
		pressed = true;
		if (!HasCapture()) CaptureMouse();
	}
	else if (evt.LeftUp()) {
		if (HasCapture()) ReleaseMouse();
		bool inside = GetClientRect().Contains(evt.GetPosition());
		bool fire = pressed && inside && IsEnabled();
		pressed = false;
		hover = inside;
		if (fire) {
			if (toggle) value = !value;
			wxCommandEvent cmd(toggle ? wxEVT_TOGGLEBUTTON : wxEVT_BUTTON, GetId());
			cmd.SetEventObject(this);
			cmd.SetInt(value);
			Refresh(false);
			ProcessWindowEvent(cmd);
			return;
		}
	}
	Refresh(false);
}

void FlatButton::OnPaint(wxPaintEvent&) {
	wxAutoBufferedPaintDC dc(this);
	// The panels get PanelColour() in the dark theme; reading the parent's
	// colour is not reliable for windows that only inherit it
	wxWindow *parent = GetParent();
	dc.SetBackground(wxBrush(parent->UseBgCol() ? parent->GetBackgroundColour() : PanelColour()));
	dc.Clear();

	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
	if (!gc) return;

	auto const& p = palette();
	const wxSize size = GetClientSize();
	const double w = size.GetWidth(), h = size.GetHeight();
	const double r = FromDIP(5);
	const bool down = (pressed && hover) || value;

	if (theme::IsDark()) {
		// WsQ Dark: flat fills like the "Timing (SE)" mockup
		wxColour fill = theme::Field(), border = look == Look::Framed ? theme::FieldBorder() : wxColour();
		if (value) { fill = wxColour(29, 53, 87); border = wxColour(47, 95, 168); }
		else if (pressed && hover) fill = wxColour(35, 38, 43);
		else if (hover && IsEnabled()) fill = wxColour(54, 59, 66);
		// Toolbar tiles are bare icons until hovered, pressed or switched on
		const bool bare = look == Look::Tile && !value && !hover;
		if (!bare) {
			gc->SetBrush(wxBrush(fill));
			gc->SetPen(border.IsOk() ? wxPen(border, 1) : *wxTRANSPARENT_PEN);
			gc->DrawRoundedRectangle(0.5, 0.5, w - 1, h - 1, r);
		}
	}
	// Body: a soft vertical gradient, flat and darker when pressed or on
	else if (down) {
		gc->SetBrush(wxBrush(p.down));
	}
	else {
		wxColour top = hover && IsEnabled() ? p.hover_top : p.top;
		wxColour bottom = hover && IsEnabled() ? p.hover_bottom : p.bottom;
		gc->SetBrush(gc->CreateLinearGradientBrush(0, 0, 0, h, top, bottom));
	}
	if (!theme::IsDark()) {
		gc->SetPen(wxPen(p.border, 1));
		gc->DrawRoundedRectangle(0.5, 0.5, w - 1, h - 1, r);
	}

	// One-pixel highlight just inside the top edge, like Adwaita
	if (!down && !theme::IsDark()) {
		gc->SetPen(wxPen(p.highlight, 1));
		gc->StrokeLine(r, 1.5, w - r, 1.5);
	}

	if (!label.empty()) {
		wxColour text = theme::IsDark() ? theme::Text() : wxColour(46, 52, 54);
		if (!IsEnabled()) text = theme::IsDark() ? wxColour(120, 120, 120) : wxColour(160, 160, 160);
		dc.SetFont(GetFont());
		dc.SetTextForeground(text);
		wxSize ts = dc.GetTextExtent(label);
		dc.DrawText(label, (size.GetWidth() - ts.GetWidth()) / 2, (size.GetHeight() - ts.GetHeight()) / 2 + (down ? 1 : 0));
	}

	if (bitmap.IsOk()) {
		wxBitmap bmp = bitmap.GetBitmapFor(this);
		if (!IsEnabled())
			bmp = bmp.ConvertToDisabled();
		wxSize bs = bmp.GetLogicalSize();
		int x = (size.GetWidth() - bs.GetWidth()) / 2;
		int y = (size.GetHeight() - bs.GetHeight()) / 2 + (down ? 1 : 0);
		dc.DrawBitmap(bmp, x, y, true);
	}
}
