# Dostflix

Dostflix is a native Qt 6 movie library and streaming client for Arch Linux. The finished application will connect through a user-selected OpenVPN profile, search user-configured Prowlarr or Torznab endpoints, stream a selected torrent while it downloads, and retain the completed movie locally.

This repository does not bundle torrent indexers or content sources. Users are responsible for configuring lawful sources and for complying with applicable copyright law.

## Current status

The current application contains the native shell, library foundation, OpenVPN
profile management, provider search, a managed TorrServer streaming backend,
and a process-scoped nftables kill switch. The installed
application registers itself in a dedicated systemd cgroup; Polkit-authorized rules
block that scope from the clear interface before VPN activation and during tunnel
loss. `networkReady` becomes true only after NetworkManager, the tunnel interface,
the default route, and protected firewall state have all been verified.

TorrServer is started only after `networkReady`, listens for control requests on
loopback, inherits Dostflix's protected cgroup, and is stopped before the VPN.
When another release is selected, Dostflix waits for TorrServer to drop the
current live swarm before submitting its replacement, guaranteeing at most one
active torrent while still allowing database-only history entries.
If its local API does not become ready within 20 seconds, Dostflix reports a
startup error instead of waiting indefinitely. Backend diagnostics are stored in
`$XDG_DATA_HOME/dostflix/torrserver/torrserver.log` (normally
`~/.local/share/dostflix/torrserver/torrserver.log`).
Ready TorrServer streams now open in an embedded libmpv render surface with
pause, seek, volume, fullscreen, buffering feedback, and a return-to-movie flow.
The player lists embedded subtitle tracks, accepts local `.srt`, `.ass`, and
`.vtt` files, and supports subtitle delay. Its final subtitle-menu action can
search Dutch and English releases through a user-configured OpenSubtitles.com
account, download the selected subtitle, and load it directly into mpv. These
requests are disabled until VPN protection is verified.
Preferred subtitle languages are configurable as ordered ISO codes. For a local
library film, searches also include its IMDb ID and OpenSubtitles movie hash;
the selected subtitle is atomically stored beside the video as
`Movie.language.srt` so it remains available with the retained film.
Users can choose a writable movie-library folder; Dostflix recursively discovers
supported local videos, registers them in SQLite without duplicates, and plays
them through embedded mpv even when the VPN is unavailable. The selected
TorrServer video is simultaneously retained as a resumable `.dostflix.part`
file. Its exact transfer identity and progress survive restarts; only an exact,
fsynced, atomically finalized video becomes visible in the Library. Downloads
can be played or removed directly, and selecting the same release again reuses
its existing torrent cache and partial-file progress.
Before a new or resumed writer starts, Dostflix verifies that the library
filesystem can hold every remaining byte while retaining a 512 MiB safety
margin. Insufficient space pauses the transfer before any request is made;
Downloads shows the `.dostflix.part` filename, bytes remaining, free space, and
a recovery message instead of failing silently mid-transfer.
Local filenames are normalized into a title and year before registration. When
a TMDB Read Access Token is configured, Dostflix enriches unmatched local films
only while VPN protection is verified, then stores the canonical title, year,
runtime, synopsis, TMDB/IMDb identity, and a locally cached poster. Once cached,
all library metadata remains available for offline browsing.
Local playback position is persisted in five-second increments and once more
before stopping. Returning to a partially watched movie resumes immediately;
completed movies automatically lose their resume marker.

The UI follows the Dostify reference with matte translucent panels, fixed 2:3
poster cards, responsive icon-only navigation on small windows, consistent
Montserrat typography, and Font Awesome icons. Borderless rounded inputs and
dark popup/hover states avoid the platform-dependent Qt Basic styling. File and
folder selection uses the same in-app visual system instead of compositor-themed
dialogs. Discover can switch between a poster grid and compact result list. A
selected release shows its retrieval/buffering spinner and concise status on its
own thumbnail instead of consuming the application header. The local
library uses compact poster rows without hover tooltips. Player controls fade
after 2.8 seconds and return on pointer
movement, pause, buffering, or keyboard input; only opacity, color, and scale are
animated to keep frame pacing smooth.

In a Gamescope/Steam session Dostflix detects `STEAM_GAMESCOPE_SESSION`,
`GAMESCOPE_WAYLAND_DISPLAY`, or a Gamescope desktop-session name and opens on
the complete output in native fullscreen. Its window dimensions remain bound to
the active `Screen`, so a 3840×2160 session is not restricted by the normal
1280×800 desktop startup size. The 1280-pixel reference width also drives the
complete interface scale: `screen width / 1280`. A 3840-pixel-wide Gamescope
output therefore renders typography, icons, controls, posters, and spacing at
3× while preserving the reference layout. The binding updates automatically
when the output width changes. `dostflix --fullscreen` and
`dostflix --windowed` provide explicit launch-option overrides.

