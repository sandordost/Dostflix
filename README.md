# Dostflix

Dostflix is a native Qt 6 movie library and streaming client for Arch Linux. The finished application will connect through a user-selected OpenVPN profile, search user-configured Prowlarr or Torznab endpoints, stream a selected torrent while it downloads, and retain the completed movie locally.

This repository does not bundle torrent indexers or content sources. Users are responsible for configuring lawful sources and for complying with applicable copyright law.

## Foundation status

The current foundation release contains the native application shell, Dostify-inspired visual tokens, a responsive fixed-ratio movie grid, XDG settings paths, and the versioned SQLite library schema. It deliberately performs no network requests. VPN, provider, torrent, player, and subtitle behavior will be added in separately tested phases.

## Arch prerequisites

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-svg qt6-tools sqlite
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

From `packaging/arch`:

```bash
makepkg -si
```

The package installs the `dostflix` executable, desktop entry, and scalable application icon.
