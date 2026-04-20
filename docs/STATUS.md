# RuntimeInspector Status

## Current Conclusion
The highest-value Fab blocker is no longer transform persistence.
`SceneComponent RelativeLocation / RelativeRotation / RelativeScale3D -> Stage -> Apply To Source -> next PIE persists`
now has real passing evidence on the current dirty local baseline.

The release is still not ready for Fab submission because the RC baseline and release evidence are inconsistent:
- no new clean RC commit has been frozen
- blank-host manual smoke has not been re-signed on the current baseline
- demo media is still missing
- package/signoff evidence is still tied to an older RC, and final Fab media still needs to be regenerated from that clean RC

## Current Stage Goal
Freeze a clean Fab RC from the current branch state, then regenerate package, manual smoke, screenshots, and demo media from that same commit.

## Confirmed Facts
- Unique authority document directory is `docs/`.
- New-session default entrypoints are:
  - `docs/PROGRAM_STATE_UNIFIED.yaml`
  - `docs/STATUS.md`
  - `docs/codex/tasks/TASK_LANE_INDEX.md`
- Current branch is `codex/runtimeinspector-fab-rc`.
- Git `HEAD` is still `7e471dd Tighten actor rows and fix actor search`.
- Worktree is dirty and includes many local code/UI/doc changes beyond a single focused fix.
- `PluginMakerEditor Win64 Development` built successfully after the transform persistence fixes.
- The current RC candidate is now staged as a single boundary and passes `git diff --cached --check`.
- `PluginMakerEditor Win64 Development` editor build preflight also passes on that same staged RC candidate.
- Real external validation now passes for transform source persistence:
  - `Saved/RuntimeInspector/Validation/TransformSourcePersistence/BC8B047E-400B-566E-1347-2DA5AA231920/prepare_report.json`
  - `Saved/RuntimeInspector/Validation/TransformSourcePersistence/BC8B047E-400B-566E-1347-2DA5AA231920/verify_restore_report.json`
- The validated runtime/source target is `BP_TestVarsActor -> Cube (StaticMeshComponent)`.
- The previous remaining false negative was not a source-promote failure:
  - `BP_TestVarsActor` drives `SetActorScale3D(TestScale)` from `Event Tick`
  - actor-root scale on that test Blueprint is therefore not a stable persistence acceptance target
- `Actor` page validation also still passes on the current dirty local baseline, including the dedicated top `Actor Transform` block:
  - `Saved/RuntimeInspector/Validation/actor_page_structure.png`
- Fab package generation and automated blank-project load validation also pass on the current dirty local baseline:
  - package root: `Saved/FabRelease/Package/RuntimeInspector_UE55/RuntimeInspector`
  - blank host: `Saved/FabRelease/BlankProjectValidation/RuntimeInspectorBlank_UE55/RuntimeInspectorBlank/RuntimeInspectorBlank.uproject`
  - automated load log: `Saved/fab_blank_project_validation.log`
- Deterministic Fab media capture now also passes on the current dirty local baseline:
  - output root: `Saved/RuntimeInspector/FabMediaCapture/`
  - capture log: `Saved/RuntimeInspector/FabMediaCapture/capture_fab_media.log`
  - capture manifest: `Saved/RuntimeInspector/FabMediaCapture/capture_manifest.txt`
- Required listing shots were visually spot-checked on the refreshed local staging set:
  - `cover.png`
  - `screenshot_01_actor_panel.png`
  - `screenshot_02_changes_workflow.png`
  - `screenshot_03_settings.png`
  - `screenshot_04_tools.png`

## Remaining Blockers
- RC baseline is not frozen on a clean commit yet.
- `FAB_SUBMISSION_CHECKLIST.md` and `docs/FAB_RC_SIGNOFF.md` still describe an older RC baseline.
- Packaged-install manual smoke on the preserved blank host still needs a current-baseline human signoff record.
- The preserved blank host contains only the packaged `RuntimeInspector` plugin; it does not include bridge automation hooks, so this remaining smoke step is genuinely human/manual.
- Demo video or GIF is still missing.
- `MarketplaceURL` is still a post-listing follow-up, not yet filled.

## Pending-Validation / Unknown
- `unknown`: whether any remaining Fab readiness issue will come from packaging/manual smoke rather than product behavior.

## Highest-Value Recent Change
- The transform persistence validation chain is now real and repeatable:
  1. prepare self-test mutates/stages/promotes component transform
  2. PIE restarts on the same local baseline
  3. verify/restore self-test confirms persisted source + restored cleanup
- The validation script is:
  - `Scripts/ValidateTransformSourcePersistence.ps1`

## Next Smallest Closure Loop
1. Freeze a clean RC commit from `codex/runtimeinspector-fab-rc`.
2. Regenerate the release package from that exact commit.
3. Re-run preserved blank-host manual smoke and fill the signoff record.
4. Re-run the deterministic screenshot capture on that same RC and copy the approved final media.
5. Record the short demo video/GIF.
6. After Fab listing creation, fill `MarketplaceURL` and do final signoff.

## Authority Docs
- State authority: `docs/PROGRAM_STATE_UNIFIED.yaml`
- Task lanes: `docs/codex/tasks/TASK_LANE_INDEX.md`
- Development authority: `docs/AGENT_DEVELOPMENT.md`
- Troubleshooting reference: `docs/TROUBLESHOOTING.md`
- UI appendix: `docs/UI_GUARDRAILS.md`
- Historical handoff only: `docs/HANDOFF_*.md`
