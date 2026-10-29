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

/// @file theme.h
/// @brief WsQ dark theme

#pragma once

class wxColour;
class wxFont;

namespace theme {
/// Apply the colour palette for the selected theme if it isn't yet, switch
/// native controls to dark mode if wanted, and follow later changes to the
/// theme option. Must be called before any window is created.
void Init();

/// Is the dark theme selected?
bool IsDark();

/// Monospace font of the same size as base, for times and numbers
wxFont MonoFont(wxFont const& base);

/// WsQ Dark colours (the "Timing (SE)" layout). In the light theme these
/// fall back to the system colours where that makes sense.
wxColour Window();      ///< main window background
wxColour Bar();         ///< toolbars, card bodies
wxColour Border();      ///< card and panel borders
wxColour Field();       ///< text field and button background
wxColour FieldBorder(); ///< text field and button border
wxColour Text();        ///< normal text
wxColour TextDim();     ///< labels and secondary text
wxColour Accent();      ///< blue accent
}
