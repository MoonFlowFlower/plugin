# RuntimeInspector Status

## Current Conclusion

The current Fab product RC baseline is frozen on:

- Branch: `codex/runtimeinspector-fab-rc`
- Product RC commit: `0a26995c1f057972bb175afa483326cb1c2df886`
- Summary: `Fix RuntimeInspector Fab plugin package build`

Repo-local package, automated blank-load validation, final listing screenshots, and a 35s H.264 demo have been regenerated. Do not claim final Fab submission readiness yet: the blank-host interaction smoke is still not human-signed, and `MarketplaceURL` remains a post-listing backend item.

## Current Stage Goal

Finish the remaining human blank-host smoke, then submit the already prepared package/media/checklist evidence to Fab without changing the product RC.

## Confirmed Facts

- Unique authority document directory is `docs/`.
- Current branch is `codex/runtimeinspector-fab-rc`.
- Product RC code was frozen and pushed.
- Package generation passed from the product RC:
  - `Saved/FabRelease/Package/RuntimeInspector_UE55/RuntimeInspector`
  - log: `Saved/build_runtimeinspector_fab_release.log`
- Blank-project automated load validation passed and preserved the validation host:
  - host: `Saved/FabRelease/BlankProjectValidation/RuntimeInspectorBlank_UE55/RuntimeInspectorBlank/RuntimeInspectorBlank.uproject`
  - log: `Saved/fab_blank_project_validation.log`
- Blank-host manual interaction smoke was attempted on 2026-06-04 but not signed:
  - the host opened to the UE splash/shader compile screen and was still compiling shaders after waiting
  - evidence screenshots:
    - `Saved/FabRelease/blank_manual_smoke_editor.png`
    - `Saved/FabRelease/blank_manual_smoke_editor_after_wait.png`
- Final listing images are in `FabMedia/`:
  - `cover.png`
  - `screenshot_01_actor_panel.png`
  - `screenshot_02_changes_workflow.png`
  - `screenshot_03_settings.png`
  - `screenshot_04_tools.png`
  - manifest: `FabMedia/fab_media_manifest.json`
- Media compliance passed locally:
  - all five final images are `1920x1080`, PNG, and under `3145728` bytes
- Demo media exists:
  - `FabMedia/demo.mp4`
  - ffprobe: H.264, `1920x1080`, `30 fps`, `35.000000s`, `734861` bytes
- Actor screenshot uses a real input-equivalent pick path:
  - `position_mouse_on_player_character`
  - `right_mouse_pick_input`
  - selected actor in the media run: `BP_ThirdPersonCharacter_C_0`

## Remaining Blockers

- Blank-host manual smoke still needs a human/operator PASS record after shader compilation finishes.
- `MarketplaceURL` must be filled after the Fab listing exists.
- Fab backend account, tax, payout, listing metadata, and review result are outside repo-local scope.

## Pending-Validation / Unknown

- `unknown`: whether the blank-host interaction smoke will reveal a packaged-install usability issue.
- `unknown`: whether Fab backend review will request media, metadata, or packaging changes.

## Next Smallest Closure Loop

1. Open the preserved blank validation host.
2. Wait for shader compilation to finish.
3. Perform the manual smoke in `docs/FAB_RC_SIGNOFF.md`.
4. If it passes, update the manual smoke record only; do not change product code.
5. Fill `MarketplaceURL` after the Fab listing exists.

## Authority Docs

- State authority: `docs/PROGRAM_STATE_UNIFIED.yaml`
- Task lanes: `docs/codex/tasks/TASK_LANE_INDEX.md`
- Development authority: `docs/AGENT_DEVELOPMENT.md`
- Troubleshooting reference: `docs/TROUBLESHOOTING.md`
- UI appendix: `docs/UI_GUARDRAILS.md`
