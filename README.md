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
Users can choose a writable movie-library folder; Dostflix recursively discovers
supported local videos, registers them in SQLite without duplicates, and plays
them through embedded mpv even when the VPN is unavailable. Copying completed
TorrServer data into that folder remains the next persistence increment.

## Arch installation and dependencies

The recommended local installation route is the Arch package. `makepkg -si`
installs Dostflix's declared runtime and build dependencies through pacman,
including NetworkManager's OpenVPN plugin, OpenVPN, nftables, Polkit, TorrServer,
mpv, Secret Service support, and Qt's Wayland platform integration.

```bash
cd packaging/arch
makepkg -si
```

For a source-only development build, install the toolchain and current direct
dependencies with:

```bash
sudo pacman -S --needed base-devel cmake hicolor-icon-theme libsecret \
  mpv networkmanager networkmanager-openvpn nftables ninja \
  openvpn polkit qt6-base qt6-declarative qt6-svg qt6-tools qt6-wayland sqlite
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
  API, secret-storage, VPN gating, fake-server tests, and future enhancements.
- `docs/superpowers/plans/2026-09-01-dostflix-local-library.md` — local folder,
  SQLite registration, offline playback, tests, and torrent-retention handoff.
- `docs/superpowers/plans/2026-08-31-dostflix-network-guard.md` — kill-switch and
  process-isolation rules that networking changes must preserve.

The plan documents describe completed increments as well as the remaining work;
do not infer that unfinished features in the design specification already exist.
