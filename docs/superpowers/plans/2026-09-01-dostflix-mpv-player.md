# Embedded mpv player plan

**Status:** Implemented on `feature/mpv-player`

## Scope

- Render the selected TorrServer loopback stream through libmpv's OpenGL render API.
- Open the dedicated player automatically after the safe buffer becomes ready.
- Provide pause/resume, relative and absolute seeking, volume, fullscreen, stop,
  current time, duration, buffering state, and visible playback errors.
- Keep playback active while browsing and expose the existing **Return to movie** card.
- Stop mpv before application shutdown so TorrServer and VPN teardown remain ordered.

Subtitle track enumeration, local subtitle attachment, and **Find subtitles…** are
the next implementation increment.

## Verification

- Build and QML lint the complete application against the installed libmpv.
- Unit-test controller defaults and bounded volume changes without remote media.
- Exercise the player page with a fake controller in Qt Quick tests.
- Keep torrent integration tests inside their isolated network namespace.
