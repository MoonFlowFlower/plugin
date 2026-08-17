# RuntimeInspector Status

## Current Conclusion

RuntimeInspector is at a UE 5.7 Fab technical RC on `codex/runtimeinspector-fab-rc`.

- Implementation baseline: `a34a1e4ac3e190db14329cba9e216d591b0eb21d`
- Tasks 0-4 are independently committed; Task 5 implementation, exact-artifact, real-input, packaged-loopback, documentation, and media evidence is technically closed.
- The exact source/compiled/blank-host chain passes from the closing commit and ZIP recorded in `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`.
- The RC branch is published after that exact chain passes; remote `main` remains intentionally unchanged until the public URL gate passes.
- Fab-ready must not be claimed while the public documentation/support URLs return HTTP 404.

## Confirmed Facts

- Native dock tabs accept real pointer clicks; legacy fallback drag/resize remains isolated.
- Tools definitions are plugin-relative, packaged, parseable, non-empty, and action-bound.
- Default typography/readability and UI scale are covered by `ui_readability` and `runtime_ui_contract_v1`.
- `mainline_full_closure` passed 24/24 after the implementation commits.
- Packaged loopback validation distinguished editor port `12097` from packaged-runtime port `12098` and verified process ownership.
- Packaged wrapper/child cleanup is enforced and verified.
- The exact ZIP-derived blank host passed real `O`, four real-mouse tabs, one Tools self-test, one workflow, and normal plus narrow/tall layouts.
- Fab media now contains five current UE 5.7 stills and a real 41.933-second operation video with final workflow identity visible.
- Upload/source bytes, compiled smoke bytes, and blank-host bytes are designed to descend from one committed source ZIP.

## Active Blockers

1. Make the canonical `MoonFlowFlower/plugin` documentation and support destinations publicly reachable, or provide publisher-approved replacements. On 2026-08-16 the repository and Issues URLs returned HTTP 404 without authentication; the publisher profile itself returned HTTP 200.
2. Keep remote `main` unchanged until both public URLs return HTTP 200.

## Next Smallest Closure Loop

1. Resolve the public URL blocker without substituting an unrelated destination.
2. Recheck DocsURL, SupportURL, the source manifest commit, ZIP SHA-256, and remote RC SHA.
3. Confirm remote `main` is still an ancestor of the verified RC commit.
4. Only then fast-forward remote `main`; stop on any non-fast-forward or protection rejection and never force push.
5. If any shipping-path file changes before that push, regenerate the complete exact-artifact chain first.

## Authority And Evidence

- State authority: `docs/PROGRAM_STATE_UNIFIED.yaml`
- Technical signoff: `docs/FAB_RC_SIGNOFF.md`
- Submission checklist: `FAB_SUBMISSION_CHECKLIST.md`
- Submission manifest: `Saved/FabRelease/Submission/RuntimeInspector-Fab-Submission.manifest.json`
- Exact blank-host real input: `Saved/RuntimeInspector/Task5/FinalExactBlankHostRC/final-exact-blank-host-real-input.json`
- Main closure: `Saved/RuntimeInspector/Task5/mainline-full-closure-final-a34.json`
- Packaged matrix: `Saved/RuntimeInspector/Task5/PackagedGreenA34/packaged-loopback-matrix-final-a34-green.normalized.json`
- Media manifest: `FabMedia/fab_media_manifest.json`

## Claim Boundary

Local technical evidence cannot guarantee Fab approval, all hardware/DPI combinations, asset-rights ownership, or future engine compatibility. MarketplaceURL and Fab account/tax/payout/review work remain external.
