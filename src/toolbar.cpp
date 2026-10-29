// Copyright (c) 2011, Thomas Goyne <plorkyeran@aegisub.org>
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

/// @file toolbar.cpp
/// @brief Dynamic menu toolbar generator.
/// @ingroup toolbar menu

#include "include/aegisub/toolbar.h"

#include "command/command.h"
#include "compat.h"
#include "flat_button.h"
#include "flat_controls.h"
#include "theme.h"
#include "include/aegisub/hotkey.h"
#include "libresrc/libresrc.h"
#include "options.h"

#include <libaegisub/hotkey.h>
#include <libaegisub/json.h>
#include <libaegisub/log.h>
#include <libaegisub/signal.h>
#include <libaegisub/string.h>

#include <boost/interprocess/streams/bufferstream.hpp>
#include <vector>

#include <wx/bmpbuttn.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/tglbtn.h>
#include <wx/toolbar.h>

namespace {
	json::Object const& get_root() {
		static json::Object root;
		if (root.empty()) {
			boost::interprocess::ibufferstream stream((const char *)default_toolbar, sizeof(default_toolbar));
			root = std::move(static_cast<json::Object&>(agi::json_util::parse(stream)));
		}
		return root;
	}

	class Toolbar final : public wxToolBar {
		/// Window ID of first toolbar control
		static const int TOOL_ID_BASE = 5000;

		/// Toolbar name in config file
		std::string name;
		/// Project context
		agi::Context *context;
		/// Commands for each of the buttons
		std::vector<cmd::Command *> commands;
		/// Hotkey context
		std::string ht_context;

		/// Current icon size
		int icon_size;

		/// Listener for icon size change signal
		agi::signal::Connection icon_size_slot;

		/// Listener for hotkey change signal
		agi::signal::Connection hotkeys_changed_slot;

		/// Enable/disable the toolbar buttons
		void OnIdle(wxIdleEvent &) {
			for (size_t i = 0; i < commands.size(); ++i) {
				if (commands[i]->Type() & cmd::COMMAND_VALIDATE)
					EnableTool(TOOL_ID_BASE + i, commands[i]->Validate(context));
				if (commands[i]->Type() & cmd::COMMAND_TOGGLE || commands[i]->Type() & cmd::COMMAND_RADIO)
					ToggleTool(TOOL_ID_BASE + i, commands[i]->IsActive(context));
			}
		}

		/// Toolbar button click handler
		void OnClick(wxCommandEvent &evt) {
			(*commands[evt.GetId() - TOOL_ID_BASE])(context);
		}

		/// Regenerate the toolbar when the icon size changes
		void OnIconSizeChange(agi::OptionValue const& opt) {
			icon_size = opt.GetInt();
			RegenerateToolbar();
		}

		/// Clear the toolbar and recreate it
		void RegenerateToolbar() {
			Unbind(wxEVT_IDLE, &Toolbar::OnIdle, this);
			ClearTools();
			commands.clear();
			Populate();
		}

		/// Populate the toolbar with buttons
		void Populate() {
			json::Object const& root = get_root();
			auto root_it = root.find(name);
			if (root_it == root.end()) {
				// Toolbar names are all hardcoded so this should never happen
				throw agi::InternalError("Toolbar named " + name + " not found.");
			}

			json::Array const& arr = root_it->second;
			commands.reserve(arr.size());
			bool needs_onidle = false;
			bool last_was_sep = false;

			for (json::String const& command_name : arr) {
				if (command_name.empty()) {
					if (!last_was_sep)
						AddSeparator();
					last_was_sep = true;
					continue;
				}

				// WsQ: small field with the frame count for the quick shift buttons
				if (command_name == "wsq/shift_frames") {
					auto spin = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition,
						wxSize(FromDIP(52), -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 1, 999,
						std::max<int>(1, OPT_GET("Tool/WsQ/Shift Frames")->GetInt()));
					spin->SetToolTip(_("Frames to shift by with the arrow buttons"));
					auto store = [spin](wxCommandEvent&) { OPT_SET("Tool/WsQ/Shift Frames")->SetInt(spin->GetValue()); };
					spin->Bind(wxEVT_SPINCTRL, store);
					spin->Bind(wxEVT_TEXT, store);
					AddControl(spin);
					last_was_sep = false;
					continue;
				}

				cmd::Command *command;
				try {
					command = cmd::get(command_name);
				}
				catch (cmd::CommandNotFound const&) {
					LOG_W("toolbar/command/not_found") << "Command '" << command_name << "' not found; skipping";
					continue;
				}

				last_was_sep = false;

				int flags = command->Type();
				wxItemKind kind =
					flags & cmd::COMMAND_RADIO ? wxITEM_RADIO :
					flags & cmd::COMMAND_TOGGLE ? wxITEM_CHECK :
					wxITEM_NORMAL;

				AddTool(TOOL_ID_BASE + commands.size(), command->StrDisplay(context), command->Icon(icon_size, GetLayoutDirection()), command->GetTooltip(ht_context), kind);

				commands.push_back(command);
				needs_onidle = needs_onidle || flags != cmd::COMMAND_NORMAL;
			}

			// Only bind the update function if there are actually any dynamic tools
			if (needs_onidle) {
				Bind(wxEVT_IDLE, &Toolbar::OnIdle, this);
			}

			Realize();
		}

