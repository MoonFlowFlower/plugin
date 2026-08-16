# Runtime Inspector Fab Asset Checklist

This is a Fab media checklist, not the RuntimeInspector development authority. Use `docs/AGENT_DEVELOPMENT.md` for implementation rules.

Capture only shipped Unreal Engine 5.7 functionality. Do not show internal-only diagnostics, temporary logs, or features outside the release scope.

## Required Listing Media

- `FabMedia/cover.png`: Runtime Inspector visible in a clean PIE scene with readable product text and controls.
- `FabMedia/screenshot_01_actor_panel.png`: a real picked actor, component list, and supported runtime values.
- `FabMedia/screenshot_02_changes_workflow.png`: the Changes workspace and staged/review path.
- `FabMedia/screenshot_03_settings.png`: hotkey and interaction settings without clipped labels.
- `FabMedia/screenshot_04_tools.png`: non-empty Tests/Workflows configuration and run controls.
- `FabMedia/demo.mp4`: a real live capture, not a slideshow; open the panel, visit Actor/Changes/Settings/Tools, and run a configured workflow. Use H.264, `1920x1080`, `30 fps`, and 30-45 seconds.

## Compliance Rules

- Use Unreal Engine 5.7 and the current shipped UI.
- Final still images must be at least `1920x1080` and below `3145728` bytes each.
- Use the same product name, release scope, documentation URL, and support URL as `RuntimeInspector.uplugin`, `README.md`, and `FAB_LISTING.md`.
- The public support URL must return HTTP 200 before the release is called Fab-ready.
- Keep final files under `FabMedia/`; provenance and hashes belong in `FabMedia/fab_media_manifest.json`.

## Capture Boundary

- Listing media may be captured in the main PluginMaker project because it contains the curated visual fixture.
- Install/load and input-path signoff must still be repeated in the exact ZIP-derived blank host; listing media is not evidence of clean-install behavior.
- Media compliance proves dimensions, encoding, and the recorded flow only. It does not prove Fab acceptance, hardware-wide DPI behavior, or asset-rights ownership.
