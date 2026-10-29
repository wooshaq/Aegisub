// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
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

/// @file dialog_progress.cpp
/// @brief Progress-bar dialog box for displaying during long operations
/// @ingroup utility
///

#include "dialog_progress.h"

#include "compat.h"
#include "theme.h"
#include "utils.h"

#include <libaegisub/dispatch.h>
#include <libaegisub/exception.h>
#include <libaegisub/util_osx.h>

#include <atomic>
#include <cmath>
#include <vector>
#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/graphics.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#ifdef _MSC_VER
#include <shobjidl.h>
#endif

using agi::dispatch::Main;

/// WsQ: an animated waveform used as the progress bar. Bars fill with the
/// accent colour from the left as the work progresses, a play-cursor line marks
/// the position, and when the progress is unknown a highlight sweeps across.
class WaveProgress final : public wxWindow {
	int range;
	int value = 0;
	bool indeterminate = false;
	wxTimer timer;
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	std::vector<double> bars;

	double Seconds() const {
		using namespace std::chrono;
		return duration<double>(steady_clock::now() - start).count();
	}

	void OnPaint(wxPaintEvent&) {
		wxAutoBufferedPaintDC dc(this);
		std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
		const wxSize size = GetClientSize();
		const bool dark = theme::IsDark();
		const double t = Seconds();

		const wxColour window_bg = GetParent()->GetBackgroundColour();
		const wxColour card     = dark ? wxColour(22, 24, 28) : wxColour(241, 243, 246);
		const wxColour border   = dark ? wxColour(46, 50, 56) : wxColour(214, 219, 225);
		const wxColour idle_bar = dark ? wxColour(58, 64, 72) : wxColour(196, 202, 210);
		const wxColour accent1(76, 141, 255);
		const wxColour accent2(94, 211, 243);
		const wxColour cursor(61, 220, 151);
		const wxColour text_col = dark ? wxColour(230, 232, 235) : wxColour(40, 44, 50);

		dc.SetBackground(wxBrush(window_bg));
		dc.Clear();
		if (!gc) return;

		const double w = size.GetWidth(), h = size.GetHeight();
		const double radius = FromDIP(8);
		gc->SetPen(wxPen(border, 1));
		gc->SetBrush(wxBrush(card));
		gc->DrawRoundedRectangle(0.5, 0.5, w - 1, h - 1, radius);

		const double pad = FromDIP(14);
		const double label_h = FromDIP(18);
		const double area_x = pad, area_w = w - 2 * pad;
		const double area_y = pad, area_h = h - 2 * pad - label_h;
		const double mid = area_y + area_h / 2;
		const double fraction = range > 0 ? double(value) / range : 0;
		const double progress_x = area_x + area_w * fraction;

		const size_t n = bars.size();
		const double step = area_w / n;
		const double bar_w = std::max(2.0, step * 0.55);

		// Indeterminate: a soft highlight sweeping back and forth
		const double sweep = indeterminate ? (0.5 - 0.5 * std::cos(t * 1.6)) : 0;

		for (size_t i = 0; i < n; ++i) {
			const double cx = area_x + step * (i + 0.5);
			const double breathe = 0.82 + 0.18 * std::sin(t * 3.0 + i * 0.35);
			const double amp = bars[i] * breathe * (area_h / 2 - FromDIP(2));
			const double bh = std::max(2.0, amp * 2);

			wxColour col;
			if (indeterminate) {
				const double d = std::abs(double(i) / n - sweep);
				const double k = std::max(0.0, 1.0 - d * 5);
				col = wxColour(
					idle_bar.Red()   + (accent1.Red()   - idle_bar.Red())   * k,
					idle_bar.Green() + (accent1.Green() - idle_bar.Green()) * k,
					idle_bar.Blue()  + (accent1.Blue()  - idle_bar.Blue())  * k);
			}
			else if (cx <= progress_x) {
				// Gradient across the filled part
				const double k = double(i) / n;
				col = wxColour(
					accent1.Red()   + (accent2.Red()   - accent1.Red())   * k,
					accent1.Green() + (accent2.Green() - accent1.Green()) * k,
					accent1.Blue()  + (accent2.Blue()  - accent1.Blue())  * k);
			}
			else
				col = idle_bar;

			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->SetBrush(wxBrush(col));
			gc->DrawRoundedRectangle(cx - bar_w / 2, mid - bh / 2, bar_w, bh, bar_w / 2);
		}

		if (!indeterminate && fraction > 0 && fraction < 1) {
			// Play cursor with a soft glow
			gc->SetPen(wxPen(wxColour(cursor.Red(), cursor.Green(), cursor.Blue(), 70), FromDIP(5)));
			gc->StrokeLine(progress_x, area_y, progress_x, area_y + area_h);
			gc->SetPen(wxPen(cursor, FromDIP(2)));
			gc->StrokeLine(progress_x, area_y, progress_x, area_y + area_h);
		}

		// Percentage, or a gentle "..." when it is unknown
		wxFont font = GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);
		gc->SetFont(font, text_col);
		wxString label = indeterminate
			? wxString(wxS("\u2022 \u2022 \u2022")).Mid(0, 1 + 2 * (int(t * 2.5) % 3))
			: wxString::Format(wxS("%d%%"), int(std::round(fraction * 100)));
		double tw, th;
		gc->GetTextExtent(label, &tw, &th);
		gc->DrawText(label, w - pad - tw, h - pad - th + FromDIP(2));
	}

