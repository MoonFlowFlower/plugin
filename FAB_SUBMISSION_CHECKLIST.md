# Runtime Inspector Fab Submission Checklist

This file is a Fab submission checklist, not the RuntimeInspector agent development authority.

For implementation rules and development workflow authority, use `docs/AGENT_DEVELOPMENT.md`.

## Current RC Status

- Current RC branch: `codex/runtimeinspector-fab-rc`
- Product RC commit: `88dacaae56b5aa834ec435c2c5d0d5ba91f73762`
- Product RC summary: `Fix RuntimeInspector blank host dock scaling`
- Superseded RC: `0a26995c1f057972bb175afa483326cb1c2df886`
  - reason: blank validation host revealed a DPI scaling bug in the dock UI.
- Repo-local package, automated blank-load validation, DPI-aware dock validation, final screenshots, media manifest, and demo video are prepared.
- Do not claim Fab-ready until full blank-host manual tab smoke is signed.
- Remaining external follow-up: replace `MarketplaceURL` after the Fab listing exists.

## Package

- [x] `RuntimeInspector.uplugin` has release metadata for UE 5.5.
- [x] `README.md` explains installation, activation, scope, and support.
- [x] `Config/FilterPlugin.ini` is present for release packaging.
- [x] `Scripts/PackageFabRelease.ps1` builds and stages a clean release package.
- [x] Clean package output exists:
  - `Saved/FabRelease/Package/RuntimeInspector_UE55/RuntimeInspector`
- [x] Final package excludes `.git`, `Docs`, `Saved`, internal logs, and other repo-only residue.
- [x] Final package retains precompiled `Binaries` for direct install into a blank UE 5.5 Blueprint project.
- [x] Final package retains validated `Intermediate` precompile data emitted by `BuildPlugin -NoHostPlatform`.

## Listing Copy

- [x] Title, short description, long description, and feature bullets are prepared in `FAB_LISTING.md`.
- [x] Support and documentation links are defined in `RuntimeInspector.uplugin`.
- [ ] Replace `MarketplaceURL` in `RuntimeInspector.uplugin` after Fab listing is created.

## Media

- [x] Capture cover image:
  - `FabMedia/cover.png`
- [x] Capture product screenshots:
  - `FabMedia/screenshot_01_actor_panel.png`
  - `FabMedia/screenshot_02_changes_workflow.png`
  - `FabMedia/screenshot_03_settings.png`
  - `FabMedia/screenshot_04_tools.png`
- [x] Normalize all final 2D images to `1920x1080`, PNG/JPEG, under `3145728` bytes:
  - `FabMedia/fab_media_manifest.json`
- [x] Record demo video:
  - `FabMedia/demo.mp4`
  - ffprobe: H.264, `1920x1080`, `30 fps`, `35.000000s`, `810418` bytes
- [x] Confirm screenshots match the current shipped UE 5.5 UI surface.
- [x] Automated capture path is available and now guards against minimized/foreground-window capture:
  - `Scripts/CaptureFabMedia.ps1`
- [x] Media normalization tool is available:
  - `Scripts/NormalizeFabMedia.py`

## Validation

- [x] Real PIE `SceneComponent transform -> Stage -> Apply To Source -> next PIE persists` validation passes on the local baseline.
  - `Saved/RuntimeInspector/Validation/TransformSourcePersistence/BC8B047E-400B-566E-1347-2DA5AA231920/prepare_report.json`
  - `Saved/RuntimeInspector/Validation/TransformSourcePersistence/BC8B047E-400B-566E-1347-2DA5AA231920/verify_restore_report.json`
- [x] UE 5.5 `BuildPlugin` validation passed through the release script.
- [x] Fresh local package regeneration passed.
- [x] Blank-project automated load validation passed:
  - `Saved/fab_blank_project_validation.log`
- [x] Preserved blank validation host exists:
  - `Saved/FabRelease/BlankProjectValidation/RuntimeInspectorBlank_UE55/RuntimeInspectorBlank/RuntimeInspectorBlank.uproject`
- [x] Blank-host packaged PIE/open/readability smoke passed for the scaling fix:
  - `Saved/FabRelease/blank_host_dpi_fix_printwindow.png`
  - `Saved/FabRelease/blank_host_pid_48944.png`
- [ ] Full blank-host manual tab smoke is signed.
  - `Actor / Changes` have blank-host visual evidence.
  - `Settings / Tools` need a human/operator click-through in the blank host because OS coordinate automation was not reliable enough to sign them.
- [x] Capture Fab screenshots from the main `PluginMaker` editor state, not the blank validation host.
  - `Saved/RuntimeInspector/FabMediaCapture/capture_fab_media.log`
  - `Saved/RuntimeInspector/FabMediaCapture/capture_manifest.txt`

## Submission Boundary

- [ ] Fab backend account/tax/payout/listing metadata completed outside the repo.
- [ ] Fab listing review completed by Epic.
- [ ] `MarketplaceURL` updated after the listing exists.
