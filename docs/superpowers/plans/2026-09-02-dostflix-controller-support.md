# Dostflix controller support

Date: 2026-09-02
Status: implemented

## Goal

Make the complete native interface usable from common living-room controllers,
including Steam Controller and Steam Deck, without replacing simultaneous mouse
or keyboard input and without adding a proprietary Steamworks runtime dependency.

## Input backend

`ControllerManager` uses SDL3's standardized gamepad API. SDL normalizes physical
button numbers into named positions such as south/east face buttons, D-pad,
shoulders, Start, and left-stick axes. SDL ships mappings for popular controllers
and accepts additional user mappings. The manager enables SDL's Steam Controller
and Steam Deck HIDAPI drivers, enumerates controllers already present at startup,
and handles add/remove events at runtime.

Steam Input gamepad emulation presents Steam Controller, PlayStation, Switch, and
other supported devices as a conventional Xbox-style gamepad on Linux. Dostflix
therefore supports Steam Input through the same SDL path. Direct SDL HIDAPI support
also covers a Steam Controller where the environment exposes it without gamepad
emulation. This increment deliberately does not link the Steamworks SDK: native
Steam action manifests and device-specific glyph lookup can be added later if
Dostflix receives a Steam App ID.

Primary references:

- <https://wiki.libsdl.org/SDL3/CategoryGamepad>
- <https://partner.steamgames.com/doc/features/steam_controller/steam_input_gamepad_emulation_bestpractices>

## Action mapping

| Standardized input | Dostflix action |
| --- | --- |
| D-pad or left stick | Move through focusable controls |
| South / A / Cross | Confirm the focused control |
| East / B / Circle or Back | Return or close |
| Start | Play or pause an active movie |
| Left/right shoulder | Previous/next page; seek 30 seconds in player |
| West / X / Square | Open subtitles in player |
| North / Y / Triangle | Focus search while browsing; toggle player fullscreen |
| Left/right while playing | Seek 10 seconds |

The analog stick uses separate press and release thresholds to prevent dead-zone
chatter. D-pad and stick directions share one state so overlapping inputs do not
double-fire. Held directions repeat after 360 ms and then every 95 ms.

## UI and lifecycle

Controller navigation scores controls by their actual on-screen position instead
of translating every direction into Tab. Up/down therefore follows rows and
left/right changes columns; modal popups constrain focus to their overlay. Search
is intentionally excluded from this directional graph and entered only through
Y/Triangle. B/Circle first closes search, subtitles, or another popup, then
returns to an active movie when browsing. Movie cards, result rows, library rows,
file-picker entries, and the Now Playing card accept controller activation.

When a controller is connected, the sidebar shows LB/RB or L1/R1 section hints,
search shows Y/Triangle, and the Return to movie card shows B/Circle. PlayStation
controllers are detected from SDL's gamepad type/name; all other mapped devices
use Xbox-style labels without changing the normalized action mapping.

Settings reports the connected device name, number of controllers, mapping, and
SDL initialization failures. Mouse, keyboard, and controller input remain active
simultaneously. Removing the final controller during unpaused playback pauses the
movie instead of allowing playback to continue without an input device.

Controller initialization and shutdown are local input operations and do not
touch the VPN, kill switch, Prowlarr, TorrServer, or network lifecycle.

## Verification

- Unit tests cover common face/shoulder mapping, button release suppression,
  D-pad input, analog dead-zone hysteresis, overlapping stick/D-pad state,
  popup detection, and spatial direction selection.
- QML tests cover controller activation of movie and Now Playing cards, subtitle
  menu access, controller-only search entry/exit, and visible focus behavior.
- QML lint, the complete CTest suite, and Arch packaging remain required.
- Final hardware validation should include Steam Controller through Steam Input,
  one Xbox-layout controller, and one PlayStation-layout controller in Gamescope.
