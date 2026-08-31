# Dostflix

Dostflix is a native Qt 6 movie library and streaming client for Arch Linux. The finished application will connect through a user-selected OpenVPN profile, search user-configured Prowlarr or Torznab endpoints, stream a selected torrent while it downloads, and retain the completed movie locally.

This repository does not bundle torrent indexers or content sources. Users are responsible for configuring lawful sources and for complying with applicable copyright law.

## Current status

The current application contains the native shell, Dostify-inspired visual tokens,
a responsive fixed-ratio movie grid, XDG settings, and the versioned SQLite
library schema. The VPN lifecycle increment can import and select OpenVPN profiles
through NetworkManager, connect the selected profile at startup, and disconnect it
at shutdown only when Dostflix started it.

Protected internet features remain disabled until the nftables kill switch and
network-isolation tests are complete. Provider, torrent, player, and subtitle
behavior will be added in separately tested phases.

## Arch installation and dependencies

The recommended local installation route is the Arch package. `makepkg -si`
installs Dostflix's declared runtime and build dependencies through pacman,
including NetworkManager's OpenVPN plugin, OpenVPN, nftables, Polkit, libtorrent,
mpv, Secret Service support, and Qt's Wayland platform integration.

```bash
cd packaging/arch
makepkg -si
```

For a source-only development build, install the toolchain and current direct
dependencies with:

```bash
sudo pacman -S --needed base-devel cmake hicolor-icon-theme libsecret \
  libtorrent-rasterbar mpv networkmanager networkmanager-openvpn nftables ninja \
  openvpn polkit qt6-base qt6-declarative qt6-svg qt6-tools qt6-wayland sqlite
```

## Build and test

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
cmake --build build --target dostflix_ui_qmllint
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
