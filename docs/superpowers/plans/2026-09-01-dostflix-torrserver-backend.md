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
5. Replacing a release first drops the current live torrent and waits for a
   successful response before submitting the replacement. Rapid choices update
   the pending replacement behind the same barrier; they never create parallel
   swarms.
6. On VPN loss TorrServer is killed immediately while firewall protection remains.
7. Normal shutdown stops TorrServer before the guard and Dostflix-owned VPN.

Startup fails visibly after 20 seconds if the local API does not become ready;
process launch errors and exit codes include the diagnostic log path.

## Crash recovery

- TorrServer and managed Prowlarr receive a Linux parent-death signal, so an
  abnormal Dostflix exit terminates both children instead of leaving database or
  port locks behind.
- A `torrserver.pid` ownership record is written after launch and removed during
  normal shutdown. At the next launch, a daemon whose recorded Dostflix owner no
  longer exists is terminated before a replacement starts.
- For upgrades from older versions without that record, recovery accepts only a
  PID-1 orphan whose command line contains TorrServer's exact private Dostflix
  data path. Unrelated TorrServer processes are never touched.

## Streaming flow

- Acquire torrent metadata and expose plausible video files for selection.
- Start TorrServer preloading for the selected file.
- Keep the long-running preload request separate from short control API requests;
  it intentionally has no transfer timeout while status polling continues.
- Poll its native download rate, peer, seed, cache, and preload statistics.
- Retain Dostflix's 30-second initial-buffer policy using TorrServer's contiguous
  preloaded byte count.
- Expose TorrServer's loopback HTTP stream URL directly to the future mpv player;
  seeking automatically moves TorrServer's reader priority and readahead window.
- Emit the selected file's loopback source, info hash, torrent index, filename,
  and authoritative size to `DownloadManager`. That follow-up keeps a sequential
  range-resumable reader open so playback cache and durable library retention
  remain separate concerns owned by TorrServer and Dostflix respectively.
- TorrServer may retain previously selected releases as status `5` (database-only)
  entries. These have no live swarm or cache and must not be counted as active
  torrents. Dostflix permits exactly one status other than database-only.

## Verification

- Torrent traffic is rejected before VPN readiness.
- The real TorrServer binary starts in a Linux network namespace with loopback as
  its only interface.
- A local torrent fixture is uploaded, its video is selected, preloading starts,
  and a loopback-only stream URL is produced without any external traffic.
- A second local fixture replaces the first; the TorrServer list then contains
  exactly one active torrent while the first remains database-only.
- A loopback test leaves a real TorrServer process behind with a dead ownership
  marker and verifies that the next manager cleans it up and becomes ready.
