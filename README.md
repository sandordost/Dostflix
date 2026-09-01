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
If its local API does not become ready within 20 seconds, Dostflix reports a
startup error instead of waiting indefinitely. Backend diagnostics are stored in
`$XDG_DATA_HOME/dostflix/torrserver/torrserver.log` (normally
`~/.local/share/dostflix/torrserver/torrserver.log`).
Ready TorrServer streams now open in an embedded libmpv render surface with
pause, seek, volume, fullscreen, buffering feedback, and a return-to-movie flow.
Embedded, local, and OpenSubtitles behavior remains a later phase.

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
