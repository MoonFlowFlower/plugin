# RuntimeInspector Status

## Current Conclusion

The current Fab product RC baseline has moved past the superseded package-build RC:

- Branch: `codex/runtimeinspector-fab-rc`
- Product RC commit: `88dacaae56b5aa834ec435c2c5d0d5ba91f73762`
- Summary: `Fix RuntimeInspector blank host dock scaling`
- Superseded RC: `0a26995c1f057972bb175afa483326cb1c2df886`
  - reason: blank validation host revealed the dock UI could render too small under editor PIE DPI scaling.

Repo-local package generation, automated blank-load validation, DPI-aware dock layout validation, final listing screenshots, and a 35s H.264 demo have been regenerated from the new RC state. Do not claim final Fab submission readiness yet: full human blank-host multi-tab smoke is still not signed, and `MarketplaceURL` remains a post-listing backend item.

## Current Stage Goal

Finish the remaining human blank-host tab smoke, then submit the prepared package/media/checklist evidence to Fab without changing the product RC unless that smoke finds a product issue.

## Confirmed Facts

- Unique authority document directory is `docs/`.
- Current branch is `codex/runtimeinspector-fab-rc`.
- Product RC code was patched for blank-host DPI scaling and committed.
- Package generation passed from the new product RC:
  - `Saved/FabRelease/Package/RuntimeInspector_UE55/RuntimeInspector`
  - log: `Saved/build_runtimeinspector_fab_release.log`
- Blank-project automated load validation passed and preserved the validation host:
  - host: `Saved/FabRelease/BlankProjectValidation/RuntimeInspectorBlank_UE55/RuntimeInspectorBlank/RuntimeInspectorBlank.uproject`
  - log: `Saved/fab_blank_project_validation.log`
- Blank-host packaged PIE/open/readability smoke has direct screenshot evidence:
  - `Saved/FabRelease/blank_host_dpi_fix_printwindow.png`
  - `Saved/FabRelease/blank_host_pid_48944.png`
  - scope: packaged blank host entered PIE, RuntimeInspector opened, selected actor context rendered, dock side panels were readable instead of physically tiny.
- Full blank-host manual tab smoke remains pending:
  - `Actor / Changes` have blank-host visual evidence.
  - `Settings / Tools` still need a human/operator click-through in the blank host; OS coordinate automation was not reliable enough to sign this.
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
  - ffprobe: H.264, `1920x1080`, `30 fps`, `35.000000s`, `810418` bytes
- Actor screenshot uses a real input-equivalent pick path:
  - `position_mouse_on_player_character`
  - `right_mouse_pick_input`
  - selected actor in the media run: `BP_ThirdPersonCharacter_C_0`
- `Scripts/CaptureFabMedia.ps1` now guards against foreground-window pollution by selecting the largest valid target window and temporarily placing it topmost before Win32 capture.

## Remaining Blockers

- Full blank-host manual tab smoke still needs a human/operator PASS record.
- `MarketplaceURL` must be filled after the Fab listing exists.
- Fab backend account, tax, payout, listing metadata, and review result are outside repo-local scope.

## Pending-Validation / Unknown

- `unknown`: whether full blank-host manual tab smoke will reveal a packaged-install usability issue outside the already fixed scaling problem.
- `unknown`: whether Fab backend review will request media, metadata, or packaging changes.

## Next Smallest Closure Loop

1. Open the preserved blank validation host.
2. Enter PIE if it is not already playing.
3. Open RuntimeInspector and click through `Actor / Changes / Settings / Tools`.
4. If it passes, update only the manual smoke record; do not change product code.
5. Fill `MarketplaceURL` after the Fab listing exists.

## Authority Docs

- State authority: `docs/PROGRAM_STATE_UNIFIED.yaml`
- Task lanes: `docs/codex/tasks/TASK_LANE_INDEX.md`
- Development authority: `docs/AGENT_DEVELOPMENT.md`
- Troubleshooting reference: `docs/TROUBLESHOOTING.md`
- UI appendix: `docs/UI_GUARDRAILS.md`
