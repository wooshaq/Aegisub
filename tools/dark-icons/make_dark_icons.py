#!/usr/bin/env python3
"""White line icons for the WsQ dark theme.

Every icon is a small SVG on a 24x24 grid. Running this script renders each one
to src/bitmaps/button/<name>_dark_<size>.png for every size the light icon
comes in, and lists them in src/bitmaps/manifest.respack. The PNGs are
committed, so building Aegisub does not need this script or cairosvg.

    pip install cairosvg
    python3 tools/dark-icons/make_dark_icons.py [--sheet out.png]

Colour is kept only where it carries meaning: red = line start, blue = line
end, green = confirm/add, red cross = close/remove, plus the colour-swatch
icons and the test-pattern icon.
"""

import argparse
import os
import sys

import cairosvg

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
BUTTON_DIR = os.path.join(ROOT, 'src', 'bitmaps', 'button')
MANIFEST = os.path.join(ROOT, 'src', 'bitmaps', 'manifest.respack')
SIZES = (16, 24, 32, 48, 64)

W = '#EEF1F5'    # main stroke
M = '#AEB6C1'    # secondary stroke
R = '#FF6B6F'    # line start / remove / error
B = '#6EA8FF'    # line end
G = '#4ADE80'    # confirm / add
C = '#5ED3F3'    # go / scroll
A = '#F5B84B'    # highlight
P = '#C49BFF'    # styles / tags
BG = '#25282D'   # badge backing disc
SW = 1.9         # default stroke width on the 24 grid

FONT = 'DejaVu Sans'
SERIF = 'DejaVu Serif'


# ---------------------------------------------------------------------------
# Primitives

def path(d, c=W, w=SW, fill='none', dash=None, cap='round'):
    extra = f' stroke-dasharray="{dash}"' if dash else ''
    return (f'<path d="{d}" fill="{fill}" stroke="{c}" stroke-width="{w}" '
            f'stroke-linecap="{cap}" stroke-linejoin="round"{extra}/>')


def fill(d, c=W):
    return f'<path d="{d}" fill="{c}"/>'


def circ(cx, cy, r, c=W, w=SW, f='none'):
    stroke = f'stroke="{c}" stroke-width="{w}"' if c != 'none' else ''
    return f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{f}" {stroke}/>'


def rect(x, y, w, h, rx=0, c=W, sw=SW, f='none', dash=None):
    stroke = f'stroke="{c}" stroke-width="{sw}" stroke-linejoin="round"' if c != 'none' else ''
    extra = f' stroke-dasharray="{dash}"' if dash else ''
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{f}" {stroke}{extra}/>'


def text(x, y, s, size, c=W, weight='bold', family=FONT, style='normal', anchor='middle', outline=None):
    o = f' stroke="{outline}" stroke-width="1.6" paint-order="stroke"' if outline else ''
    return (f'<text x="{x}" y="{y}" font-family="{family}" font-size="{size}" font-weight="{weight}" '
            f'font-style="{style}" text-anchor="{anchor}" fill="{c}"{o}>{s}</text>')


def group(transform, *items):
    return f'<g transform="{transform}">' + ''.join(items) + '</g>'


def place(glyph, x, y, s, disc=True):
    """Draw a glyph (a function of stroke width, on the 24 grid) scaled by s at x, y."""
    inner = (circ(12, 12, 12, c='none', f=BG) if disc else '') + glyph(SW / s * 0.85)
    return group(f'translate({x},{y}) scale({s})', inner)


def badge(glyph, corner='br'):
    """Small glyph in a bottom corner, on a dark disc so it reads over the icon."""
    x = 11 if corner == 'br' else 0
    return place(glyph, x, 11, 0.54)


# ---------------------------------------------------------------------------
# Glyphs: functions of stroke width so they can be drawn small

def g_doc(w=SW):
    return path('M5.5 2.5H14l4.5 4.5v13a1.5 1.5 0 0 1-1.5 1.5H5.5A1.5 1.5 0 0 1 4 20V4a1.5 1.5 0 0 1 1.5-1.5z', w=w) + \
        path('M14 2.5V7h4.5', w=w)


def g_speaker(w=SW, c=W):
    return fill('M3.5 9H7l5-4v14l-5-4H3.5z', c) + \
        path('M15.5 8.8a4.6 4.6 0 0 1 0 6.4', c, w) + path('M18.4 6a8.6 8.6 0 0 1 0 12', c, w)


