# Watch progress and local resume plan

**Status:** Implemented on `feature/watch-progress`

## Scope

- Persist local playback position and actual media duration in the existing
  library record.
- Offer **Resume** and **Start over** when a meaningfully watched local movie is
  selected again.
- Seek only after mpv confirms that the file has loaded.
- Clear progress automatically when playback reaches the final minute or 95%.

## Implementation map

- `LibraryDatabase::updateWatchProgress` updates the existing
  `watched_seconds`/`duration_seconds` fields; no schema migration is required.
- `LibraryManager` owns the active local playback identity, writes progress at
  five-second intervals, forces a final write before stop, and ignores position
  reports from torrent or unrelated local playback. Playback signal arguments
  are copied before a restart refreshes the model, preventing dangling model
  references from crossing into QML.
- `LocalLibraryModel::updateProgress` updates only the affected model roles, so
  persistence does not reset the grid or scroll position.
- `MpvPlayer::play` accepts an optional start position and applies it on
  `MPV_EVENT_FILE_LOADED`. `playbackStopping` exposes the final position before
  player state is reset.
- `LibraryPage.qml` shows the saved position and opens a resume/start-over dialog
  once at least 30 seconds have been watched and the movie is not nearly complete.

## Invariants

- Playback progress is local database state and never depends on VPN readiness.
- Starting a torrent or a Downloads-page file clears the active library session,
  preventing that playback from being written onto the previous library item.
- A crash can lose at most roughly five seconds of progress. Normal stop and app
  shutdown force an exact final write.
- Restarting explicitly clears saved progress before playback begins.

## Verification

- Database tests cover progress/duration persistence.
- Library manager tests cover resume, restart, forced writes, and session clearing.
- QML tests cover direct play and the resume-dialog action.
- The mpv integration test starts a generated local video at an offset and checks
  the pre-reset stop signal.