Controller input is handled through SDL3's standardized gamepad API. Xbox,
PlayStation, Nintendo Switch Pro, Steam Deck, Steam Controller, Steam Input's
virtual Xbox controller, and other SDL-mapped gamepads share one action layout.
Both the D-pad and left stick navigate spatially with dead-zone filtering and
hold-repeat: up/down follows visual rows and left/right changes columns. The
south face button explicitly activates the focused Qt control, east closes the
active context or returns to an active movie, Start pauses, triggers or shoulders
change section or seek 30 seconds, and west opens subtitles. D-pad/stick
left/right remain spatial navigation inside the player. North focuses search
while browsing and toggles player fullscreen.
Controller-only hints adapt between Xbox and PlayStation labels.
Controllers can be connected or removed while Dostflix is running. Losing the
last controller during playback pauses the movie, while mouse and keyboard input
remain usable at all times. Search stays outside the directional focus graph and
is entered with Y/Triangle; one B/Circle press leaves it and restores the prior
selection or focuses the first displayed result. Settings shows the controller.
Settings uses explicit focus rows: left/right stays within one visual row, while
up/down moves to the first available control of the previous or next row. Open
select boxes trap controller navigation on their options until A/Cross confirms
or B/Circle closes them; empty select boxes do not open.

## Arch installation and dependencies

The recommended local installation route is the Arch package. `makepkg -si`
installs Dostflix's declared runtime and build dependencies through pacman,
including NetworkManager's OpenVPN plugin, OpenVPN, nftables, Polkit, TorrServer,
mpv, SDL3 controller support, Secret Service support, and Qt's Wayland platform
integration.

```bash
cd packaging/arch
makepkg -si
```

For a source-only development build, install the toolchain and current direct
dependencies with:

```bash
sudo pacman -S --needed base-devel cmake hicolor-icon-theme libsecret \
  mpv networkmanager networkmanager-openvpn nftables ninja otf-font-awesome \
  openvpn polkit qt6-base qt6-declarative qt6-svg qt6-tools qt6-wayland sqlite \
  ttf-montserrat
yay -S --needed prowlarr-bin torrserver-bin
```

## Build and test

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
cmake --build build --target dostflix_ui_qmllint
```

The privileged isolation test creates temporary network namespaces and virtual
interfaces, then verifies bootstrap, protected, and removal behavior:

```bash
cmake --build build --target network_isolation_test
```

Run the application with:

```bash
./build/dostflix
```

## Local package

To rebuild the local package from `packaging/arch`:

```bash
makepkg -si
```

The package installs the `dostflix` executable, desktop entry, and scalable application icon.

## Design and implementation notes

Agents and contributors should start with the following documents before
changing cross-component behavior:

- `docs/superpowers/specs/2026-08-31-dostflix-design.md` — product architecture,
  security boundaries, UX requirements, and definition of done.
- `docs/superpowers/plans/2026-09-01-dostflix-torrserver-backend.md` — managed
  torrent backend, buffering, diagnostics, and loopback streaming contract.
- `docs/superpowers/plans/2026-09-01-dostflix-mpv-player.md` — current native
  player implementation, lifecycle invariants, verification, and the next
  subtitle increment.
- `docs/superpowers/plans/2026-09-01-dostflix-subtitle-controls.md` — embedded
  and local subtitle implementation and its networked handoff boundary.
- `docs/superpowers/plans/2026-09-01-dostflix-opensubtitles.md` — OpenSubtitles
  API, language preferences, hash/IMDb matching, sidecar storage, VPN gating,
  and fake-server tests.
- `docs/superpowers/plans/2026-09-01-dostflix-local-library.md` — local folder,
  SQLite registration, offline playback, tests, and torrent-retention handoff.
- `docs/superpowers/plans/2026-09-01-dostflix-library-metadata.md` — filename
  recognition, TMDB matching, poster caching, schema v4, and network invariants.
- `docs/superpowers/plans/2026-09-01-dostflix-watch-progress.md` — local playback
  persistence, resume/start-over behavior, completion rules, and tests.
- `docs/superpowers/plans/2026-09-01-dostflix-durable-retention.md` — resumable
  loopback writer, playback/removal lifecycle, cache reuse, atomic completion,
  and security rules.
- `docs/superpowers/plans/2026-09-01-dostflix-download-disk-safety.md` — free-space
  preflight, safety margin, disk-full handling, incomplete-file UX, and tests.
- `docs/superpowers/plans/2026-09-01-dostflix-ui-overhaul.md` — Dostify-inspired
  visual tokens, responsive layout, animation performance rules, player auto-hide,
  keyboard controls, accessibility, and visual-regression follow-up.
- `docs/superpowers/plans/2026-09-02-dostflix-controller-support.md` — SDL3
  controller coverage, Steam Input behavior, action mapping, focus, hot-plugging,
  and verification.
- `docs/superpowers/plans/2026-08-31-dostflix-network-guard.md` — kill-switch and
  process-isolation rules that networking changes must preserve.

The plan documents describe completed increments as well as the remaining work;
do not infer that unfinished features in the design specification already exist.
