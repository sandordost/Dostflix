# Subtitle controls plan

**Status:** Implemented in PR #7; OpenSubtitles follow-up implemented on
`feature/opensubtitles-search`

## Scope

- Enumerate embedded and external subtitle tracks reported by libmpv.
- Select a track or disable subtitles without restarting playback.
- Attach local `.srt`, `.ass`, and `.vtt` files and select them immediately.
- Adjust subtitle delay from -60 to +60 seconds.
- Keep **Find subtitles…** as the final subtitle menu action while clearly
  identifying OpenSubtitles search as the next networked increment.

## Implementation map

- `src/player/MpvPlayer.{h,cpp}` observes mpv's `track-list` and `sub-delay`
  properties. It exposes a QML-friendly track list and uses `sid`, `sub-add`,
  and `sub-delay` for player control.
- `qml/pages/PlayerPage.qml` owns the subtitle menu, local file dialog, and
  delay control. The menu order is: disabled, available tracks, local file,
  then **Find subtitles…**.
- `qml/Main.qml` now opens the network-gated OpenSubtitles search dialog for the
  final search action.
- `tests/player/tst_mpv_player.cpp` validates defaults, delay bounds, extension
  filtering, and a real local `.srt` load through libmpv.
- `tests/qml/tst_player_page.qml` validates that the player exposes all subtitle
  entry points.

## Invariants

- Embedded and local subtitles must work offline and must never depend on VPN,
  Prowlarr, TorrServer API access, or OpenSubtitles credentials.
- Only local files with `.srt`, `.ass`, or `.vtt` extensions are accepted by the
  file attachment action. Paths are passed directly to mpv and are not logged.
- Track IDs are opaque strings at the QML boundary even though mpv commonly
  reports numeric IDs. Do not use list indices as IDs.
- Track state is sourced from mpv's `track-list`; QML must not maintain a second
  authoritative track model.
- The last subtitle-menu action remains **Find subtitles…**.
- Preserve the player ownership and shutdown invariants documented in
  `2026-09-01-dostflix-mpv-player.md`.

## OpenSubtitles follow-up

The networked search is implemented as a separate service with these boundaries:

1. Add optional OpenSubtitles credentials/settings, storing secrets through the
   existing desktop Secret Service abstraction.
2. Enable search only when `networkReady` is true; never bypass the process
   kill-switch or contact the service for embedded/local subtitle operations.
3. Prefer media hash plus movie identifiers, then present language and release
   matches before downloading anything.
4. Save the chosen subtitle beside the retained movie when possible, otherwise
   under Dostflix application data, and load it through `addSubtitleFile()`.
5. Use a fake local HTTP endpoint in automated tests. Do not call the public API
   from CI or unit tests.

See `2026-09-01-dostflix-opensubtitles.md` for its API contract, security
invariants, tests, and remaining metadata improvements.

## Verification

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cmake --build build --target all_qmllint
ctest --test-dir build --output-on-failure

# Local video and subtitle fixture only; no external networking.
QT_QPA_PLATFORM=wayland \
  ./build/tests/tst_mpv_player initializesOpenGlRenderContext
```

At the local-controls implementation time the full suite passed 19/19, and the real Wayland/OpenGL
test loaded and selected a generated local `.srt` track successfully.
