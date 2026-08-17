# Runtime Inspector Fab Technical RC Signoff

This document defines the technical release-candidate gate. It is not the development authority; use `docs/AGENT_DEVELOPMENT.md` for implementation rules.

## Baseline

- Branch: `codex/runtimeinspector-fab-rc`
- Implementation baseline: `a34a1e4ac3e190db14329cba9e216d591b0eb21d`
- Target: Unreal Engine 5.7, Win64
- Source of upload bytes: committed-HEAD `RuntimeInspector-Fab-Submission.zip`
- Source of compiled smoke bytes: that exact ZIP, not the working tree
- Final commit and ZIP SHA-256 authority: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`

## Verified Before Final Artifact Freeze

- Native dock tab pointer handlers yield to Slate/UMG buttons while legacy fallback drag/resize behavior remains tested.
- Real mouse input opened RuntimeInspector and switched Actor, Changes, Settings, and Tools in the main project.
- A runtime Transform edit was staged and rendered as an old/new Changes row.
- Tools displayed non-empty Tests/Workflows, ran `dock_layout`, and preserved the full `mainline_safe_patch_core=PASS | Passed=6 Failed=0` workflow identity.
- `mainline_full_closure` passed 24/24 after the final implementation commits.
- The exact committed source ZIP chain passed at `a34a1e4`: source contract, compiled contract, Development/Shipping BuildPlugin, and UE 5.7 blank-host install/load smoke.
- The exact ZIP-derived `RIFabBlank` passed real-`O` and real-mouse Actor/Changes/Settings/Tools click-through in normal and narrow/tall windows, including a Tools self-test and workflow.
- Packaged loopback validation used a process-owned listener on `127.0.0.1:12098`; editor and packaged listeners were not conflated.
- The packaged-runtime cleanup scripts removed both wrapper and child processes and verified no dedicated-package process remained.
- Five UE 5.7 screenshots and one live 41.933-second demo were visually inspected and normalized.

## Final Exact-Artifact Gate

The technical RC is closed only when all of the following generated evidence is present and passing for the final committed HEAD:

1. `Scripts/RunFab57Validation.cmd` passes all three stages.
2. Source manifest reports clean shipping paths, one `RuntimeInspector/` top-level directory, UE 5.7, entry count, commit, and ZIP SHA-256.
3. SourceSubmission and CompiledSmoke contract reports pass.
4. Development and Shipping BuildPlugin passes without new RuntimeInspector warnings.
5. `RIFabBlank` install/load smoke passes without ToolsConfig, missing-config, module-load, or plugin-consequential warnings.
6. In the exact ZIP-derived blank host, real mouse input opens the panel and reaches Actor/Changes/Settings/Tools in both normal and narrow/tall layouts.
7. A blank-host Tools self-test and one workflow run successfully.
8. Public documentation and support URLs return HTTP 200.

Expected evidence roots:

- Plugin-local submission: `Saved/FabRelease/Submission/`
- Project-level package/contracts/blank host: `../../Saved/FabRelease/`
- Main-project runtime reports: `Saved/RuntimeInspector/Task5/`
- Final media: `FabMedia/`

## Media Signoff

- `FabMedia/cover.png`
- `FabMedia/screenshot_01_actor_panel.png`
- `FabMedia/screenshot_02_changes_workflow.png`
- `FabMedia/screenshot_03_settings.png`
- `FabMedia/screenshot_04_tools.png`
- `FabMedia/demo.mp4`
- `FabMedia/fab_media_manifest.json`

All five images are `1920x1080` PNG files below 3 MiB. The demo is a live interaction capture, H.264, `1920x1080`, `30 fps`, `41.933008s`, `5066747` bytes. Idle-only gaps were removed and retained real-input segments use one uniform `1.5x` presentation rate; it is not a screenshot-sequence substitute.

## Current Stop Condition

An unauthenticated HTTP check on 2026-08-16 returned `404` for both:

- `https://github.com/pen364692088/plugin`
- `https://github.com/pen364692088/plugin/issues`

Therefore the descriptor/listing links cannot yet be signed as public. Do not make the repository public, create a replacement site, or point support to an unrelated page without publisher authorization. This blocks the `Fab-ready` claim and the final fast-forward of `main`, but it does not invalidate local technical evidence. The RC branch may be pushed only after `Scripts/RunFab57Validation.cmd` passes from the exact closing commit.

## Claim Boundary

This signoff cannot prove Fab acceptance, all GPUs/DPIs, third-party rights ownership, or future UE-version compatibility. MarketplaceURL, publisher account/tax/payout setup, and Epic review remain external.
