# Durable torrent retention plan

**Status:** Implemented; lifecycle controls completed on `feature/download-lifecycle`

## Scope

- Save the selected TorrServer video sequentially into the configured movie
  library while loopback playback remains available.
- Preserve partial progress and the exact torrent/file identity in SQLite.
- Resume after pause, VPN loss, normal shutdown, application restart, or a
  process interruption without starting again at byte zero.
- Atomically expose only complete, size-verified videos to the local Library.

## Architecture

- `DownloadManager` accepts only an `http://` loopback source emitted by the
  managed `TorrServerManager`. It never accepts a provider URL or performs peer
  traffic itself.
- Data is appended to `<video>.dostflix.part`. A resumed request sends
  `Range: bytes=<current-size>-` and requires HTTP 206 with a matching
  `Content-Range` start before writing any response bytes.
- `LibraryDatabase` schema version 3 adds a `transfers` table keyed by torrent
  hash and file index. It persists title, selected filename, expected size,
  partial/final paths, actual bytes, state, and update time.
- `TorrServerManager` can reopen a database-saved torrent by info hash and file
  index after its private daemon is ready. Its `/stream` request reloads the
  TorrServer database entry and restores peer acquisition behind the VPN.
- `DownloadsPage.qml` exposes the current/resumable transfer, byte progress,
  pause, resume, playback, and confirmed removal. Completion triggers a
  `LibraryManager` refresh.
- Selecting the same magnet in Discover compares its normalized BTIH info hash
  with the stored transfer. A match restores the existing TorrServer torrent
  and piece cache instead of submitting a duplicate or discarding the buffer.
- Completed downloads play directly from the final local file. Partial downloads
  ask TorrServer to reopen the stored hash/file index, so playback keeps its
  existing cache and remains behind VPN protection.

The HTTP behavior follows TorrServer's official `server/web/api/stream.go` and
`server/torr/stream.go`: the stream handler delegates to Go `http.ServeContent`
and advertises byte-range support for range requests.

## Durability and security invariants

- No writer or restore request starts until `VpnManager::networkReady` is true.
  VPN loss aborts the loopback reply, fsyncs and closes the partial file, records
  `paused`, and leaves the process-scoped firewall rules authoritative.
- Source URLs must be plain HTTP on a numeric loopback address. Remote URLs are
  rejected even if passed internally by mistake.
- Untrusted torrent paths are reduced to a basename. Partial and final paths
  must remain under the selected library root and may not be symbolic links.
- Progress is reconciled from the real partial-file size rather than trusting a
  potentially stale database counter.
- Completion requires exactly the selected torrent file's expected byte count.
  The partial file is flushed and fsynced, renamed within the same directory,
  and the directory is fsynced before SQLite is marked completed.
- A crash after the rename but before the database update, or after the final
  byte but before rename, is reconciled locally at the next start without VPN.
- Selecting another torrent pauses the previous sequential writer before the
  single-active-torrent replacement proceeds. Its partial state remains stored.
- Cancelling streaming aborts all in-flight status/preload replies and clears the
  Discover state without deleting the cached torrent or durable partial file.
- Stopping mpv suppresses automatic reopening of that exact stream URL despite
  continued TorrServer status polling; an explicit Play action clears the block.
- Removing a download is an explicit confirmed action. It pauses the writer,
  validates both paths, removes partial/final data and SQLite history, refreshes
  the library, and asks a running TorrServer daemon to drop its cached torrent.

## Automated verification

- Full local transfer creates only the final video and registers it once.
- A persisted ten-byte partial resumes with `Range: bytes=10-` and reconstructs
  byte-identical content.
- A fully written partial is finalized after restart without any network.
- Transfer records round-trip through schema version 3.
- The real managed TorrServer fixture emits the loopback retention source while
  remaining isolated in a loopback-only network namespace.
- QML tests cover resume, play, and confirmed removal. C++ tests cover completed
  local playback, matching-magnet cache reuse, file/history deletion, and stable
  cancellation state.

No public torrent, provider, VPN, or metadata service is contacted by tests.

## Remaining download work

- Present all paused/completed transfer records instead of only the most recent
  resumable transfer.
- Add free-space forecasting and an explicit library-location recovery flow for
  disk-full failures.
- Persist and display transfer speed independently of TorrServer's peer rate.