def g_clock(w=SW, c=W):
    return circ(12, 12, 8.5, c, w) + path('M12 7.5V12l3 2', c, w)


def g_key(w=SW, c=W):
    return circ(7.5, 12, 4, c, w) + path('M11.5 12H21M18 12v3.5M21 12v2.5', c, w)


def g_screen(w=SW, c=W):
    return rect(2.5, 4, 19, 13, 1.5, c, w) + path('M8.5 21h7M12 17v4', c, w)


def g_x(w=SW, c=R):
    return path('M6 6l12 12M18 6L6 18', c, w * 1.25)


def g_plus(w=SW, c=G):
    return path('M12 5v14M5 12h14', c, w * 1.25)


def g_minus(w=SW, c=W):
    return path('M5 12h14', c, w * 1.25)


def g_check(w=SW, c=G):
    return path('M4.5 12.5l5 5L19.5 7', c, w * 1.3)


def g_pencil(w=SW, c=A):
    return path('M16.5 3.5l4 4L8.5 19.5 3 21l1.5-5.5z', c, w) + path('M14 6l4 4', c, w)


def g_arrow_right(w=SW, c=C):
    return path('M4 12h15M13 6l6 6-6 6', c, w * 1.2)


def g_auto(w=SW, c=B):
    # circular arrows: "automatic"
    return path('M19 12a7 7 0 0 1-12.2 4.7M5 12a7 7 0 0 1 12.2-4.7', c, w) + \
        path('M17.5 3.5v4h-4M6.5 20.5v-4h4', c, w)


def g_floppy(w=SW):
    return path('M5 3h11.5L21 7.5V19.5A1.5 1.5 0 0 1 19.5 21h-15A1.5 1.5 0 0 1 3 19.5v-15A1.5 1.5 0 0 1 4.5 3z', w=w) + \
        path('M7.5 3v5h8V3', w=w) + rect(6.5, 13, 11, 8, 1, W, w)


def g_folder_open(w=SW):
    return path('M3 18.5V5.5A1.5 1.5 0 0 1 4.5 4h4.2l2 2.2h7.8a1.5 1.5 0 0 1 1.5 1.5V10', w=w) + \
        path('M3 18.5L6.2 11h16l-3.2 8.5z', w=w)


def g_folder(w=SW):
    return path('M3 6.5A1.5 1.5 0 0 1 4.5 5h4.2l2 2.2h8.8a1.5 1.5 0 0 1 1.5 1.5v9.8a1.5 1.5 0 0 1-1.5 1.5h-15A1.5 1.5 0 0 1 3 18.5z', w=w)


def g_window(w=SW):
    return rect(2.5, 4, 19, 16, 1.5, W, w) + path('M2.5 8.5h19', w=w)


def g_binoculars(w=SW, c=W):
    return circ(6.5, 15.5, 3.8, c, w) + circ(17.5, 15.5, 3.8, c, w) + path('M10.3 15h3.4', c, w) + \
        path('M2.8 15L5.5 5.5h3.2l1.3 6M21.2 15L18.5 5.5h-3.2l-1.3 6', c, w)


def g_magnifier(w=SW):
    return circ(10, 10, 6.5, W, w) + path('M14.8 14.8L21 21', w=w * 1.3)


def g_bracket_start(x, c=R, w=SW * 1.15):
    return path(f'M{x + 2.5} 5H{x}v14h{2.5}', c, w)


def g_bracket_end(x, c=B, w=SW * 1.15):
    return path(f'M{x - 2.5} 5H{x}v14h{-2.5}', c, w)


def play(x, y, s=1.0, c=W):
    return fill(f'M{x} {y - 4 * s}l{6.5 * s} {4 * s}-{6.5 * s} {4 * s}z', c)


def gear(cx, cy, r_out, r_in, teeth, c=W, w=SW):
    import math
    pts = []
    for i in range(teeth * 2):
        a0 = math.pi * 2 * i / (teeth * 2)
        a1 = math.pi * 2 * (i + 1) / (teeth * 2)
        r = r_out if i % 2 == 0 else r_in
        for a in (a0 + 0.12, a1 - 0.12):
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    d = 'M' + ' L'.join(f'{x:.2f} {y:.2f}' for x, y in pts) + 'z'
    return path(d, c, w)


