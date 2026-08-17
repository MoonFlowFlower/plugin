# Runtime Inspector Fab Media Staging

This directory contains the current Unreal Engine 5.7 listing media. It is not the RuntimeInspector development authority.

## Final Files

- `cover.png`
- `screenshot_01_actor_panel.png`
- `screenshot_02_changes_workflow.png`
- `screenshot_03_settings.png`
- `screenshot_04_tools.png`
- `demo.mp4`
- `fab_media_manifest.json`

## Compliance

- All five PNG files are `1920x1080` and below `3145728` bytes.
- `demo.mp4` is H.264, `1920x1080`, `30 fps`, approximately `41.93s`, and below 25 MiB; exact values and hashes are recorded in `fab_media_manifest.json`.
- The video is a live desktop capture of actual RuntimeInspector keyboard and mouse interaction. Only idle gaps were removed, the retained real-input segments use one uniform `1.5x` presentation rate, and the Windows taskbar was cropped before letterboxing. It is not a screenshot-sequence video.
- The recorded path opens the panel with `O`, visits Actor/Changes/Settings/Tools by mouse, displays a staged old/new change, runs `Dock Layout`, runs `Mainline Safe Patch Core`, and ends on its `Passed=6 Failed=0` activity-log identity.

## Reproduction Flow

1. Use `Scripts/OpenFabScreenshotState.ps1` to establish the curated main-project presentation state.
2. Capture staging PNGs under the project-level `Saved/RuntimeInspector/FabMediaCaptureA34/` directory from the live preview state.
3. Visually inspect every staging image; dimensions alone do not detect clipped or stale-window captures.
4. Normalize approved stills:
   - `python Scripts/NormalizeFabMedia.py --input <capture-dir> --output FabMedia --width 1920 --height 1080 --max-bytes 3145728`
5. Capture the video from the live PluginMaker Preview window using real mouse/keyboard interaction, then encode H.264/yuv420p/30fps.
6. Record hashes and provenance in `fab_media_manifest.json`.

## Boundary

- Listing visuals are captured in the main PluginMaker project because it supplies the curated scene and test actor.
- Exact ZIP-derived `RIFabBlank` evidence is kept separately under `Saved/RuntimeInspector/Task5/`; it must not be replaced by these listing images.
- These files do not prove Fab acceptance, asset-rights ownership, or every hardware/DPI configuration.