	public:
		Toolbar(wxWindow *parent, std::string name, agi::Context *c, std::string ht_context, bool vertical)
		: wxToolBar(parent, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT | (vertical ? wxTB_VERTICAL : wxTB_HORIZONTAL))
		, name(std::move(name))
		, context(c)
		, ht_context(std::move(ht_context))
		, icon_size(OPT_GET("App/Toolbar Icon Size")->GetInt())
		, icon_size_slot(OPT_SUB("App/Toolbar Icon Size", &Toolbar::OnIconSizeChange, this))
		, hotkeys_changed_slot(hotkey::inst->AddHotkeyChangeListener(&Toolbar::RegenerateToolbar, this))
		{
			if (theme::IsDark()) SetBackgroundColour(theme::Window());
			Populate();
			Bind(wxEVT_TOOL, &Toolbar::OnClick, this);
			Bind(wxEVT_DPI_CHANGED, [this] (wxDPIChangedEvent &e) { RegenerateToolbar(); e.Skip(); });
		}

		Toolbar(wxFrame *parent, std::string name, agi::Context *c, std::string ht_context)
		: wxToolBar(parent, -1, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_HORIZONTAL)
		, name(std::move(name))
		, context(c)
		, ht_context(std::move(ht_context))
#ifndef __WXMAC__
		, icon_size(OPT_GET("App/Toolbar Icon Size")->GetInt())
		, icon_size_slot(OPT_SUB("App/Toolbar Icon Size", &Toolbar::OnIconSizeChange, this))
#else
		, icon_size(32)
#endif
		, hotkeys_changed_slot(hotkey::inst->AddHotkeyChangeListener(&Toolbar::RegenerateToolbar, this))
		{
			if (theme::IsDark()) SetBackgroundColour(theme::Bar());
			parent->SetToolBar(this);
			Populate();
			Bind(wxEVT_TOOL, &Toolbar::OnClick, this);
			Bind(wxEVT_DPI_CHANGED, [this] (wxDPIChangedEvent &e) { RegenerateToolbar(); e.Skip(); });
		}
	};
}

namespace {
	/// WsQ: a toolbar made of real framed buttons, sized like the edit box
	/// buttons, so the audio toolbar matches the rows below it
	class ButtonBar final : public wxPanel {
		struct Button {
			cmd::Command *command;
			FlatButton *button;
			FlatButton *toggle; ///< Same as button for toggles, else null
		};

		std::string name;
		agi::Context *context;
		std::string ht_context;
		std::vector<Button> buttons;
		agi::signal::Connection hotkeys_changed_slot;

		void UpdateTooltips() {
			for (auto const& b : buttons)
				b.button->SetToolTip(b.command->GetTooltip(ht_context));
		}

		void OnIdle(wxIdleEvent &evt) {
			evt.Skip();
			for (auto const& b : buttons) {
				int type = b.command->Type();
				if (type & cmd::COMMAND_VALIDATE) {
					bool enable = b.command->Validate(context);
					if (b.button->IsEnabled() != enable)
						b.button->Enable(enable);
				}
				if (b.toggle && (type & (cmd::COMMAND_TOGGLE | cmd::COMMAND_RADIO))) {
					bool active = b.command->IsActive(context);
					if (b.toggle->GetValue() != active)
						b.toggle->SetValue(active);
				}
			}
		}