def timeline(y, marker_x, marker_c):
    return path(f'M3 {y}h18', M, SW) + rect(marker_x - 1.5, y - 3, 3, 6, 0.8, 'none', 0, marker_c)


def bubble(x, y, w_, h_, tail_left=True, c=W, w=SW):
    r = 2
    xr, yb = x + w_, y + h_
    if tail_left:
        tail = f'H{x + 8}L{x + 4} {yb + 3.5}V{yb}'
    else:
        tail = f'H{xr - 4}V{yb + 3.5}L{xr - 8} {yb}'
    d = (f'M{x + r} {y}H{xr - r}A{r} {r} 0 0 1 {xr} {y + r}V{yb - r}A{r} {r} 0 0 1 {xr - r} {yb}'
         f'{tail}H{x + r}A{r} {r} 0 0 1 {x} {yb - r}V{y + r}A{r} {r} 0 0 1 {x + r} {y}z')
    return path(d, c, w)


def grid_rows(highlight, mark=None):
    out = rect(3, 4, 18, 16, 1.5)
    for i, y in enumerate((8, 12, 16)):
        out += path(f'M3 {y}h18', M, 1.2)
    for row in highlight:
        y = 4 + 4 * row
        out += rect(4.2, y + 0.9, 15.6, 2.2, 0.5, 'none', 0, C)
    if mark:
        out += mark
    return out


def points(*xy, c=R, s=2.6):
    return ''.join(rect(x - s / 2, y - s / 2, s, s, 0.4, 'none', 0, c) for x, y in xy)


# ---------------------------------------------------------------------------
# Icons

ICONS = {}


def icon(name):
    def deco(fn):
        ICONS[name] = fn
        return fn
    return deco


def simple(name, *parts):
    ICONS[name] = lambda: ''.join(parts)


# General
simple('about_menu', circ(12, 12, 9), path('M12 11v5.5'), circ(12, 7.6, 1.2, 'none', 0, W))
simple('arrow_down', fill('M5 8h14l-7 9z'))
simple('arrow_up', fill('M5 16h14l-7-9z'))
simple('arrow_down_stop', fill('M5 5h14l-7 9z'), path('M5 18.5h14', w=2.4))
simple('arrow_up_stop', fill('M5 19h14l-7-9z'), path('M5 5.5h14', w=2.4))
simple('arrow_sort', path('M20 5v5h-5'), path('M4 19v-5h5'),
       path('M5.5 9.5a7.5 7.5 0 0 1 13.4-1.5l1.1 2M18.5 14.5a7.5 7.5 0 0 1-13.4 1.5L4 14'))
simple('attach_button', path('M20.5 11.5l-8.6 8.6a5.5 5.5 0 0 1-7.8-7.8l8.6-8.6a3.7 3.7 0 0 1 5.2 5.2l-8.6 8.6a1.8 1.8 0 0 1-2.6-2.6l7.9-7.9'))
simple('automation_toolbutton', gear(12, 12, 9, 6.6, 8), circ(12, 12, 2.8))
simple('bugtracker_button',
       rect(7.5, 8, 9, 12.5, 4.5), path('M12 8v12.5', M, 1.4),
       path('M9 8.2a3 3 0 0 1 6 0'), path('M9.5 5.5L8 3.5M14.5 5.5L16 3.5'),
       path('M7.5 12H3.5M7.5 16H4M8 19.5l-3 2M16.5 12h4M16.5 16H20M16 19.5l3 2'))
simple('contents_button', path('M4 5.5A2.5 2.5 0 0 1 6.5 3H20v15H6.5A2.5 2.5 0 0 0 4 20.5z'), path('M4 20.5A2.5 2.5 0 0 0 6.5 23H20v-5'),
       path('M10 8.3a2.4 2.4 0 1 1 3.6 2.1c-.8.5-1.4 1-1.4 2', B, 1.7), circ(12.2, 15.2, 0.9, 'none', 0, B))
