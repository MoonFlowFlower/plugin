# Runtime Inspector Fab Media Staging

This directory contains the current Unreal Engine 5.7 listing media. It is not the RuntimeInspector development authority.

## Final Files

- `cover.png`
- `screenshot_01_actor_panel.png`
- `screenshot_02_changes_workflow.png`
- `screenshot_03_settings.png`
- `screenshot_04_tools.png`
- `screenshot_05_responsive_layouts.png`
- `demo.mp4`
- `fab_media_manifest.json`

## Compliance

- All six PNG files are `1920x1080` and below `3145728` bytes.
- `demo.mp4` is H.264/yuv420p, `1920x1080`, `30 fps`, `44.033333s`, and below 25 MiB. Exact dimensions, byte counts, SHA-256 values, and provenance are in `fab_media_manifest.json`.
- The video is a live desktop capture of actual RuntimeInspector keyboard and mouse interaction. It is not a screenshot-sequence video.
- The recorded path opens the panel with `O`, visits Actor/Changes/Settings/Tools by mouse, edits and stages a property, runs `Dock Layout`, runs `Mainline Safe Patch Core`, and ends with `Passed=6 Failed=0` visible.
- `screenshot_05_responsive_layouts.png` compares `UIScale=1.0` at 1280x720, 1920x1080, 3840x2160, and 900x1200. It illustrates the fixed screen-pixel default; automated reports remain the measurement authority.

## Reproduction Flow

1. Use `Scripts/OpenFabScreenshotState.ps1` to establish the curated main-project presentation state.
2. Capture live PNGs under project/plugin `Saved/RuntimeInspector/` and visually inspect them; dimensions alone cannot detect clipped or stale-window captures.
3. Normalize approved stills with `Scripts/NormalizeFabMedia.py` to 1920x1080 and the 3 MiB limit.
4. Capture the video from the live PluginMaker Preview window with real mouse/keyboard interaction, then encode H.264/yuv420p/30fps.
5. Generate the responsive comparison from passing matrix screenshots, not from synthetic UI mockups.
6. Record hashes and capture provenance in `fab_media_manifest.json`.

## Provenance Boundary

- The user-facing runtime implementation shown by the media is commit `6020b240bd635333102d77beb9829e62c1e7d8e6`; later validation-only commits do not change those pixels.
- The release commit cannot embed its own final Git hash or source-ZIP hash without creating a self-reference. `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json` is the post-commit authority for both.
- Listing visuals use the main PluginMaker project because it supplies the curated scene and test actor. Exact ZIP-derived `RIFabBlank` evidence is kept separately under `Saved/` and must not be replaced by listing images.
- These files do not prove Fab acceptance, asset-rights ownership, or every hardware/DPI configuration.
