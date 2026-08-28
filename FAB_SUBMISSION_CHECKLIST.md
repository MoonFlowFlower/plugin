# Runtime Inspector Fab Submission Checklist

This checklist separates the repo-local technical RC from Fab backend and legal/account work. Development authority remains `docs/AGENT_DEVELOPMENT.md`.

## Technical RC Baseline

- Branch: `codex/runtimeinspector-fab-rc`
- Responsive runtime implementation: `6020b240bd635333102d77beb9829e62c1e7d8e6`
- Current validation harness baseline before the closing media/docs commit: `8a8872d44cc5d1cafe5dc35898b159bd2a5dbfa0`
- Engine: Unreal Engine 5.7 / Win64
- The final shipping commit and ZIP SHA-256 are recorded after the closing commit by `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`. They cannot be embedded in the ZIP without changing the bytes they identify.
- The upload artifact is the committed-HEAD source ZIP. The compiled package and blank host must be derived from that exact ZIP.

## Product And Packaging

- [x] Native dock tab pointer input no longer enters legacy drag/resize handling.
- [x] `TryGetTabButtonScreenCenterForAutomation` exists as a native automation-only interface; no user Blueprint API was added.
- [x] Tools definitions resolve from `IPluginManager::FindPlugin("RuntimeInspector")->GetBaseDir()`.
- [x] `Config/FilterPlugin.ini` explicitly packages both Tools JSON files.
- [x] `ui_readability` is included in `runtime_ui_contract_v1`.
- [x] Default typography uses title `12`, label/value `11`, muted `10`, and control height `28`.
- [x] Declared UI scale range is `0.8-1.5`; Fab default validation uses `UIScale=1.0`.
- [x] Plugin-local responsive scale is `UserUIScale / HostViewportDPI`; Runtime Inspector does not modify the host project's global DPI curve.
- [x] Side panels use `SizeBox` + `ScaleBox`; the central Overlay remains Fill, unscaled, and hit-test invisible.
- [x] Native dock adaptation uses layout containers, compact mode, scrolling, and ellipsis rather than Canvas fixed coordinates or reduced default fonts.
- [x] `responsive_dpi_layout` validates the formula, ScaleBox boundaries, physical panel/token metrics, tab rectangles, and resize updates.
- [x] `Scripts/TestFabArtifactContract.ps1` validates SourceSubmission and CompiledSmoke boundaries.
- [x] `Scripts/MakeFabSubmissionZip.ps1` builds the source ZIP from committed `HEAD` and rejects dirty shipping paths.
- [x] `Scripts/PackageFabRelease.ps1` compiles from the exact source ZIP.
- [x] `Scripts/ValidateFabBlankProject.ps1` creates `RIFabBlank`, reads UE 5.7 from `Build.version`, and treats plugin-consequential warnings as failures.
- [x] All release paths use `UE57`, not `UE55`.

## Directed And Closure Evidence

- [x] Directed dock tests: `panel_interaction`, `right_inspector_tabs`, `dock_layout`.
- [x] UI contract: `ui_readability` and `runtime_ui_contract_v1`.
- [x] Responsive matrix covers six viewport sizes and four user scales; default-scale physical tokens stay within `±1 px` or `±5%` rounding tolerance.
- [x] Real OS `O` input and real mouse tab clicks are required; automation/control opening is recovery only and cannot count as PASS.
- [x] Main project closure: `mainline_full_closure=PASS`, 24/24 checks.
- [x] Tools action binding contract: every configured/rendered action is bound (`203/203` in the final closure run).
- [x] Packaged loopback validation used a separately owned packaged-runtime listener on port `12098`.
- [x] Packaged-runtime stop scripts kill and verify the complete wrapper/child process tree.
- [x] Listing media regenerated from the current UE 5.7 UI.
- [x] Demo video is a live 44.033333-second H.264 real-input capture, not a screenshot sequence.
- [generated gate] The exact-artifact chain is passing only when the final source manifest, SourceSubmission/CompiledSmoke reports, Development/Shipping BuildPlugin logs, and blank-host smoke all name the same committed source ZIP.
- [generated gate] Exact ZIP-derived `RIFabBlank` must preserve real-`O`, four real-mouse tabs, normal/narrow layout, one Tools self-test, and one workflow evidence for the final manifest commit.
- [x] Repository, DocsURL, and Issues SupportURL were anonymously reachable on 2026-08-28; they must be checked again immediately before push.

Authoritative generated evidence paths:

- Source ZIP: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.zip`
- Source manifest: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`
- Source contract: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.source-contract.json`
- Compiled package: `../../Saved/FabRelease/Package/RuntimeInspector_UE57/RuntimeInspector`
- Contracts: `../../Saved/FabRelease/Contracts/RuntimeInspector_UE57/`
- Blank host: `../../Saved/FabRelease/BlankHostLoadSmoke/RIFabBlank_UE57/RIFabBlank/RIFabBlank.uproject`
- Blank-host log: `../../Saved/FabRelease/fab_blank_host_install_load_smoke_UE57.log`
- Exact blank-host responsive/real input: `Saved/RuntimeInspector/FinalExactBlankHostResponsive/`
- Main-project responsive closure: `Saved/RuntimeInspector/ResponsiveDPI/PluginMakerClosure/Final38b58ef/final-closure-summary.json`
- Main-project matrix: `Saved/RuntimeInspector/ResponsiveDPI/PluginMakerHorizontalFinal38b58ef/responsive-dpi-matrix.json` plus the clean real-input retry report
- Packaged matrix/lifecycle: generated under `Saved/RuntimeInspector/` by the final packaged-loopback run

## Listing And Media

- [x] Listing copy is prepared in `FAB_LISTING.md`.
- [x] README and Chinese user guide consistently state UE 5.7.
- [x] Six final stills are `1920x1080`, PNG, and below 3 MiB, including a responsive-layout comparison.
- [x] `FabMedia/demo.mp4` is H.264/yuv420p, `1920x1080`, `30 fps`, `44.033333s`, and `4199422` bytes.
- [x] Image/video hashes and capture provenance are recorded in `FabMedia/fab_media_manifest.json`.
- [x] Publisher metadata uses the canonical repository owner, `MoonFlowFlower`; the publisher profile returns HTTP 200.
- [x] `RuntimeInspector.uplugin` DocsURL and SupportURL point to the public canonical repository and Issues page; anonymous checks returned HTTP 200 on 2026-08-28.
- [ ] Replace `MarketplaceURL` only after the Fab listing exists.

## Final Release Gate

Run `Scripts/RunFab57Validation.cmd` only with Unreal Editor closed. It must execute, in order:

1. committed source ZIP plus SourceSubmission contract;
2. BuildPlugin from that ZIP plus CompiledSmoke contract;
3. `RIFabBlank` install/load smoke.

Do not call the result Fab-ready if any of these remain true:

- a tab can only be reached through console/automation rather than a real click;
- Tools JSON is missing, empty, or produces `[RI][ToolsConfig]` warnings;
- default visible body text falls below the readability contract;
- shipping paths are dirty when the ZIP is created;
- `mainline_full_closure` regresses;
- any public release surface still claims UE 5.5;
- DocsURL or SupportURL is not publicly reachable.

## External Submission Boundary

- [ ] Fab account, tax, payout, and backend listing metadata completed by the publisher.
- [ ] MarketplaceURL filled after the listing exists.
- [ ] Epic/Fab review completed.
- [ ] Asset-rights and licensing review completed by the rights holder.

Passing this checklist cannot guarantee Fab approval or every hardware/DPI configuration.
