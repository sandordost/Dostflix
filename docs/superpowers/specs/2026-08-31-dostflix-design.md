# Dostflix Design Specification

**Date:** 2026-08-31

**Target:** Arch Linux desktop

**Status:** User-approved design

## 1. Product definition

Dostflix is a native Linux desktop application for discovering user-configured torrent releases, streaming a selected video while it downloads, and retaining the completed video in a local library. The application activates a user-selected OpenVPN profile when it starts, prevents protected traffic from leaving outside that VPN, and disconnects a VPN connection it activated when it exits.

Dostflix provides generic provider interfaces only. It does not include, recommend, or preconfigure torrent indexers. Users configure their own Prowlarr or Torznab-compatible endpoints and are responsible for ensuring that their sources and content use are lawful.

The first release targets Arch Linux. Its interface follows the visual language of Dostify while using native Qt Quick components rather than web technology.

## 2. Goals

- Import and select arbitrary `.ovpn` configurations through NetworkManager.
- Establish the selected VPN before external provider or torrent traffic starts.
- Enforce a strict kill switch during search, download, and seeding.
- Query user-configured Prowlarr or Torznab endpoints without bundling indexers.
- Enrich movie results through an optional user-supplied TMDB token.
- Stream partially downloaded videos through an embedded mpv player.
- Start playback after an adaptive safe buffer targeting 30 seconds.
- Continue downloading the selected video into a persistent library.
- Seed while Dostflix is open and the VPN remains active.
- Support embedded subtitles, local subtitle files, and optional OpenSubtitles search.
- Package the application for Arch Linux with a `PKGBUILD`.

## 3. Non-goals for the first release

- Bundled torrent sites, indexer definitions, or unofficial site mirrors.
- A built-in HTML scraper or arbitrary JavaScript provider plugins.
- Usenet downloads.
- Background seeding after Dostflix exits.
- Automatic subtitle providers other than OpenSubtitles.
- Mobile, television, Windows, or macOS versions.
- A Films/Series mode switch. The first release is movie-oriented; the header will not contain a decorative mode button.
- A persistent music-style playback bar on browsing screens.

## 4. Technology stack

- C++20
- Qt 6 and Qt Quick/QML
- CMake
- TorrServer as a managed torrent-to-HTTP streaming service
- `libmpv` using the render API
- NetworkManager D-Bus API and OpenVPN plugin
- `nftables` controlled by a narrowly scoped privileged helper and Polkit policy
- SQLite through Qt SQL
- Qt Network for local HTTP range serving and remote APIs
- Qt Test for automated C++ and QML tests

## 5. Component architecture

### 5.1 `VpnManager`

Imports `.ovpn` files through NetworkManager, lists suitable VPN profiles, activates the selected profile, observes connection state, and deactivates only profiles that Dostflix itself activated. It stores the NetworkManager connection UUID rather than copying private keys into the application data directory.

### 5.2 `NetworkGuard`

Coordinates the privileged helper that installs and removes Dostflix-specific `nftables` rules. While protection is enabled, external Dostflix traffic may use only the selected VPN interface. Exceptions are limited to loopback, local communication required by Dostflix, and the VPN server endpoint needed to establish the tunnel. Torrent, provider, TMDB, and OpenSubtitles traffic remains disabled until both NetworkManager state and routing checks pass.

Rules are tagged and scoped so Dostflix removes only its own rules. The helper exposes a minimal command surface and validates every requested interface, endpoint, and rule identifier.

### 5.3 `ProviderManager`

Stores multiple user-configured Prowlarr or Torznab endpoints and their credentials in the desktop secret store when available. It validates capabilities, sends searches, normalizes results, deduplicates releases, and returns title, size, seeders, leechers, publish time, category, source label, download URL, and magnet information where available.

No provider definition ships with Dostflix.

### 5.4 `MetadataService`

Uses a user-supplied TMDB access token to match normalized movie titles and retrieve poster, backdrop, year, synopsis, genres, runtime, and external identifiers. Provider results remain usable when TMDB is unavailable or no token is configured. Cached metadata has an expiry and can be refreshed manually.

