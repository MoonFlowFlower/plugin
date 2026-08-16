# Runtime Inspector Fab Submission Checklist

This checklist separates the repo-local technical RC from Fab backend and legal/account work. Development authority remains `docs/AGENT_DEVELOPMENT.md`.

## Technical RC Baseline

- Branch: `codex/runtimeinspector-fab-rc`
- Implementation baseline: `ad6ddd9e8eebf1552f3aa18c76e795f37a10b47a`
- Engine: Unreal Engine 5.7 / Win64
- The final shipping commit is recorded by `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`; do not infer it from an older hard-coded SHA in this file.
- The upload artifact is the committed-HEAD source ZIP. The compiled package and blank host must be derived from that exact ZIP.

## Product And Packaging

- [x] Native dock tab pointer input no longer enters legacy drag/resize handling.
- [x] `TryGetTabButtonScreenCenterForAutomation` exists as a native automation-only interface; no user Blueprint API was added.
- [x] Tools definitions resolve from `IPluginManager::FindPlugin("RuntimeInspector")->GetBaseDir()`.
- [x] `Config/FilterPlugin.ini` explicitly packages both Tools JSON files.
- [x] `ui_readability` is included in `runtime_ui_contract_v1`.
- [x] Default typography uses title `12`, label/value `11`, muted `10`, and control height `28`.
- [x] Declared UI scale range is `0.8-1.5`; Fab default validation uses `UIScale=1.0`.
- [x] `Scripts/TestFabArtifactContract.ps1` validates SourceSubmission and CompiledSmoke boundaries.
- [x] `Scripts/MakeFabSubmissionZip.ps1` builds the source ZIP from committed `HEAD` and rejects dirty shipping paths.
- [x] `Scripts/PackageFabRelease.ps1` compiles from the exact source ZIP.
- [x] `Scripts/ValidateFabBlankProject.ps1` creates `RIFabBlank`, reads UE 5.7 from `Build.version`, and treats plugin-consequential warnings as failures.
- [x] All release paths use `UE57`, not `UE55`.

## Directed And Closure Evidence

- [x] Directed dock tests: `panel_interaction`, `right_inspector_tabs`, `dock_layout`.
- [x] UI contract: `ui_readability` and `runtime_ui_contract_v1`.
- [x] Main project closure: `mainline_full_closure=PASS`, 24/24 checks.
- [x] Tools action binding contract: all configured actions bound (`223/223` in the final main-project layout run).
- [x] Packaged loopback validation used a separately owned packaged-runtime listener on port `12098`.
- [x] Packaged-runtime stop scripts kill and verify the complete wrapper/child process tree.
- [x] Listing media regenerated from the current UE 5.7 UI.
- [x] Demo video is a live 43.833-second H.264 interaction capture, not a screenshot sequence.
- [ ] Final exact-HEAD source contract, BuildPlugin contract, and blank-host install/load report exist and pass after the final docs/media commit.
- [ ] Exact ZIP-derived blank host has a recorded real-mouse Actor/Changes/Settings/Tools click-through in normal and narrow/tall windows.
- [ ] Public DocsURL and SupportURL return HTTP 200.

Authoritative generated evidence paths:

- Source ZIP: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.zip`
- Source manifest: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`
- Source contract: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.source-contract.json`
- Compiled package: `../../Saved/FabRelease/Package/RuntimeInspector_UE57/RuntimeInspector`
- Contracts: `../../Saved/FabRelease/Contracts/RuntimeInspector_UE57/`
- Blank host: `../../Saved/FabRelease/BlankHostLoadSmoke/RIFabBlank_UE57/RIFabBlank/RIFabBlank.uproject`
- Blank-host log: `../../Saved/FabRelease/fab_blank_host_install_load_smoke_UE57.log`
- Main-project closure: `Saved/RuntimeInspector/Task5/mainline-full-closure-after-commits.json`
- Packaged ownership: `Saved/RuntimeInspector/Task5/packaged-port-ownership-green.json`
- Packaged lifecycle: `Saved/RuntimeInspector/Task5/packaged-run-stop-lifecycle-green.json`

## Listing And Media

- [x] Listing copy is prepared in `FAB_LISTING.md`.
- [x] README and Chinese user guide consistently state UE 5.7.
- [x] Five final stills are `1920x1080`, PNG, and below 3 MiB.
- [x] `FabMedia/demo.mp4` is H.264, `1920x1080`, `30 fps`, `43.833333s`, and `3509988` bytes.
- [x] Image/video hashes and capture provenance are recorded in `FabMedia/fab_media_manifest.json`.
- [ ] `RuntimeInspector.uplugin` SupportURL is publicly reachable. Current repository and Issues URLs returned HTTP 404 in an unauthenticated check on 2026-08-16; do not silently substitute an unrelated page.
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
