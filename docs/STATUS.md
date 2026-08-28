# RuntimeInspector Status

## Current Conclusion

RuntimeInspector's responsive UE 5.7 implementation and main-project gates are verified on `codex/runtimeinspector-fab-rc`.

- User-facing responsive implementation: `6020b240bd635333102d77beb9829e62c1e7d8e6`.
- Validation harness before the closing media/docs commit: `8a8872d44cc5d1cafe5dc35898b159bd2a5dbfa0`.
- Final release commit and source-ZIP SHA-256 are intentionally not hard-coded into files inside that ZIP. The post-commit authority is `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json` together with its contract/build/blank-host reports.
- If that generated evidence is missing, failing, or names a different commit/ZIP, the current checkout is not a technical RC regardless of this document.
- The canonical repository, README DocsURL, and Issues SupportURL were anonymously reachable on 2026-08-28. This removed the historical 404 blocker, but the URLs remain a fail-closed pre-push check.

## Confirmed Facts

- At `UIScale=1.0`, Runtime Inspector uses `UserUIScale / HostViewportDPI` to keep plugin fonts, controls, spacing, and target side-panel sizes on the same screen-pixel baseline; it does not change global project DPI or scale the central viewport.
- Side panels use responsive `SizeBox`/`ScaleBox` boundaries and container layout. Narrow space uses compact mode, scrolling, or ellipsis rather than Canvas fixed-position adaptation or smaller default fonts.
- Base typography remains title `12`, label/value `11`, muted `10`, and control height `28`.
- Native dock tabs accept real pointer clicks; legacy fallback drag/resize remains isolated.
- Tools definitions are plugin-relative, packaged, parseable, non-empty, and action-bound.
- Six resolutions by four `UIScale` values passed the responsive layout/self-test rows. A clean OS-input retry passed real `O` plus four mouse tabs at normal and narrow/tall sizes.
- The current main-project targeted set passed 11/11, `runtime_ui_contract_v1` passed 19/19, and `mainline_full_closure` passed 24/24.
- A long contaminated session with two leftover packaged-runtime processes eventually exhausted Unreal's UObject limit. After the owned packaged process tree was stopped and Editor restarted, the clean responsive real-input run passed. This is environment/lifecycle evidence, not proof that unbounded mixed validation sessions are safe.
- Fab media contains six UE 5.7 stills and a real 44.033333-second operation video; exact hashes are in `FabMedia/fab_media_manifest.json`.

## Generated Release Gate

A release is technically closed only when all of these are fresh for the same final commit:

1. committed source ZIP with clean shipping paths;
2. SourceSubmission and CompiledSmoke artifact contracts;
3. UE 5.7 Development and Shipping BuildPlugin;
4. exact ZIP-derived `RIFabBlank` install/load and consequential-warning scan;
5. blank-host real `O`, four real mouse tabs, Tools self-test/workflow, and responsive normal/narrow evidence;
6. packaged loopback run plus verified process-tree cleanup;
7. final anonymous HTTP 200 checks and ordinary fast-forward-only publication.

## Authority And Evidence

- State contract: `docs/PROGRAM_STATE_UNIFIED.yaml`
- Technical gate: `docs/FAB_RC_SIGNOFF.md`
- Submission checklist: `FAB_SUBMISSION_CHECKLIST.md`
- Submission manifest: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`
- Main-project closure: `Saved/RuntimeInspector/ResponsiveDPI/PluginMakerClosure/Final38b58ef/final-closure-summary.json`
- Responsive matrices: `Saved/RuntimeInspector/ResponsiveDPI/`
- Final exact blank-host and packaged evidence: generated under `Saved/RuntimeInspector/` and `../../Saved/FabRelease/`
- Media manifest: `FabMedia/fab_media_manifest.json`

## Claim Boundary

The enumerated matrix cannot prove every hardware, OS scale, third-party DPI curve, or future UE version. Local technical evidence cannot guarantee Fab approval or asset-rights ownership. MarketplaceURL and Fab account/tax/payout/licensing/review work remain external.
