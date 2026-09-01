# Dostflix UI overhaul

Date: 2026-09-01
Status: implemented

## Goal

Bring the native Qt Quick interface in line with the Dostify reference without
copying controls that have no Dostflix function. The result uses a black canvas,
matte translucent anthracite windows, strong dark-on-light control contrast,
and a restrained purple accent. Borders are reserved for keyboard focus.

The Dostify source under `work/Dostify/dostify` remains reference material only.
Dostflix does not depend on or package that project.

## Visual system

`qml/theme/Theme.qml` is the single source for colors, spacing, radii, typography,
poster geometry, and motion durations. The application uses packaged Montserrat
and Font Awesome rather than compositor-dependent control fonts and theme icons.
Headings use a small 12/14/22/28 pixel scale; icons use 20 or 28 pixels. Movie
posters remain exactly 170 pixels wide with a 2:3 aspect ratio.

Reusable `AppTextField`, `AppComboBox`, `AppButton`, `AppToolButton`, and
`AppSpinBox` controls own their rendering. Inputs are borderless and rounded;
hover, pressed, focus, popup, and disabled states stay within the dark palette so
foreground text cannot become unreadable under a desktop theme. Button content
is centered as one unit by default; the navigation rail explicitly opts into
left alignment. `PathPickerDialog` provides consistent in-app file and folder
selection because native portal dialogs cannot be styled reliably across Linux
compositors.

Navigation collapses from a 252-pixel text sidebar to a 76-pixel icon rail below
980 pixels. The content inset also becomes smaller below 900 pixels. Search stays
available from a popup in compact mode. Result and library grids derive their
column count from the available width and center every fixed-size card in its
cell. Discover also offers a compact list mode. Release selection starts directly
without a redundant confirmation step, and active torrent progress is shown in
the global header. The local library deliberately uses 132-pixel rows with a
left-hand 2:3 poster and no synopsis tooltip.

## Motion and render performance

Interactive motion is limited to GPU-friendly opacity, color, and scale changes.
No layout, width, height, anchor, blur, or background-image animation is used.
Durations are 120 ms for direct feedback and 180 ms for larger transitions.
Poster decoding is asynchronous, cached, and constrained to a HiDPI-aware source
size. Grid views clip content and stop at their bounds.

These constraints are intentional. Future UI changes must not add layout
animations or unbounded image decoding without measuring frame pacing first.

## Player contract

The embedded mpv surface stays unobscured. A matte title bar and control deck
appear over it and fade after 2.8 seconds while playback is running. Moving the
pointer, using a playback shortcut, buffering, pausing, or returning to the
player reveals the controls again. Keeping the pointer over either control bar
keeps it visible. The cursor hides together with the controls.

Keyboard controls:

- `Space`: play or pause.
- `Left` / `Right`: seek ten seconds.
- `F`: toggle fullscreen.
- `Escape`: return to browsing while playback continues.
- `Ctrl+1` through `Ctrl+4`: Discover, Library, Downloads, and Settings.

The Stop action remains distinct from Browse: Stop terminates mpv once and then
returns to browsing. Browsing has no permanent player bar; the compact Now
Playing card is shown only for an active playback session.

## Accessibility

Navigation and player actions expose accessible names, compact navigation shows
tooltips, movie cards expose button semantics, controls retain tab focus, and
focus is the only routine use of a bright outline. Small-window behavior hides
secondary subtitle-delay and volume controls before primary playback controls.

## Verification

The QML suite covers fixed poster geometry, responsive result wrapping, the
Discover view toggle and direct release action, centered controls, the in-app
picker, player control existence, Stop semantics, automatic control hiding and
reveal, Now Playing visibility, library row/resume behavior, and stable
theme/performance tokens.
The normal build, `dostflix_ui_qmllint`, and complete CTest suite must pass before
merging. Do not launch the production application for UI-only validation because
startup intentionally manages the VPN and network guard.

## Follow-up

Before the first release, capture reference screenshots at 780x520, 1280x800,
and a HiDPI scale factor on KDE, GNOME, and one wlroots compositor. Treat those
as visual regression fixtures after the layout has received a final human pass.
