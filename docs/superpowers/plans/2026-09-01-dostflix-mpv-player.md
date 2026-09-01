# Embedded mpv player plan

**Status:** Implemented in PR #6 on `feature/mpv-player`

## Scope

- Render the selected TorrServer loopback stream through libmpv's OpenGL render API.
- Open the dedicated player automatically after the safe buffer becomes ready.
- Provide pause/resume, relative and absolute seeking, volume, fullscreen, stop,
  current time, duration, buffering state, and visible playback errors.
- Keep playback active while browsing and expose the existing **Return to movie** card.
- Stop mpv before application shutdown so TorrServer and VPN teardown remain ordered.

Subtitle track enumeration, local subtitle attachment, and **Find subtitles…**
are implemented by the follow-up subtitle-controls increment.

## Implementation map

- `src/player/MpvPlayer.{h,cpp}` owns the `mpv_handle`, exposes the playback
  state to QML, and renders video through libmpv's OpenGL render API in a
  `QQuickFramebufferObject` renderer.
- `qml/Main.qml` keeps one player instance alive for the window lifetime. It
  starts a URL only after `TorrServerManager::bufferReady`, prevents duplicate
  loads with `launchedStreamUrl`, and switches between the player and browsing.
- `qml/pages/PlayerPage.qml` provides the overlay controls and delegates all
  playback state and commands to `MpvPlayer`.
- `src/main.cpp` forces Qt Quick's OpenGL backend and stops mpv before
  TorrServer, Prowlarr, and VPN shutdown.
- `tests/player/tst_mpv_player.cpp` covers the controller contract and an actual
  libmpv render/load cycle using a locally generated video.
- `tests/qml/tst_player_page.qml` covers the QML player contract without remote
  media.

## Lifecycle and security invariants

- Never load the TorrServer URL before `bufferReady` is true. Provider, torrent,
  and playback networking remains gated by the existing VPN/kill-switch state.
- The player consumes only the loopback TorrServer URL; it must not resolve or
  fetch torrent sources itself.
- Keep the `MpvSharedState` ownership boundary intact. The scene-graph renderer
  can outlive the QML item during window teardown, so the mpv handle must remain
  alive until the render context has been freed on the render thread.
- Keep shutdown ordered as: mpv, TorrServer, managed Prowlarr, VPN. Do not run
  the full application or manipulate VPN processes from automated tests.
- Qt Quick must use OpenGL because libmpv is currently integrated through
  `MPV_RENDER_API_TYPE_OPENGL`.

## Follow-up: subtitles

The subtitle-controls increment implements the local portions of this contract
without replacing the player or weakening network gating:

1. Observe mpv's track list and expose embedded subtitle tracks plus a disabled
   state to QML.
2. Add local `.srt`, `.ass`, and `.vtt` attachment through mpv's `sub-add`
   command.
3. Add subtitle selection and delay controls. The final menu item must be
   **Find subtitles…**.
4. Keep embedded/local subtitles fully usable without credentials or network.
5. The remaining OpenSubtitles search must stay behind the existing
   `networkReady` boundary and
   store credentials in the desktop Secret Service, never in settings or logs.
6. Test the controller with synthetic mpv events and the menu with a fake QML
   player. Any external API test must use a fake local endpoint.

Acceptance criteria and the broader intended behavior are defined in
`docs/superpowers/specs/2026-08-31-dostflix-design.md`, especially the Player,
Subtitles, Network-loss behavior, Testing, and Definition of done sections.
Implementation and handoff details are in
`docs/superpowers/plans/2026-09-01-dostflix-subtitle-controls.md`.

## Verification

- Build and QML lint the complete application against the installed libmpv.
- Unit-test controller defaults and bounded volume changes without remote media.
- Exercise the player page with a fake controller in Qt Quick tests.
- Keep torrent integration tests inside their isolated network namespace.

Commands used for this increment:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cmake --build build --target all_qmllint
ctest --test-dir build --output-on-failure

# Real render-path smoke test; generates/uses local media only.
QT_QPA_PLATFORM=wayland \
  ./build/tests/tst_mpv_player initializesOpenGlRenderContext

cd packaging/arch
makepkg -f --noconfirm
```

At handoff, the complete test suite passed 19/19, the Wayland/OpenGL render-path
test passed, and the Arch package built and installed as `dostflix-git 0.1.0-23`.
