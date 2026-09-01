# Download disk-space safety plan

**Status:** Implemented on `feature/download-disk-safety`

## Scope

- Reject a new or resumed durable writer before network I/O when the configured
  movie-library filesystem cannot safely contain the remaining video bytes.
- Preserve the exact partial transfer so freeing space and pressing **Resume**
  continues from the existing byte offset.
- Make incomplete files and the reason for a pause visible in Downloads.
- Turn write-time out-of-space failures into a clear, recoverable message.

## Policy and implementation

- `DownloadManager::refreshDiskSpace()` uses `QStorageInfo::bytesAvailable()`
  for the filesystem containing the authoritative `.dostflix.part` path.
- A writer may start only when `available >= remaining` and at least 512 MiB
  remains after the expected final byte. The fixed reserve protects SQLite,
  desktop services, and normal system operation on a shared filesystem.
- The preflight runs for both `beginTransfer()` and `resume()`, and runs again
  immediately before opening the partial file. It is based on the real partial
  size, not the possibly stale SQLite counter.
- Failure stores state `paused`, keeps the partial file and torrent identity,
  sets **Waiting for disk space**, and never submits the loopback GET request.
  Pressing **Resume** reruns the check, so no restart or provider search is
  required after space has been freed.
- A later filesystem race can still exhaust the disk. A `QFile` resource error
  is therefore reported as an explicit full-library error and follows the same
  paused, resumable path. Available space is also refreshed after writes; the
  writer pauses if another process consumes the safety margin.

## UI contract

While a transfer is incomplete, Downloads displays:

- the `.dostflix.part` basename;
- remaining GiB and currently available GiB;
- red status text when the preflight is not satisfied;
- the detailed recovery error and the existing **Resume** action.

Completed movies retain the existing atomic rename and no longer show an
incomplete-file label.

## Security and durability invariants

- The check does not weaken VPN gating: it occurs before the existing
  loopback-only request and never starts TorrServer or external networking.
- Disk paths remain confined to the configured library and symbolic links
  remain rejected.
- No placeholder or final movie is exposed on preflight failure. Only an
  existing `.dostflix.part` file may remain visible on disk.
- Disk-space values are advisory UI state; exact byte count and finalization
  rules remain authoritative.

## Verification

- A deterministic oversized-transfer test derives its limit from the temporary
  filesystem, confirms zero HTTP requests, checks paused SQLite state, and
  verifies that no partial file was created.
- Existing complete, byte-range resume, offline finalization, playback, cache
  reuse, and removal tests remain green.
- QML tests verify that an incomplete basename is visible.
- The full suite and Arch package check use only local fixtures and do not start
  a public torrent, VPN, or provider request.