simple('copy_button', rect(8.5, 8.5, 12, 13, 1.5), path('M15.5 8.5V4.5A1.5 1.5 0 0 0 14 3H5A1.5 1.5 0 0 0 3.5 4.5v11A1.5 1.5 0 0 0 5 17h3.5'))
simple('cut_button', circ(6, 6.5, 3), circ(6, 17.5, 3), path('M20 4.5L8.4 15.4M14.3 13.6L20 19.5M8.4 8.6L11.5 11.5'))
simple('delete_button', g_x(SW, R))
simple('paste_button', rect(4.5, 4.5, 15, 17, 2), rect(8.5, 2.5, 7, 4, 1, W, SW, BG), path('M8.5 11h7M8.5 14.5h7M8.5 18h4', M, 1.6))
simple('properties_toolbutton', rect(4.5, 4.5, 15, 17, 2), rect(8.5, 2.5, 7, 4, 1, W, SW, BG),
       path('M8 11h1M11 11h5M8 14.5h1M11 14.5h5M8 18h1M11 18h5', M, 1.6))
simple('undo_button', path('M9 13.5L4 8.5l5-5'), path('M4 8.5h10.5a5.5 5.5 0 0 1 0 11H10'))
simple('redo_button', path('M15 13.5l5-5-5-5'), path('M20 8.5H9.5a5.5 5.5 0 0 0 0 11H14'))
simple('find_button', g_binoculars())
simple('find_next_menu', group('translate(0,-1) scale(0.8)', g_binoculars(SW / 0.8)), badge(g_arrow_right))
simple('find_replace_menu', group('translate(0,-1) scale(0.8)', g_binoculars(SW / 0.8)), badge(g_pencil))
simple('zoom_in_button', g_magnifier(), path('M10 7v6M7 10h6'))
simple('zoom_out_button', g_magnifier(), path('M7 10h6'))
simple('website_button', circ(12, 12, 9), path('M3 12h18'), path('M12 3c3 3.2 3 14.8 0 18M12 3c-3 3.2-3 14.8 0 18'))
simple('irc_button', bubble(2.5, 3.5, 19, 13), text(12, 13.4, 'IRC', 7.2, R))
simple('options_button', path('M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.8-3.8a6 6 0 0 1-7.9 7.9l-6.9 6.9a2.1 2.1 0 0 1-3-3l6.9-6.9a6 6 0 0 1 7.9-7.9z'))
simple('eyedropper_tool', path('M3 21l1-1h2.5l8.5-8.5'), path('M4 20v-2.5l8.5-8.5'),
       path('M14.5 6.5l3.2-3.2a2.1 2.1 0 1 1 3 3l-3.2 3.2.5.5a1.4 1.4 0 0 1-2 2L11.2 7.2a1.4 1.4 0 0 1 2-2z'))
simple('languages_menu', bubble(2.5, 3.5, 19, 13), text(12, 13.6, 'Aa', 8.5, W))
simple('join_dialogue_button', bubble(2, 3, 13, 9.5), bubble(9, 10, 13, 9.5, tail_left=False, c=A))
simple('jumpto_button', path('M3 12h12M11 7.5l4.5 4.5-4.5 4.5', C, 2.2), path('M19.5 4.5v15', W, 2.2))

# Files and windows
simple('new_toolbutton', g_doc())
simple('new_window_menu', g_window(), badge(g_plus))
simple('open_toolbutton', g_folder_open())
simple('open_with_toolbutton', g_folder_open(), badge(g_plus))
simple('save_toolbutton', g_floppy())
simple('save_as_toolbutton', g_floppy(), badge(g_pencil))
simple('save_keyframes_menu', g_floppy(), badge(g_key))
simple('save_timecodes_menu', g_floppy(), badge(g_clock))
simple('open_keyframes_menu', g_doc(), place(g_key, 5.5, 9, 0.5, disc=False))
simple('open_timecodes_menu', g_doc(), place(g_clock, 5.3, 8.6, 0.5, disc=False))
simple('open_audio_menu', g_doc(), place(g_speaker, 5.2, 8.6, 0.5, disc=False))
simple('open_video_menu', g_doc(), place(g_screen, 5.3, 8.4, 0.5, disc=False))
simple('open_audio_from_video_menu', g_screen(), place(g_speaker, 6.6, 4.6, 0.45, disc=False))
simple('close_audio_menu', g_speaker(), badge(g_x, 'bl'))
simple('close_keyframes_menu', g_doc(), place(g_key, 5.5, 9, 0.5, disc=False), badge(g_x, 'bl'))
simple('close_timecodes_menu', g_doc(), place(g_clock, 5.3, 8.6, 0.5, disc=False), badge(g_x, 'bl'))
simple('close_video_menu', g_screen(), badge(g_x, 'bl'))
simple('detach_video_menu', rect(2.5, 7.5, 13, 13, 1.5), path('M13 3h8v8M21 3l-9 9', C))
simple('export_menu', g_doc(), path('M11 15h10M18 12l3 3-3 3', G, 2.1))
simple('font_collector_button', path('M8 6V3.5h11.5v15H17', M), rect(4.5, 6, 12.5, 15, 1.5), path('M7.5 10h6.5M10.75 10v8'))

