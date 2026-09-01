# Local library and offline playback plan

**Status:** Implemented on `feature/local-library`

## Scope

- Let the user select a writable local movie-library folder.
- Recursively discover supported video files and register them idempotently in
  the existing SQLite library database.
- Display registered, currently available files in the native Library page.
- Play a selected local file through the embedded mpv player without requiring
  VPN, provider, metadata, or torrent networking.

Durable transfer completion and resume are implemented by the follow-up in
`2026-09-01-dostflix-durable-retention.md`.
Metadata enrichment is implemented by the follow-up in
`2026-09-01-dostflix-library-metadata.md`.

## Implementation map

- `LibraryDatabase` migrates to schema version 2, makes `video_path` unique,
  and exposes typed list/upsert operations.
- `LibraryManager` owns the selected directory, scan, database synchronization,
  validation, and local playback requests.
- `LocalLibraryModel` exposes stable QML roles without reusing provider-release
  semantics.
- `SettingsPage.qml` provides folder selection and manual rescan actions.
- `LibraryPage.qml` uses a fixed-size responsive grid and an explicit play
  action. `Main.qml` routes that action directly to the existing mpv player.
- `tests/library` validates migration, idempotent registration, recursive scan,
  extension filtering, settings persistence, invalid remote-folder rejection,
  and playback dispatch. A QML test covers the page action.

## Invariants

- Local playback remains usable when VPN protection is unavailable. Scanning
  performs no network operation.
- Only `.mkv`, `.mp4`, `.webm`, `.avi`, `.mov`, `.m4v`, and `.ts` files are
  registered by the scanner.
- Database identity is the canonical video path. Repeated scans update the
  existing row instead of duplicating it.
- The visible model includes only existing files whose canonical path is below
  the currently selected library root. Stale database rows are not presented.
- A folder must be local, creatable, and writable before it replaces the saved
  setting.

## Durable torrent retention follow-up

The follow-up now:

1. Streams the selected TorrServer file into a resumable partial file below the
   selected library directory while playback continues over loopback.
2. Persists torrent hash, selected file index, partial path, expected size, and
   transfer state transactionally.
3. Verifies the completed byte count, atomically renames the partial file, and asks
   `LibraryManager` to register/refresh it.
4. Exposes active and resumable work on the Downloads page.
5. Preserves one-active-torrent and VPN-loss guarantees; it never continues remote
   transfer work outside verified protection.

## Verification

```bash
cmake --build build
cmake --build build --target all_qmllint
ctest --test-dir build --output-on-failure
```

All library tests use temporary local directories and files only.
