# Runtime Inspector Fab Media Staging

This file is a Fab media staging note, not the RuntimeInspector agent development authority.

For implementation rules and development workflow authority, use `docs/AGENT_DEVELOPMENT.md`.

## Final Listing Files

- `cover.png`
- `screenshot_01_actor_panel.png`
- `screenshot_02_changes_workflow.png`
- `screenshot_03_settings.png`
- `screenshot_04_tools.png`
- `fab_media_manifest.json`
- `demo.mp4`

## Current Compliance

- All final PNG screenshots are `1920x1080`.
- All final PNG screenshots are below `3145728` bytes.
- `demo.mp4` is H.264, `1920x1080`, `30 fps`, `35.000000s`, `810418` bytes, and below 25MB.

## Capture Flow

1. Use `Scripts/PackageFabRelease.ps1` to refresh the clean package when product code changes.
2. Use `Scripts/OpenFabScreenshotState.ps1` to normalize the screenshot presentation state in the main project.
3. Use `Scripts/CaptureFabMedia.ps1` to generate deterministic staging PNGs in:
   - `Saved/RuntimeInspector/FabMediaCapture/`
4. Use `Scripts/NormalizeFabMedia.py` to normalize approved screenshots into this folder:
   - `python Scripts\NormalizeFabMedia.py --input Saved\RuntimeInspector\FabMediaCapture --output FabMedia --width 1920 --height 1080 --max-bytes 3145728`
5. Generate or refresh `demo.mp4` from the approved final screenshots.

## Capture Notes

- `CaptureFabMedia.ps1` uses the RuntimeInspector bridge only to set presentation state and perform the Actor-page real pick.
- Final screenshot bytes are captured through a local Win32 whole-window capture fallback to avoid bridge screenshot stalls.
- The capture script now skips minimized target windows, selects the largest valid `PluginMaker Preview` window, temporarily foregrounds it, and removes topmost after capture.
- Always visually inspect at least `cover.png` and one tab-specific screenshot after capture. A tiny titlebar image or unrelated browser/Codex image is a failed media run, even if the size manifest passes.
- The Actor screenshot uses `position_mouse_on_player_character` plus `right_mouse_pick_input`, so the visible actor page is backed by a real input-equivalent selection.
- Optional remote/promote shots are disabled by default; pass `-IncludeOptionalShots` only when those slower states are needed.
- Do not use the blank validation host for final Fab screenshots. It exists only for install/load/manual smoke validation.

## Demo Note

The current `demo.mp4` is a deterministic screenshot-sequence video built from the final approved listing images. It is not a live operation recording.