# Subtitle editing
simple('button_bold', text(12, 18, 'B', 17, W, 'bold'))
simple('button_italics', text(12, 18, 'I', 18, W, 'normal', SERIF, 'italic'))
simple('button_underline', text(12, 16.5, 'U', 15, W, 'bold'), path('M6 21h12', w=2))
simple('button_strikeout', text(12, 18, 'S', 16, W, 'bold'), path('M4 12.6h16', R, 2))
simple('button_fontname', text(12, 17, 'fn', 15, W, 'bold', SERIF, 'italic'))
simple('button_color_one', text(12, 17.5, 'AB', 13, R, 'bold'))
simple('button_color_two', text(7.6, 17.5, 'A', 13, R, 'bold'), text(16.4, 17.5, 'B', 13, W, 'bold'))
simple('button_color_three', text(12, 17.5, 'AB', 13, BG, 'bold', outline=R))
simple('button_color_four', text(13.3, 18.8, 'AB', 13, R, 'bold'), text(12, 17.5, 'AB', 13, W, 'bold'))
simple('toggle_tag_hiding', text(12, 17, '{\\t}', 11.5, P, 'bold'))
simple('style_toolbutton', text(12, 18.5, 'S', 19, P, 'bold', SERIF, 'italic'))
simple('styling_toolbutton', text(12, 16, 'S', 15, P, 'bold', SERIF, 'italic'), path('M3 20c3-3 6 3 9 0s6-3 9 0', C, 1.8))
simple('spellcheck_toolbutton', text(10, 11, 'ABC', 8, W, 'bold'), g_check(SW).replace('M4.5 12.5l5 5L19.5 7', 'M8 17.5l3.5 3.5L20 12.5'))
simple('translation_toolbutton', path('M2.5 12c3.2-5.2 9.8-6 14.5-1.8L21.5 7v10l-4.5-3.2C12.3 18 5.7 17.2 2.5 12z', A), circ(7, 11, 1, 'none', 0, A))
simple('select_lines_button', grid_rows([1, 2]))
simple('select_visible_button', grid_rows([1], path('M1.5 9v6M22.5 9v6', A, 2.2)))
simple('kara_split_accept', rect(2.5, 4, 8, 7, 1), rect(13.5, 4, 8, 7, 1), path('M7 17l3.5 3.5L18 13', G, 2.4))
simple('kara_split_cancel', rect(2.5, 4, 8, 7, 1), rect(13.5, 4, 8, 7, 1), path('M8 13.5l8 8M16 13.5l-8 8', R, 2.4))
simple('kara_timing_copier', rect(2.5, 3, 9, 7, 1), path('M5 6.5h4', M, 1.6), rect(12.5, 14, 9, 7, 1), path('M15 17.5h4', M, 1.6),
       path('M7 10.5v4a2.5 2.5 0 0 0 2.5 2.5h1', C), path('M9 15l2 2-2 2', C))
simple('kara_mode', rect(9, 2.5, 6, 11, 3), path('M5.5 10.5a6.5 6.5 0 0 0 13 0M12 17v4M8.5 21h7'))