public:
	WaveProgress(wxWindow *parent, int range)
	: wxWindow(parent, -1, wxDefaultPosition, parent->FromDIP(wxSize(420, 110)), wxFULL_REPAINT_ON_RESIZE)
	, range(range)
	, timer(this)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetMinSize(FromDIP(wxSize(420, 110)));

		// A fixed, speech-like waveform: a few overlapping "words" plus fine detail
		const int count = 56;
		uint32_t seed = 0x5eed;
		auto rnd = [&] { seed = seed * 1664525u + 1013904223u; return (seed >> 8) / double(1 << 24); };
		for (int i = 0; i < count; ++i) {
			const double x = double(i) / count;
			double env = 0.35 + 0.65 * std::pow(std::abs(std::sin(x * 3.14159 * 3.2 + 0.4)), 0.7);
			env *= 0.55 + 0.45 * rnd();
			bars.push_back(std::clamp(env, 0.12, 1.0));
		}

		Bind(wxEVT_PAINT, &WaveProgress::OnPaint, this);
		Bind(wxEVT_TIMER, [this](wxTimerEvent&) { Refresh(false); });
		timer.Start(33);
	}

	~WaveProgress() { timer.Stop(); }

	void SetValue(int v) {
		indeterminate = false;
		value = std::clamp(v, 0, range);
	}

	/// Called repeatedly while the progress is unknown
	void Pulse() { indeterminate = true; }
};

namespace {
	void set_taskbar_progress([[maybe_unused]] int progress) {
#ifdef _MSC_VER
		ITaskbarList3 *taskbar;
		auto hr = ::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
			__uuidof(ITaskbarList3), (LPVOID *)&taskbar);
		if (FAILED(hr)) return;

		hr = taskbar->HrInit();
		if (FAILED(hr)) {
			taskbar->Release();
			return;
		}

		auto hwnd = wxTheApp->GetTopWindow()->GetHWND();
		if (progress == 0 || progress == 100)
			taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
		else if (progress == -1)
			taskbar->SetProgressState(hwnd, TBPF_INDETERMINATE);
		else
			taskbar->SetProgressValue(hwnd, progress, 100);

		taskbar->Release();
#endif
	}
}

class DialogProgressSink final : public agi::ProgressSink {
	DialogProgress *dialog;
	std::atomic<bool> cancelled{false};
	int progress = 0;

public:
	DialogProgressSink(DialogProgress *dialog) : dialog(dialog) { }

	void SetTitle(std::string const& title) override {
		Main().Async([=, this]{ dialog->title->SetLabelText(to_wx(title)); });
	}

	void SetMessage(std::string const& msg) override {
		Main().Async([=, this]{ dialog->text->SetLabelText(to_wx(msg)); });
	}

	void SetProgress(int64_t cur, int64_t max) override {
		int new_progress = mid<int>(0, double(cur) / max * 300, 300);
		if (new_progress != progress) {
			progress = new_progress;
			Main().Async([=, this]{ dialog->SetProgress(new_progress); });
		}
	}

	void Log(std::string const& str) override {
		Main().Async([=, this]{ dialog->pending_log += to_wx(str); });
	}

	bool IsCancelled() override {
		return cancelled;
	}

	void Cancel() {
		cancelled = true;
	}

	void SetIndeterminate() override {
		Main().Async([this]{ dialog->pulse_timer.Start(1000); });
	}
};

