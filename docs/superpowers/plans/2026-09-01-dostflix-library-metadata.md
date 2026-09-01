# Local library metadata plan

**Status:** Implemented on `feature/library-metadata`

## Scope

- Recognize common release filenames as a readable title and optional year.
- Match unmatched local movies with TMDB when a Read Access Token is configured.
- Persist canonical title, year, runtime, synopsis, TMDB ID, and IMDb ID.
- Cache posters below the Dostflix data directory for offline library browsing.

## Implementation map

- `MovieFilenameParser` is a network-free parser used by every library scan. It
  removes common release separators/tokens and recognizes plain or bracketed
  years while retaining years that belong to a title, such as `Blade Runner 2049`.
- `LibraryDatabase` schema v4 adds `tmdb_id`, `imdb_id`, `synopsis`, and
  `metadata_updated_at`. Existing year, poster, and duration columns are now
  populated. Rescans do not overwrite an already enriched record.
- `LibraryMetadataManager` queues unmatched database rows, performs TMDB search
  and detail requests, stores the selected result, and downloads the poster via
  an atomic `QSaveFile` write to `metadata/posters`.
- `LocalLibraryModel` exposes the synopsis in addition to the existing metadata
  roles. `LibraryPage.qml` displays year and runtime and shows synopsis on hover.

## Security and lifecycle invariants

- Parsing, scanning, database access, cached posters, and local playback never
  require a VPN or network connection.
- TMDB requests begin only when the existing verified `networkReady` gate is
  true and a token exists. Losing protection aborts the active reply and clears
  queued network work.
- The metadata worker is cancelled before application shutdown. It neither
  starts nor controls the VPN and never performs torrent operations.
- TMDB credentials remain in the desktop secret store; only the resulting public
  movie metadata is stored in SQLite.

## Verification

- `tst_movie_filename_parser` covers standard, bracketed, yearless, and title-year
  release names.
- `tst_library_database` verifies schema v4 and the complete metadata round trip.
- `tst_library_metadata_manager` uses a loopback fake TMDB server to verify search,
  details, bearer authentication, poster caching, and database/model refresh.
- QML tests verify that enriched and unenriched library rows both instantiate and
  remain playable.

No test contacts TMDB, starts a VPN, or transfers torrent data.

## Deliberate follow-ups

- Add an explicit manual match/correction flow for ambiguous filenames.
- Store genres and backdrop artwork when the detail page needs them.
- Add locale selection and stale-metadata refresh policy.
- Watch-position persistence and resume choice are implemented in
  `2026-09-01-dostflix-watch-progress.md`.
