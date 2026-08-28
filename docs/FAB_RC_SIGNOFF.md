# Runtime Inspector Fab Technical RC Signoff

This document defines the technical release-candidate gate. It is not the development authority; use `docs/AGENT_DEVELOPMENT.md` for implementation rules.

## Baseline And Provenance

- Branch: `codex/runtimeinspector-fab-rc`
- Responsive runtime implementation: `6020b240bd635333102d77beb9829e62c1e7d8e6`
- Validation harness before the closing media/docs commit: `8a8872d44cc5d1cafe5dc35898b159bd2a5dbfa0`
- Target: Unreal Engine 5.7, Win64
- Upload bytes: committed-HEAD `RuntimeInspector-Fab-Submission.zip`
- Compiled smoke bytes: unpacked from that exact ZIP, never copied from the live working tree
- Final release commit and ZIP SHA-256 authority: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`

The closing commit cannot contain its own immutable Git hash or the hash of a ZIP that contains the closing document. The generated sidecar is therefore the non-self-referential authority. A stale or missing sidecar invalidates the technical-RC claim.

## Responsive UI Contract

- `EffectiveContentScale = UserUIScale / HostViewportDPI`.
- At `UIScale=1.0`, supported host resolution/DPI changes keep the plugin's physical fonts, controls, spacing, and target side-panel sizes within `±1 px` or `±5%` rounding tolerance.
- User `UIScale=0.8-1.5` scales Runtime Inspector only. The plugin does not modify project-global DPI and does not scale or replace the center gameplay viewport.
- Each side uses an outer responsive `SizeBox` and inner `ScaleBox`; the center Overlay remains Fill, unscaled, and hit-test invisible.
- Layout adaptation uses Overlay/HorizontalBox/VerticalBox/ScrollBox, compact mode, and ellipsis. Canvas fixed coordinates are not used for viewport adaptation; Canvas remains only for intrinsic 2D color controls.
- The native runtime UI remains native UMG rather than becoming a fully Designer-authored UMG Blueprint. No user Blueprint API, config-format change, or production dependency was added.

## Main-Project Evidence Before Artifact Freeze

- The 6x4 resolution/UIScale matrix passed all responsive layout rows.
- A clean real-input run passed OS `O` opening and real mouse Actor/Changes/Settings/Tools clicks at 1280x720 and 900x1200. Control/console switching is not accepted as a substitute.
- The targeted responsive/layout set passed 11/11.
- `runtime_ui_contract_v1` passed 19/19.
- `mainline_full_closure` passed 24/24.
- A property edit was staged and rendered in Changes; Tools ran `Dock Layout` and retained `mainline_safe_patch_core=PASS | Passed=6 Failed=0`.
- A long mixed session with two leftover packaged processes was rejected after UObject exhaustion. Stopping the owned packaged process tree and restarting Editor produced the passing clean run; final scripts must preserve lifecycle cleanup.

## Final Exact-Artifact Gate

The technical RC is closed only when all of the following generated evidence is present and passing for the same final committed HEAD:

1. `Scripts/RunFab57Validation.cmd` completes committed ZIP, exact-ZIP BuildPlugin, and `RIFabBlank` stages.
2. Source manifest reports clean shipping paths, exactly one `RuntimeInspector/` top-level directory, UE 5.7, entry count, commit, and ZIP SHA-256.
3. SourceSubmission and CompiledSmoke contract reports pass.
4. Development and Shipping BuildPlugin pass without new RuntimeInspector warnings.
5. `RIFabBlank` install/load passes without ToolsConfig, missing-config, module-load, or plugin-consequential warnings.
6. Exact ZIP-derived blank host passes the responsive 6x4 matrix for the engine-default DPI rule and matches the PluginMaker `UIScale=1.0` physical-token baseline.
7. Exact blank host passes real `O`, four real-mouse tabs after resize, one Tools self-test, and one workflow in normal and narrow/tall layouts.
8. Packaged loopback validation passes and the complete wrapper/child process tree is stopped and verified.
9. Repository, DocsURL, and SupportURL return anonymous HTTP 200 immediately before publication.
10. Remote `main` is updated only by an ordinary fast-forward; any non-fast-forward/protection failure stops release and force push is forbidden.

Expected evidence roots:

- Plugin-local submission: `Saved/FabRelease/Submission/`
- Project package/contracts/blank host: `../../Saved/FabRelease/`
- Main-project runtime reports: `Saved/RuntimeInspector/ResponsiveDPI/`
- Exact blank-host/packaged reports: `Saved/RuntimeInspector/`
- Final media: `FabMedia/`

## Media Signoff

- `FabMedia/cover.png`
- `FabMedia/screenshot_01_actor_panel.png`
- `FabMedia/screenshot_02_changes_workflow.png`
- `FabMedia/screenshot_03_settings.png`
- `FabMedia/screenshot_04_tools.png`
- `FabMedia/screenshot_05_responsive_layouts.png`
- `FabMedia/demo.mp4`
- `FabMedia/fab_media_manifest.json`

All six stills are 1920x1080 PNG files below 3 MiB. The demo is a real keyboard/mouse interaction capture: H.264/yuv420p, 1920x1080, 30 fps, 44.033333 seconds, 4199422 bytes. It is not a screenshot-sequence substitute.

## Public URL State

Anonymous checks on 2026-08-28 reached the public canonical repository, README DocsURL, and Issues SupportURL. This supersedes the historical 2026-08-16 404 blocker. URL availability is mutable, so the final pre-push check remains fail-closed.

## Claim Boundary

This signoff cannot prove Fab acceptance, every GPU/display/OS/DPI curve, third-party rights ownership, or future UE compatibility. MarketplaceURL, publisher account/tax/payout setup, pricing/licensing, and Epic review remain external.
