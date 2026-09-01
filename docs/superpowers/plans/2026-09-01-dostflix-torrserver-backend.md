# Managed TorrServer backend plan

**Date:** 2026-09-01
**Status:** Implemented on `feature/torrent-streaming-foundation`

## Decision

Dostflix delegates torrent streaming, peer scheduling, piece selection, readahead,
and cache management to TorrServer. A live qBittorrent trial downloaded tens of
MiB across many incomplete pieces despite sequential mode, leaving no contiguous
playback buffer. TorrServer is purpose-built around an HTTP reader and explicitly
prioritizes the active playback range and readahead window.

## Lifecycle and isolation

1. Dostflix establishes and verifies its VPN and process-scoped kill switch.
2. `TorrServerManager` starts `/usr/bin/torrserver` as a child in the same
   protected systemd scope.
3. Its HTTP API binds to `127.0.0.1` on a random port and uses a private database
   below `$XDG_DATA_HOME/dostflix/torrserver/`.
   Process diagnostics are persisted to `torrserver.log` in that directory.
4. Magnets and torrent files are submitted only while VPN protection is ready.
5. On VPN loss TorrServer is killed immediately while firewall protection remains.
6. Normal shutdown stops TorrServer before the guard and Dostflix-owned VPN.

Startup fails visibly after 20 seconds if the local API does not become ready;
process launch errors and exit codes include the diagnostic log path.

## Streaming flow

- Acquire torrent metadata and expose plausible video files for selection.
- Start TorrServer preloading for the selected file.
- Poll its native download rate, peer, seed, cache, and preload statistics.
- Retain Dostflix's 30-second initial-buffer policy using TorrServer's contiguous
  preloaded byte count.
- Expose TorrServer's loopback HTTP stream URL directly to the future mpv player;
  seeking automatically moves TorrServer's reader priority and readahead window.

## Verification

- Torrent traffic is rejected before VPN readiness.
- The real TorrServer binary starts in a Linux network namespace with loopback as
  its only interface.
- A local torrent fixture is uploaded, its video is selected, preloading starts,
  and a loopback-only stream URL is produced without any external traffic.
