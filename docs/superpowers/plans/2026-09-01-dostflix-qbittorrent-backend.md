# Managed qBittorrent backend plan

**Date:** 2026-09-01
**Status:** Implemented on `feature/torrent-streaming-foundation`

## Decision

Dostflix delegates torrent protocol, peer selection, tracker behavior, disk I/O,
queueing, file priority, and fast-resume state to `qbittorrent-nox`. Direct
ownership of a libtorrent session was removed after real swarms exposed fragile
priority and scheduling behavior in the application layer.

The C++/Qt application remains responsible for VPN gating, process ownership,
release confirmation, video selection, buffer policy, loopback HTTP streaming,
player integration, and the user interface.

## Lifecycle and isolation

1. Dostflix establishes and verifies its VPN and process-scoped kill switch.
2. `QBitTorrentManager` starts `qbittorrent-nox` as a child in the same protected
   systemd scope.
3. Its Web API binds to `127.0.0.1` on a random port and uses a dedicated profile
   below `$XDG_DATA_HOME/dostflix/qbittorrent/`.
4. Previously persisted jobs are stopped at startup. Selecting the same release
   reattaches to its info hash and resumes it instead of submitting a duplicate.
5. On VPN loss the child is killed immediately while firewall protection remains.
6. Normal shutdown gives qBittorrent time to persist fast-resume state before the
   guard and Dostflix-owned VPN are removed.

## Download flow

- Add magnets running so peer metadata can be acquired; add complete `.torrent`
  payloads stopped.
- Identify plausible video files after metadata arrives.
- Give the selected video maximal priority and all other files priority zero.
- Enable qBittorrent sequential download and first/last-piece priority.
- Poll torrent statistics, file metadata, properties, and piece states through
  the loopback API.
- Derive contiguous buffered bytes from completed qBittorrent pieces and retain
  the existing 30-second initial-buffer policy.

## Verification

- Unit tests prove that torrent work is rejected before VPN readiness.
- A magnet without available peers remains in metadata acquisition instead of
  treating qBittorrent's temporary zero piece size as a fatal error.
- A restart integration test verifies that a persisted torrent is reattached and
  resumed without a duplicate-torrent conflict.
- The managed-daemon integration test runs in a fresh Linux network namespace
  with loopback as its only interface, starts qBittorrent, submits a local torrent
  fixture, discovers its video file, and shuts down cleanly.
- Stream-server tests continue to verify tokenized loopback access, HTTP byte
  ranges, and waiting for completed pieces.
