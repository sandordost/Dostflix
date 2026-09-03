# Dostflix V1 handoff

## Runtime flow

1. `VpnManager` installs the process-scoped guard and verifies NetworkManager, tunnel, route and firewall state.
2. Only `networkReady == true` enables Prowlarr, TorrServer, downloads, TMDB and OpenSubtitles.
3. `ProwlarrManager` resolves a selected release. `TorrServerManager` retires the prior swarm before starting its replacement.
4. `DownloadManager` writes a resumable `.dostflix.part` from TorrServer loopback and atomically renames only a complete file.
5. `MpvPlayer` opens the loopback stream with `loadfile … replace`; local library playback remains offline-capable.

Never weaken VPN gating, expose managed service APIs beyond loopback, or run the production app in automated verification.

## V1 additions

- Search submits only on Enter. Submitting an empty query selects Highlights without provider traffic.
- `MovieHighlightsManager` fetches VPN-gated TMDB shelves from `trending/movie/week` and two `discover/movie` queries. Tokens remain inside `ProviderManager`; QML sees models only.
- Highlights cards are search shortcuts, not playable releases. Controller navigation treats each shelf as a row: left/right within it and up/down to column zero of another shelf.
- Starting another release stops the current mpv session, cancels the live TorrServer transfer state and then prepares the replacement. Partial retention remains resumable.
- `DownloadManager::finalPath` establishes subtitle context as soon as retention starts. OpenSubtitles writes `Movie.language.ext` beside that future path even while the video is still `.dostflix.part`, so mpv discovers it after restart.
- Fill Screen maps to mpv `panscan=1`; Fit Video maps to `panscan=0`. This crops only as needed to remove side bars and does not distort the image.

## Verification

`ctest` covers VPN gating, fake TMDB/OpenSubtitles servers, subtitle sidecar persistence, torrent replacement primitives, player state, controller focus and QML behavior. The network-isolation target uses namespaces and must remain separate from normal unit tests.

Release screenshots are rendered from the real QML component library with inert models; they do not start VPN, Prowlarr or TorrServer.