# Timing and audio
simple('button_play', fill('M7 4.5l12 7.5-12 7.5z'))
simple('button_pause', rect(6, 5, 4, 14, 1, 'none', 0, W), rect(14, 5, 4, 14, 1, 'none', 0, W))
simple('button_stop', rect(6, 6, 12, 12, 1.5, 'none', 0, W))
simple('button_next', fill('M3.5 6l8 6-8 6zM12 6l8 6-8 6z'))
simple('button_prev', fill('M20.5 6l-8 6 8 6zM12 6l-8 6 8 6z'))
simple('button_playline', g_bracket_start(3.5), play(9, 12), g_bracket_end(20.5))
simple('button_playsel', g_bracket_end(6, B), play(9, 12), g_bracket_start(18, B))
simple('button_playfirstfiveh', g_bracket_start(2.5), play(7, 12), path('M14.5 12h5M20.5 8.5v7', R, 2))
simple('button_playlastfiveh', path('M3.5 8.5v7M4.5 12h5', B, 2), play(10.5, 12), g_bracket_end(21.5))
simple('button_playfivehbefore', path('M2 12h1.5M5.5 12h1.5', R, 2), play(9, 12), g_bracket_start(18.5))
simple('button_playfivehafter', g_bracket_end(5.5), play(8.5, 12), path('M17 12h1.5M20.5 12h1.5', B, 2))
simple('button_playtoend', g_bracket_start(2.5), path('M8.5 9v6M11 8v8', M, 1.8), fill('M13.5 5.5l8 6.5-8 6.5z'))
simple('button_leadin', path('M2 12h2.5M6.5 12h2.5M11 12h2.5', G, 2.2), g_bracket_start(17))
simple('button_leadout', g_bracket_end(7), path('M10.5 12H13M15 12h2.5M19.5 12H22', G, 2.2))
simple('button_audio_commit', g_check(SW))
simple('button_audio_goto', path('M3.5 12h14M12.5 6l6 6-6 6', C, 2.4))
simple('toggle_audio_autocommit', group('translate(-1,-2)', g_check(SW)), badge(g_auto))
simple('toggle_audio_nextcommit', group('translate(-2,-2) scale(0.92)', g_check(SW / 0.92)), badge(g_arrow_right))
simple('toggle_audio_medusa', rect(2.5, 6, 19, 12, 2), path('M6 10h1M9.5 10h1M13 10h1M16.5 10h1M7.5 14h9', W, 1.9))
simple('toggle_audio_autoscroll', path('M3 7v10h18V7', C), path('M7.5 12h9M10 9.5L7.5 12l2.5 2.5M14 9.5l2.5 2.5-2.5 2.5'))
simple('toggle_audio_link', path('M10 13.5a4.5 4.5 0 0 0 6.8.5l3-3a4.5 4.5 0 0 0-6.4-6.4l-1.7 1.7'),
       path('M14 10.5a4.5 4.5 0 0 0-6.8-.5l-3 3a4.5 4.5 0 0 0 6.4 6.4l1.7-1.7'))
simple('toggle_audio_spectrum',
       *[rect(x, 21 - h, 3, h, 0.8, 'none', 0, col) for x, h, col in
         ((2.5, 7, B), (6.5, 14, C), (10.5, 18, G), (14.5, 11, A), (18.5, 6, R))])
simple('toggle_video_autoscroll', g_screen(), place(g_arrow_right, 5.5, 2.2, 0.55, disc=False), badge(g_auto))
simple('timing_processor_toolbutton', circ(12, 13.5, 8), path('M12 9v4.5l3 2'), path('M9.5 2.5h5M12 2.5v3M18.5 6l1.5-1.5'))
simple('shift_times_toolbutton', circ(9, 12, 6.5), path('M9 8.5V12l2.2 1.5'), path('M16.5 12h5M19 9l2.5 3-2.5 3', C, 2))
simple('shift_to_frame', rect(5, 2.5, 14, 9, 1.2), path('M8 2.5v9M16 2.5v9', M, 1.3), path('M12 13.5v4M9.5 15.5l2.5 2.5 2.5-2.5', C), timeline(21, 12, G))
simple('snap_subs_to_scene', rect(2.5, 4, 19, 16, 1.5), path('M2.5 7.5h19M2.5 16.5h19', M, 1.3),
       path('M6.5 4v3.5M11 4v3.5M15.5 4v3.5M6.5 16.5V20M11 16.5V20M15.5 16.5V20', M, 1.3),
       path('M8.5 9.5H7v5h1.5', R, 1.8), path('M15.5 9.5H17v5h-1.5', B, 1.8))