	public:
		ButtonBar(wxWindow *parent, std::string name, agi::Context *c, std::string ht_context)
		: wxPanel(parent, -1)
		, name(std::move(name))
		, context(c)
		, ht_context(std::move(ht_context))
		, hotkeys_changed_slot(hotkey::inst->AddHotkeyChangeListener(&ButtonBar::UpdateTooltips, this))
		{
			auto root_it = get_root().find(this->name);
			if (root_it == get_root().end())
				throw agi::InternalError("Toolbar named " + this->name + " not found.");

			const wxSize button_size = FromDIP(wxSize(26, 26));
			const int icon_size = 16;
			auto sizer = new wxBoxSizer(wxHORIZONTAL);
			bool gap_pending = false;

			const bool dark = theme::IsDark();
			if (dark) SetBackgroundColour(theme::Bar());
			auto add = [&](wxWindow *w) {
				if (!sizer->IsEmpty()) {
					if (dark && gap_pending) {
						// WsQ Dark: a thin divider between groups, like the mockup
						auto line = new wxPanel(this, -1, wxDefaultPosition, FromDIP(wxSize(1, 18)));
						line->SetBackgroundColour(wxColour(54, 59, 66));
						sizer->Add(line, wxSizerFlags().Center().Border(wxLEFT | wxRIGHT, FromDIP(5)));
					}
					else
						sizer->AddSpacer(FromDIP(gap_pending ? 10 : dark ? 4 : 2));
				}
				gap_pending = false;
				sizer->Add(w, wxSizerFlags().Center());
			};

			for (json::String const& command_name : static_cast<json::Array const&>(root_it->second)) {
				if (command_name.empty()) {
					gap_pending = true;
					continue;
				}

				if (command_name == "wsq/shift_frames") {
					auto spin = new FlatSpinCtrl(this, std::max<int>(1, OPT_GET("Tool/WsQ/Shift Frames")->GetInt()), 1, 999,
						wxSize(FromDIP(78), button_size.GetHeight()));
					spin->SetToolTip(_("Frames to shift by with the arrow buttons"));
					auto store = [spin](wxCommandEvent&) { OPT_SET("Tool/WsQ/Shift Frames")->SetInt(spin->GetValue()); };
					spin->Bind(wxEVT_TEXT, store);
					add(spin);
					continue;
				}

				cmd::Command *command;
				try {
					command = cmd::get(command_name);
				}
				catch (cmd::CommandNotFound const&) {
					LOG_W("toolbar/command/not_found") << "Command '" << command_name << "' not found; skipping";
					continue;
				}

				FlatButton *toggle = nullptr;
				FlatButton *button = new FlatButton(this, command->Icon(icon_size),
					command->Type() & (cmd::COMMAND_TOGGLE | cmd::COMMAND_RADIO));
				if (command->Type() & (cmd::COMMAND_TOGGLE | cmd::COMMAND_RADIO))
					toggle = button;
				button->SetLook(FlatButton::Look::Tile);
				// At least as large as the edit box buttons; larger only if the
				// platform's button padding needs it, so the icon is never clipped
				wxSize size = button->GetBestSize();
				size.IncTo(button_size);
				button->SetMinSize(size);
				button->SetToolTip(command->GetTooltip(this->ht_context));
				button->Bind(toggle ? wxEVT_TOGGLEBUTTON : wxEVT_BUTTON, [this, command](wxCommandEvent&) {
					(*command)(context);
				});
				buttons.push_back({command, button, toggle});
				add(button);
			}

			auto outer = new wxBoxSizer(wxHORIZONTAL);
			outer->Add(sizer, wxSizerFlags().Border(wxTOP | wxBOTTOM, FromDIP(3)));
			SetSizerAndFit(outer);
			Bind(wxEVT_IDLE, &ButtonBar::OnIdle, this);
		}
	};
}

namespace toolbar {
	wxWindow *GetButtonBar(wxWindow *parent, std::string const& name, agi::Context *c, std::string const& hotkey) {
		return new ButtonBar(parent, name, c, hotkey);
	}

	void AttachToolbar(wxFrame *frame, std::string const& name, agi::Context *c, std::string const& hotkey) {
		new Toolbar(frame, name, c, hotkey);
	}

	wxToolBar *GetToolbar(wxWindow *parent, std::string const& name, agi::Context *c, std::string const& hotkey, bool vertical) {
		return new Toolbar(parent, name, c, hotkey, vertical);
	}
}
