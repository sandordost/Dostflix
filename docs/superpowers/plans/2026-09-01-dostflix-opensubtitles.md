# OpenSubtitles search and download plan

**Status:** Implemented through `feature/subtitle-preferences`

## Scope

- Configure an OpenSubtitles.com API key, username, and password in Settings.
- Search releases in the user's ordered preferred languages from the player's final
  **Find subtitles…** menu action.
- Let the user select a result, store it beside a retained local movie (or in
  Dostflix application data for a stream), and
  attach it immediately through the existing mpv subtitle API.
- Keep every OpenSubtitles request behind the existing verified VPN and
  process-scoped kill-switch boundary.

## Implementation map

- `src/subtitles/OpenSubtitlesManager.{h,cpp}` owns authentication, search,
  download, result parsing, cancellation, and atomic local file storage.
- `qml/components/SubtitleSearchDialog.qml` presents search progress, errors,
  language/release metadata, and explicit download actions.
- `qml/pages/SettingsPage.qml` configures the account without exposing saved
  secrets, and `qml/Main.qml` connects the player action to the dialog.
- `AppSettings` persists the normalized, comma-separated ISO language order;
  OpenSubtitles credentials remain in the secret store.
- `LibraryManager` supplies the authoritative local video path and cached IMDb
  ID when local playback begins. `src/main.cpp` mirrors
  `VpnManager::networkReady` into the subtitle service and passes completed
  local files to `MpvPlayer::addSubtitleFile()`.
- `tests/subtitles/tst_open_subtitles_manager.cpp` exercises login, search,
  authenticated download, storage, and the VPN gate through a loopback-only
  fake server in an isolated network namespace.

## API and security contract

- API base: `https://api.opensubtitles.com/api/v1/`.
- Login uses `POST /login`; search uses `GET /subtitles`; selecting a result
  uses `POST /download` followed by the returned temporary URL.
- Local files of at least 128 KiB are identified with the standard
  OpenSubtitles 64-bit movie hash and exact byte size. A normalized numeric
  IMDb ID is included when TMDB metadata has supplied one. Release-title search
  remains present as the fallback and for live streams.
- Requests identify Dostflix with `User-Agent: Dostflix v0.1.0`. Search carries
  the API key; prepared downloads carry both the API key and in-memory bearer
  token.
- The required login `base_url` (`api.opensubtitles.com` or
  `vip-api.opensubtitles.com`) becomes the validated base for authenticated
  search and download requests. Requests explicitly accept JSON. A 503 is
  reported as an upstream download-service failure and is not automatically
  retried because OpenSubtitles may already charge the request to the quota.
- The API key, username, and password are stored as one JSON secret under
  `subtitles-opensubtitles` using `SecretStore`. The bearer token is never
  persisted. None of these values may be logged or placed in `settings.ini`.
- Losing `networkReady` aborts any active reply and clears the bearer token.
  Embedded and manually selected local subtitles remain fully offline.
- Downloaded filenames are stripped to their basename, limited to `.srt`,
  `.ass`, or `.vtt`, and written atomically. Local-library subtitles are named
  after the video (`Movie.nl.srt`) in the video's directory. Stream subtitles
  use `$XDG_DATA_HOME/dostflix/subtitles`.
- Preferred languages accept unique two- or three-letter alphabetic ISO codes,
  are normalized to lowercase, and retain user order. Invalid input does not
  overwrite the last valid preference; the default remains `nl,en`.

The request and response shapes follow the official OpenSubtitles API OpenAPI
description. If that service contract changes, update the manager and its fake
endpoint together; automated tests must never contact the public service.

## Remaining improvements

- Persist a subtitle reference in SQLite if later UI needs to distinguish
  downloaded sidecars from manually supplied files. mpv already discovers and
  reuses the sidecar filename without this extra database state.
- Add hash/IMDb context to a completed Downloads-page movie before its first
  metadata refresh; it currently gets sidecar storage immediately and richer
  identification after the Library metadata pass.

## Verification

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cmake --build build --target all_qmllint
ctest --test-dir build --output-on-failure
```

No public API, torrent, or VPN connection is used by this verification.