### 5.5 `TorrServerManager`

Starts and supervises a private TorrServer child process after VPN protection is ready. The process inherits Dostflix's protected systemd scope, stores its database below Dostflix's data directory, and exposes its HTTP API on a random loopback port only. Dostflix uses that API for magnet and torrent submission, metadata acquisition, file selection, playback-range priority, readahead, cache state, progress, and peer health. Dostflix does not expose or reuse a system TorrServer service.

If a torrent contains multiple plausible video files, Dostflix asks the user which one to play. TorrServer remains the sole owner of peer selection, tracker behavior, streaming-piece priority, cache allocation, and reader readahead.

### 5.6 `BufferController`

Estimates playable seconds from contiguous available bytes, media bitrate, current download rate, and recent rate stability. Playback targets a 30-second safe buffer. A fully downloaded file can start immediately; otherwise the user sees progress and an estimated wait until the target is reached. Rebuffering pauses playback and resumes after at least 10 seconds are contiguous and the estimated download rate exceeds playback bitrate. The controller then continues rebuilding toward the 30-second target.

### 5.7 `StreamServer`

TorrServer exposes the selected video to mpv through its loopback-only, seekable HTTP endpoint with byte-range support. Its reader moves piece priority and readahead when playback seeks, so unavailable ranges return to buffering while verified data is fetched around the new position.

### 5.8 `PlayerController`

Embeds mpv through its render API for Qt/Wayland-compatible rendering. It controls playback, seek, volume, fullscreen, hardware decoding, audio tracks, subtitle tracks, subtitle delay, and buffering state. The default player is mpv. A later enhancement may add an external-player action for VLC, but that is not required for the first release.

### 5.9 `SubtitleService`

Lists embedded tracks and lets users attach `.srt`, `.ass`, and `.vtt` files. The subtitle menu ends with **Find subtitles…**. When selected, Dostflix uses the user's OpenSubtitles credentials to search by media hash and movie identifiers, presents language and release matches, downloads the chosen subtitle beside the video, and loads it into mpv. Local and embedded subtitle support works without OpenSubtitles.

### 5.10 `LibraryRepository`

Uses SQLite for movie records, torrent identity, selected file, download state, watch progress, metadata cache references, subtitle references, and resume-data paths. Video files and partial downloads live in a user-configurable library directory. State updates use transactions and atomic file replacement where applicable.

## 6. Application lifecycle and security

### 6.1 Startup

1. Open the local database and recover incomplete state.
2. Load settings without starting external traffic.
3. If onboarding is incomplete, show configuration while keeping external functions disabled.
4. Parse the VPN transport endpoint from the selected NetworkManager profile.
5. Resolve only that VPN hostname using the current system resolver as bootstrap traffic.
6. Install kill-switch rules that permit loopback, the resolved VPN endpoint and port, and only the resolver traffic required for VPN activation.
7. Ask NetworkManager to activate the selected VPN profile.
8. Verify active connection, tunnel interface, default routes, DNS routing, and external socket binding.
9. Remove the pre-tunnel resolver exception while retaining the VPN transport exception.
10. Enable provider, metadata, subtitle, and torrent networking.
11. Resume eligible downloads and seeding only after protection is confirmed.

Local library playback is available without a VPN when it needs no external metadata, subtitle, provider, or torrent traffic.

### 6.2 VPN loss

Kernel firewall rules remain the primary leak-prevention layer. On loss of the protected interface, all affected external sockets become unusable. Dostflix pauses provider work, torrents, seeding, and any player read that needs unavailable pieces. It shows a blocking security state and makes five reconnect attempts after 1, 2, 4, 8, and 16 seconds. After that, the user can retry or exit. It never falls back to the normal internet route.

### 6.3 Normal shutdown

1. Stop playback and cancel stream reads.
2. Allow the managed TorrServer process to persist its database and cache state.
3. Terminate TorrServer and thereby stop announcing, downloading, seeding, DHT, and peer sockets.
4. Confirm the managed process has exited.
5. Remove Dostflix-specific kill-switch rules.
6. Deactivate the VPN only if Dostflix activated it for this session.
7. Close the database and exit.