simple('subend_to_video', rect(3.5, 2.5, 17, 9, 1.2), path('M16 13v3.5M13.8 14.6L16 16.8l2.2-2.2', C), timeline(20.5, 16, B))
simple('substart_to_video', rect(3.5, 2.5, 17, 9, 1.2), path('M8 13v3.5M5.8 14.6L8 16.8l2.2-2.2', C), timeline(20.5, 8, R))
simple('video_to_subend', timeline(4, 16, B), path('M16 8v3.5M13.8 9.6L16 11.8l2.2-2.2', C), rect(3.5, 13, 17, 8.5, 1.2))
simple('video_to_substart', timeline(4, 8, R), path('M8 8v3.5M5.8 9.6L8 11.8l2.2-2.2', C), rect(3.5, 13, 17, 8.5, 1.2))
simple('show_video_details_menu', g_screen(), path('M12 9.5v5'), circ(12, 7, 1.1, 'none', 0, W))
simple('use_dummy_video_menu',
       *[rect(3 + i * 2.57, 4.5, 2.57, 9, 0, 'none', 0, col) for i, col in
         enumerate(('#C8C8C8', '#E5D84A', '#4FD1D9', '#55C95A', '#D554C9', '#E25050', '#4D6FE0'))],
       rect(3, 13.5, 18, 3.5, 0, 'none', 0, '#3B3F46'), rect(2.5, 4, 19, 13.5, 1, W, 1.6),
       path('M8.5 21h7M12 17.5V21', W, 1.6))
simple('resample_toolbutton', rect(3, 3, 18, 18, 1.5, W, 1.6, dash='2.5 2'), rect(6.5, 6.5, 7, 7, 1, 'none', 0, W),
       path('M14 14l5.5 5.5M19.5 15v4.5H15', C, 2))

# Visual typesetting tools
simple('visual_standard', circ(12, 12, 6.5), path('M12 2.5v5M12 16.5v5M2.5 12h5M16.5 12h5'), circ(12, 12, 1.5, 'none', 0, R))
simple('visual_move', path('M12 3v18M3 12h18'), path('M9 6l3-3 3 3M9 18l3 3 3-3M6 9l-3 3 3 3M18 9l3 3-3 3'))
simple('visual_move_conv_move', circ(4.5, 12, 2.2, 'none', 0, R), path('M8.5 12h10M15 8.5l3.5 3.5-3.5 3.5'), circ(20.5, 12, 2.2, B, 1.6))
simple('visual_move_conv_pos', path('M3 6v12M21 6v12', M), path('M5.5 12h3.5M18.5 12H15', W), path('M7.5 10l2 2-2 2M16.5 10l-2 2 2 2'), circ(12, 12, 1.8, 'none', 0, R))
simple('visual_rotatez', path('M19 12a7 7 0 1 1-2.05-4.95'), path('M17.2 3.5v3.8h-3.8'), circ(12, 12, 1.6, 'none', 0, R))
simple('visual_rotatexy', '<ellipse cx="12" cy="12" rx="9.5" ry="4" fill="none" stroke="#EEF1F5" stroke-width="1.9" transform="rotate(-20 12 12)"/>',
       '<ellipse cx="12" cy="12" rx="4" ry="9.5" fill="none" stroke="#AEB6C1" stroke-width="1.6" transform="rotate(-20 12 12)"/>',
       circ(12, 12, 1.6, 'none', 0, R))
simple('visual_scale', rect(3, 3, 18, 18, 1.5, W, 1.6, dash='2.5 2'), rect(6, 6, 9, 9, 1, W), path('M15 15l5 5M20 15.5V20h-4.5', C, 2))
simple('visual_clip', rect(5, 5, 14, 14, 0.5, W), points((5, 5), (19, 5), (5, 19), (19, 19), s=3.4))
simple('visual_vector_clip', path('M5 17C3 10 7.5 4.5 13 5.5s7.5 7 4 11.5-10 4-12 0z'), points((5, 17), (13, 5.5), (17, 17), s=3.2))
simple('visual_vector_clip_bicubic', path('M4 19C9 19 6 5 12 5s3 14 8 14'), path('M4 19h3.5M20 19h-3.5', M, 1.2), points((4, 19), (12, 5), (20, 19), s=3.2))
simple('visual_vector_clip_convert', path('M5 18L5 6', W), path('M19 18c-4-3-4-9 0-12', W), points((5, 6), (5, 18), (19, 6), (19, 18), s=3),
       path('M8.5 12h7M10.5 10l-2 2 2 2M13.5 10l2 2-2 2', B, 1.8))
