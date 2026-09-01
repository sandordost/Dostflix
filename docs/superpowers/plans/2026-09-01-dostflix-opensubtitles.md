# OpenSubtitles search and download plan

**Status:** Implemented on `feature/opensubtitles-search`

## Scope

- Configure an OpenSubtitles.com API key, username, and password in Settings.
- Search Dutch and English subtitle releases from the player's final
  **Find subtitles…** menu action.
- Let the user select a result, download it to Dostflix application data, and
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
- `src/main.cpp` mirrors `VpnManager::networkReady` into the subtitle service
  and passes completed local files to `MpvPlayer::addSubtitleFile()`.
- `tests/subtitles/tst_open_subtitles_manager.cpp` exercises login, search,
  authenticated download, storage, and the VPN gate through a loopback-only
  fake server in an isolated network namespace.

## API and security contract

- API base: `https://api.opensubtitles.com/api/v1/`.
- Login uses `POST /login`; search uses `GET /subtitles`; selecting a result
  uses `POST /download` followed by the returned temporary URL.
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
  `.ass`, or `.vtt`, written atomically, and stored below
  `$XDG_DATA_HOME/dostflix/subtitles`.

The request and response shapes follow the official OpenSubtitles API OpenAPI
description. If that service contract changes, update the manager and its fake
endpoint together; automated tests must never contact the public service.

## Remaining improvements

- Prefer an OpenSubtitles movie hash and stable title identifiers when the
  retained media file and metadata expose them; release-title search is the
  current safe baseline.
- Allow configurable preferred languages instead of the current `nl,en` order.
- Persist a chosen subtitle beside a retained library movie when its final
  storage path becomes authoritative.

## Verification

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cmake --build build --target all_qmllint
ctest --test-dir build --output-on-failure
```

No public API, torrent, or VPN connection is used by this verification.