If rule removal or VPN deactivation fails, Dostflix reports the problem before completing shutdown when possible. The next start audits stale Dostflix rules and session state.

## 7. User flows

### 7.1 Onboarding and settings

Users choose or import an OpenVPN profile, select the library directory, and optionally add provider, TMDB, and OpenSubtitles credentials. Every remote integration has a test action. Sensitive values are excluded from logs and diagnostics.

### 7.2 Search and selection

The search field queries configured providers after the VPN is protected. Results are deduplicated and may be grouped under a TMDB movie. The default release ordering favors healthy seeder count, then quality and size information without silently selecting a torrent. The user explicitly chooses a release.

### 7.3 Buffer and playback

After release and file selection, the selected result thumbnail shows metadata
acquisition or buffering with a local spinner and concise state text. The header
remains reserved for brand and VPN state. Playback then opens fullscreen or in a
dedicated player view. Seeking moves TorrServer's active reader and may return to
buffering.

### 7.4 Library and seeding

Completed movies stay in the configured library. Partial downloads remain resumable. Dostflix seeds active completed torrents only while the application is open and VPN protection is confirmed. Closing Dostflix stops seeding before disconnecting its VPN.

### 7.5 Returning to playback

Browsing screens have no permanent playback bar. When a movie is active or paused, a compact clickable **Now watching** card appears with poster, title, progress, and **Return to movie**. The card is absent when no playback session exists.

## 8. Visual design

Dostflix follows Dostify's visual language rather than copying controls without their function:

- Dostify's dark blue-black textured background remains visible.
- Large content windows use translucent near-black/antracite fills with restrained blur.
- Borders are avoided; hierarchy comes from background contrast and spacing.
- Bright blue is reserved for meaningful separation or progress.
- Purple is used sparingly for brand and active actions.
- The header contains the Dostflix brand and VPN status. It has no decorative Films button.
- Transfer progress belongs to the selected release card, never to a global header bar.
- Typography uses one documented family and a small, consistent token scale.
- Menu items have semantically appropriate icons with uniform size and optical alignment.
- The movie grid wraps responsively based on available width.
- Poster thumbnails use a fixed `2:3` aspect ratio, stable width constraints, and `PreserveAspectCrop` behavior.
- Cards retain consistent geometry regardless of title or metadata length; text truncates predictably.
- Layout and icons scale correctly on HiDPI displays.
- Keyboard navigation, visible focus, screen-reader labels, and sufficient contrast are required.
- SDL-mapped controllers and Steam Input use spatial focus: vertical input stays
  in visual columns, horizontal input changes columns, and modal overlays contain
  focus. Search opens explicitly with Y/Triangle, while controller hints adapt
  between Xbox and PlayStation layouts. Keyboard and mouse remain active.
- Player D-pad/stick directions navigate controls; L2/R2 or LT/RT seek by 30 seconds.
- Player subtitle, delay, and volume controls use contained focus modes. Volume
  adjustment exits on confirmation or back, while small pointer jitter does not
  prevent the player chrome from auto-hiding.
- Activating an editable field with a controller opens a responsive in-app QWERTY
  keyboard on the screen edge opposite that field.
- Partially watched local movies resume directly instead of opening a resume/start-over prompt.
- Settings groups interactive controls into semantic rows. Horizontal input
  moves inside the current row; vertical input always enters the first available
  control of the adjacent row. Open select boxes exclusively consume navigation
  until selection or cancellation.

## 9. Error handling

- **Provider unavailable:** Keep the local library available and offer endpoint testing.
- **TMDB unavailable or unmatched:** Show normalized provider information and a placeholder image.
- **OpenSubtitles unavailable:** Keep embedded and local subtitle options available.
- **No peers or insufficient speed:** Show peer state and allow waiting, choosing another release, or cancelling.
- **Disk space insufficient:** Pause before corruption, show required/free space, and allow another library location.
- **Unsupported media or decoder failure:** Show concise diagnostics and a copy-details action.
- **VPN import failure:** Preserve the original file, show NetworkManager's sanitized error, and make no external request.
- **VPN loss:** Block protected traffic immediately and present the reconnect state.
- **Crash or power loss:** Recover from SQLite transactions and TorrServer's database without assuming streams completed.