simple('visual_vector_clip_drag', path('M12 4v16M4 12h16', M, 1.6), path('M9.5 6.5L12 4l2.5 2.5M9.5 17.5L12 20l2.5-2.5M6.5 9.5L4 12l2.5 2.5M17.5 9.5L20 12l-2.5 2.5'), points((12, 12), s=3.6))
simple('visual_vector_clip_freehand', path('M2 20.5l3.5-5 3.5 3.5 3.5-5.5 3.5 3.5 4-4', W), group('translate(9,0) scale(0.62)', g_pencil(SW / 0.62, B)))
simple('visual_vector_clip_freehand_smooth', path('M2 19.5c2.5-6 5.5-6 8-3s6 3 8-1.5 2.5-3 4-2', W), group('translate(9,0) scale(0.62)', g_pencil(SW / 0.62, B)))
simple('visual_vector_clip_insert', path('M3 18c4-12 14-12 18 0'), points((3, 18), (21, 18), s=3), path('M12 6.5v6M9 9.5h6', G, 2.2))
simple('visual_vector_clip_line', path('M5 19L19 5'), points((5, 19), (19, 5), s=3.6))
simple('visual_vector_clip_remove', path('M5 19L12 8l7 11', M, 1.6), points((5, 19), (19, 19), s=3.2), path('M9 4.5l6 6M15 4.5l-6 6', R, 2.2))
simple('visual_help', path('M8.5 8.5a3.6 3.6 0 1 1 5.3 3.2c-1.2.7-1.8 1.5-1.8 2.8v.5', W, 2.2), circ(12, 19, 1.3, 'none', 0, W))

# The WsQ shift buttons are already white on blue and keep their own icons.
SKIP = {'wsq_shift_back', 'wsq_shift_forward'}


# ---------------------------------------------------------------------------

def svg(body):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">{body}</svg>')


def light_icon_names():
    names = set()
    for f in os.listdir(BUTTON_DIR):
        if not f.endswith('.png') or '_dark_' in f:
            continue
        base, size = f[:-4].rsplit('_', 1)
        if size.isdigit():
            names.add(base)
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sheet', help='also write a preview sheet of all icons to this PNG')
    args = ap.parse_args()

    light = light_icon_names()
    missing = sorted(light - set(ICONS) - SKIP)
    unknown = sorted(set(ICONS) - light)
    if unknown:
        sys.exit(f'icons drawn for names that do not exist: {unknown}')

    written = []
    for name in sorted(ICONS):
        body = svg(ICONS[name]())
        for size in SIZES:
            out = os.path.join(BUTTON_DIR, f'{name}_dark_{size}.png')
            cairosvg.svg2png(bytestring=body.encode(), write_to=out, output_width=size, output_height=size)
            written.append(f'button/{name}_dark_{size}.png')

    with open(MANIFEST) as f:
        lines = [l.rstrip('\n') for l in f if '_dark_' not in l]
    while lines and lines[-1] == '':
        lines.pop()
    with open(MANIFEST, 'w') as f:
        f.write('\n'.join(lines + [''] + written) + '\n')

    print(f'{len(ICONS)} dark icons, {len(written)} files')
    if missing:
        print('no dark variant (light icon used):', ', '.join(missing))

    if args.sheet:
        import io
        from PIL import Image, ImageDraw
        names = sorted(ICONS)
        cols = 10
        cell = 92
        rows = (len(names) + cols - 1) // cols
        sheet = Image.new('RGB', (cols * cell, rows * cell), (32, 34, 38))
        draw = ImageDraw.Draw(sheet)
        for i, name in enumerate(names):
            x, y = (i % cols) * cell, (i // cols) * cell
            big = Image.open(os.path.join(BUTTON_DIR, f'{name}_dark_48.png'))
            small = Image.open(os.path.join(BUTTON_DIR, f'{name}_dark_16.png'))
            sheet.paste(big, (x + 6, y + 6), big)
            sheet.paste(small, (x + 62, y + 22), small)
            draw.text((x + 4, y + 70), name[:15], fill=(150, 156, 166))
        sheet.save(args.sheet)


if __name__ == '__main__':
    main()
