# Dostflix VPN Lifecycle Plan

**Date:** 2026-08-31

**Scope:** Safe profile management and application-owned VPN lifecycle. Protected internet features remain disabled until the separate privileged kill-switch increment has landed and passed network-isolation tests.

## Architecture

- `NetworkManagerBackend` is the operating-system boundary. It uses fixed-argument `nmcli` calls for OpenVPN import/profile discovery and NetworkManager's D-Bus API for activation and deactivation.
- `VpnManager` owns the UI-facing state machine, selected profile UUID, connection ownership, refresh polling, startup activation, and shutdown cleanup.
- `AppSettings` persists the NetworkManager UUID plus a non-secret ownership
  marker containing the owning Dostflix PID. Dostflix never copies the `.ovpn`
  file or private keys into its own data directories.
- `VpnProfileModel` exposes suitable NetworkManager VPN profiles to QML.
- The Settings page imports, selects, connects, and disconnects profiles. The header reflects real state.

## Safety invariants

1. Import accepts only an existing local `.ovpn` file and invokes no shell.
2. All external networking remains disabled in this increment, even when the VPN reports connected.
3. Dostflix disconnects on exit only when it activated that connection during
   the current process, or when a persisted UUID/PID marker proves that a crashed
   Dostflix instance activated it.
4. A VPN that was already active before startup is observed but never claimed or disconnected.
5. Errors shown to the user contain no profile file contents or secrets.
6. A genuinely external active VPN has no matching dead-owner marker and remains
   observed but unclaimed.

## Implementation order

1. Add backend result types and deterministic unit tests for profile parsing.
2. Add the `VpnManager` state machine with a fake backend covering ownership and shutdown.
3. Add the profile list model and persisted selection.
4. Wire startup/shutdown and expose the manager to QML.
5. Replace the Settings placeholder and bind the header to live status.
6. Update Arch dependencies and documentation.
7. Run unit tests, QML tests, qmllint, and a headless startup smoke test.

## Deferred to the next security increment

- Endpoint parsing and bootstrap DNS.
- Polkit-authorized, narrowly scoped nftables helper.
- Route, DNS, and socket-binding verification.
- Network namespace leak tests.
- Enabling provider, metadata, subtitle, torrent, or seeding traffic.