Logs redact credentials, API tokens, cookies, private keys, signed URLs, and magnet query parameters that may contain secrets.

## 10. Testing strategy

### 10.1 Unit tests

Cover provider normalization and deduplication, title parsing, metadata matching, buffer calculations, library transitions, settings validation, ownership of VPN sessions, and shutdown ordering.

### 10.2 Integration tests

Use local fake HTTP services for Prowlarr/Torznab, TMDB, OpenSubtitles, and byte-range behavior. Start the real managed TorrServer backend in an isolated network namespace with loopback as its only interface. Use small freely distributable torrent fixtures to test metadata acquisition, file selection, preload startup, persistence, HTTP range behavior, and shutdown.

### 10.3 Network isolation tests

Run Dostflix components in Linux network namespaces with controlled VPN and non-VPN interfaces. Assert that provider requests, trackers, DHT, peer TCP, peer UDP, and metadata APIs cannot use the clear interface before connection, during tunnel loss, or during shutdown. Verify that the VPN endpoint exception cannot be generalized to arbitrary destinations.

### 10.4 Player tests

Test representative MKV and MP4 samples, embedded and external subtitles, multiple audio tracks, hardware-decoding fallback, initial buffering, rebuffering, and seek behavior. Include an MP4 whose metadata is at the end of the file.

### 10.5 UI tests

Use QML component tests and screenshot comparisons across window sizes, KDE, GNOME, and a wlroots-based Wayland compositor. Verify icon sizing, typography tokens, fixed poster ratios, grid wrapping, focus order, HiDPI scaling, conditional Now Watching visibility, and VPN error states.

## 11. Packaging and configuration

The Arch package contains the executable, QML modules, desktop file, application icon, Polkit action, privileged helper, translations, and licenses. The `PKGBUILD` declares Qt 6, NetworkManager/OpenVPN support, `torrserver-bin`, mpv, SQLite, and other runtime dependencies explicitly.

Application data follows XDG locations:

- Configuration: `$XDG_CONFIG_HOME/dostflix/`
- Database and resume state: `$XDG_DATA_HOME/dostflix/`
- Disposable cache: `$XDG_CACHE_HOME/dostflix/`
- Movies: user-selected library directory
- Secrets: Freedesktop Secret Service through `libsecret`; remote integrations cannot persist credentials when no compatible secret service is available

The repository and packages never contain the user's AirVPN file, client certificate, private key, provider credentials, TMDB token, or OpenSubtitles credentials.

## 12. Acceptance criteria

The first release is acceptable when:

1. A user can import and select a valid OpenVPN configuration through settings.
2. Protected searches and torrent traffic cannot start before VPN and kill-switch validation.
3. Automated isolation tests demonstrate no protected traffic leaks during VPN failure.
4. A configured Prowlarr/Torznab endpoint can return normalized movie releases.
5. Optional TMDB metadata enriches results without becoming a hard dependency.
6. A selected torrent can begin playback after the adaptive 30-second target buffer.
7. Seeking into unavailable data returns to buffering without serving unverified bytes.
8. The completed movie remains in the selected library and can play locally later.
9. Seeding occurs only while Dostflix is open and VPN protection is active.
10. Embedded and local subtitles work, and OpenSubtitles search appears at the bottom of the subtitle menu when configured.
11. The browsing UI has no playback bar and shows a Return to movie card only for an active session.
12. The responsive grid, poster geometry, icon scale, typography, HiDPI behavior, and Dostify-inspired translucency pass UI tests.
13. Normal shutdown stops torrent traffic before removing protection and disconnecting a Dostflix-owned VPN.
14. The application installs successfully through its Arch `PKGBUILD`.
15. Xbox-, PlayStation-, Nintendo-, Steam-, and other SDL-mapped controllers can
    navigate spatially, enter and leave search, confirm, return to an active
    movie, pause, seek, operate subtitles, and toggle player fullscreen with
    hot-plugging and layout-aware hints supported.
