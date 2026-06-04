# Runtime Inspector RC Signoff

This file is the release-candidate signoff guide for the current Fab submission pass.

It is not the development authority. For implementation rules, use `docs/AGENT_DEVELOPMENT.md`.

## RC Baseline

- Branch: `codex/runtimeinspector-fab-rc`
- Product RC commit: `0a26995c1f057972bb175afa483326cb1c2df886`
- Product RC summary: `Fix RuntimeInspector Fab plugin package build`
- Package root:
  - `Saved/FabRelease/Package/RuntimeInspector_UE55/RuntimeInspector`
- Package log:
  - `Saved/build_runtimeinspector_fab_release.log`
- Blank validation host:
  - `Saved/FabRelease/BlankProjectValidation/RuntimeInspectorBlank_UE55/RuntimeInspectorBlank/RuntimeInspectorBlank.uproject`
- Automated blank-load log:
  - `Saved/fab_blank_project_validation.log`

## Automated Evidence

- Build:
  - `PluginMakerEditor Win64 Development` passed after the package-build include fix.
- Runtime/UI regression:
  - `favorite_icon_visual_contract`
  - `dock_material_tree_route`
  - `dock_material_edit_route`
  - `row_text_overflow_contract`
  - `function_run_button_visual_contract`
  - `color_picker_preview_perf`
  - `runtime_ui_contract_v1`
- Package:
  - `Scripts/PackageFabRelease.ps1`
- Blank host automated load:
  - `Scripts/ValidateFabBlankProject.ps1 -KeepValidationProject`
- Media:
  - `Scripts/OpenFabScreenshotState.ps1`
  - `Scripts/CaptureFabMedia.ps1`
  - `python Scripts/NormalizeFabMedia.py --input Saved/RuntimeInspector/FabMediaCapture --output FabMedia --width 1920 --height 1080 --max-bytes 3145728`

## Media Signoff

- Final image manifest:
  - `FabMedia/fab_media_manifest.json`
- Final image status:
  - all five images passed `1920x1080`, PNG, `<3145728` bytes
- Final images:
  - `FabMedia/cover.png`
  - `FabMedia/screenshot_01_actor_panel.png`
  - `FabMedia/screenshot_02_changes_workflow.png`
  - `FabMedia/screenshot_03_settings.png`
  - `FabMedia/screenshot_04_tools.png`
- Actor screenshot note:
  - generated through real input-equivalent actor pick
  - selected actor: `BP_ThirdPersonCharacter_C_0`
- Demo:
  - `FabMedia/demo.mp4`
  - ffprobe: H.264, `1920x1080`, `30 fps`, `35.000000s`, `734861` bytes
  - note: deterministic screenshot-sequence video, not a live operation recording

## Blank-Host Manual Smoke

Use the preserved blank-install host project. Do not use the main `PluginMaker` project for this signoff.

Important boundary:

- the preserved blank host contains the packaged `RuntimeInspector` plugin only
- it does not include `UE_MCP_Bridge` or `control_runtime_inspector` automation hooks
- this step remains human/manual

### Minimum Path

1. Open:
   - `Saved/FabRelease/BlankProjectValidation/RuntimeInspectorBlank_UE55/RuntimeInspectorBlank/RuntimeInspectorBlank.uproject`
2. Wait for shader compilation to finish.
3. Open Runtime Inspector.
4. Select a test actor in the level.
5. Verify `Actor` page opens and shows selection plus component/property content.
6. Switch to `Changes` and confirm the page opens without embedded settings content.
7. Switch to `Settings` and confirm it is the only highlighted settings-style tab.
8. Switch to `Tools` and confirm it is the only highlighted tools-style tab.
9. Return to `Actor` and confirm basic interaction still works.

### Pass Criteria

- Panel opens successfully.
- Actor selection appears.
- `Actor / Changes / Settings / Tools` all open.
- No page is blank, blocked, or misrouted.
- No double-highlighted tabs.
- No blocking modal/input leak after normal use.

### Current Record

- Date: 2026-06-04
- Operator: Codex attempted launch only
- Result: `PENDING`
- Notes:
  - Automated blank-load validation passed.
  - Manual interaction smoke was not completed because the blank host remained on the UE splash/shader compilation screen during the wait window.
  - Evidence:
    - `Saved/FabRelease/blank_manual_smoke_editor.png`
    - `Saved/FabRelease/blank_manual_smoke_editor_after_wait.png`

## Remaining Non-Repo Steps

- Complete blank-host manual smoke after shader compilation finishes.
- Create/submit Fab listing in the backend.
- Fill `MarketplaceURL` after the Fab listing exists.
- Wait for Epic review result.