DialogProgress::DialogProgress(wxWindow *parent, wxString const& title_text, wxString const& message)
: wxDialog(parent, -1, title_text, wxDefaultPosition, wxDefaultSize, theme::IsDark() ? wxBORDER_NONE : wxBORDER_RAISED)
, pulse_timer(GetEventHandler())
{
	title = new wxStaticText(this, -1, title_text, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
	gauge = new WaveProgress(this, 300);
	text = new wxStaticText(this, -1, message, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
	cancel_button = new wxButton(this, wxID_CANCEL);
	log_output = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxSize(600, 240), wxTE_MULTILINE | wxTE_READONLY);

	// make the title a slightly larger font
	wxFont title_font = title->GetFont();
	int fontsize = title_font.GetPointSize();
	title_font.SetPointSize(fontsize * 1.375);
	title_font.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(title_font);

	wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(title, wxSizerFlags().Expand());
	sizer->Add(gauge, wxSizerFlags(1).Expand().Border());
	sizer->Add(text, wxSizerFlags().Expand());
	sizer->Add(cancel_button, wxSizerFlags().Center().Border());
	sizer->Add(log_output, wxSizerFlags().Expand().Border(wxALL & ~wxTOP));
	sizer->Hide(log_output);

	SetSizerAndFit(sizer);
	CenterOnParent();

	// WsQ Dark: a flat dark popup with a thin border instead of the white
	// raised frame (rounded by Windows 11 itself)
	if (theme::IsDark()) {
		SetBackgroundColour(theme::Bar());
		for (wxWindow *child : {static_cast<wxWindow*>(title), static_cast<wxWindow*>(text)})
			child->SetForegroundColour(theme::Text());
		Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
			wxPaintDC dc(this);
			dc.SetPen(wxPen(theme::Border()));
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(GetClientRect());
		});
		theme::StyleDialogFrame(this);
	}

	Bind(wxEVT_SHOW, &DialogProgress::OnShow, this);
	Bind(wxEVT_TIMER, [this](wxTimerEvent&) { gauge->Pulse(); });
}

void DialogProgress::Run(std::function<void(agi::ProgressSink*)> task) {
	DialogProgressSink ps(this);
	this->ps = &ps;

	auto current_title = from_wx(title->GetLabelText());
	agi::dispatch::Background().Async([=, this]{
		agi::osx::AppNapDisabler app_nap_disabler(current_title);
		try {
			task(this->ps);
		}
		catch (agi::Exception const& e) {
			this->ps->Log(e.GetMessage());
		}

		Main().Async([this]{
			pulse_timer.Stop();
			Unbind(wxEVT_IDLE, &DialogProgress::OnIdle, this);

			// Unbind the cancel handler so that the default behavior happens (i.e. the
			// dialog is closed) as there's no longer a task to cancel
			Unbind(wxEVT_BUTTON, &DialogProgress::OnCancel, this, wxID_CANCEL);

			// If it ran to completion and there is debug output, leave the window open
			// so the user can read the debug output and switch the cancel button to a
			// close button
			bool cancelled = this->ps->IsCancelled();
			if (cancelled || (log_output->IsEmpty() && !pending_log))
				EndModal(!cancelled);
			else {
				if (!pending_log.empty()) {
					wxIdleEvent evt;
					OnIdle(evt);
				}
				cancel_button->SetLabelText(_("Close"));
				gauge->SetValue(300);
			}
			set_taskbar_progress(0);
		});
	});

	if (!ShowModal())
		throw agi::UserCancelException("Cancelled by user");
}

void DialogProgress::OnShow(wxShowEvent& evt) {
	if (!evt.IsShown()) return;

	// Restore the cancel button in case it was previously switched to a close
	// button
	Bind(wxEVT_BUTTON, &DialogProgress::OnCancel, this, wxID_CANCEL);
	Bind(wxEVT_IDLE, &DialogProgress::OnIdle, this);
	cancel_button->SetLabelText(_("Cancel"));
	cancel_button->Enable();

	wxSizer *sizer = GetSizer();
	if (sizer->IsShown(log_output)) {
		sizer->Hide(log_output);
		Layout();
		sizer->Fit(this);
		log_output->Clear();
	}
}

void DialogProgress::OnIdle(wxIdleEvent&) {
	if (progress_current > progress_target) {
		progress_current = progress_target;
		gauge->SetValue(progress_current);
		set_taskbar_progress(progress_current / 3);
	}
	else if (progress_current < progress_target) {
		using namespace std::chrono;
		auto now = steady_clock::now();
		int ms = mid<int>(0, duration_cast<milliseconds>(now - progress_anim_start_time).count(), progress_anim_duration);
		int dist = (progress_target - progress_anim_start_value) * ms / progress_anim_duration;
		if (dist) {
			progress_current = progress_anim_start_value + dist;
			gauge->SetValue(progress_current);
			set_taskbar_progress(progress_current / 3);
		}
	}

	if (!pending_log) return;

	if (log_output->IsEmpty()) {
		wxSizer *sizer = GetSizer();
		sizer->Show(log_output);
		Layout();
		sizer->Fit(this);
		CenterOnParent();
	}

	*log_output << pending_log;
	log_output->SetInsertionPointEnd();
	pending_log.clear();
}

void DialogProgress::OnCancel(wxCommandEvent &) {
	ps->Cancel();
	cancel_button->Enable(false);
	cancel_button->SetLabelText(_("Cancelling..."));
}

void DialogProgress::SetProgress(int target) {
	pulse_timer.Stop();

	if (target == progress_target) return;
	using namespace std::chrono;

	progress_anim_start_value = progress_current;
	auto now = steady_clock::now();
	if (progress_target == 0)
		progress_anim_duration = 1000;
	else
		progress_anim_duration = std::max<int>(100, duration_cast<milliseconds>(now - progress_anim_start_time).count() * 11 / 10);
	progress_anim_start_time = now;
	progress_target = target;
}
