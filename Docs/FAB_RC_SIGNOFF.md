# Runtime Inspector RC Signoff

This file is the release-candidate signoff guide for the current Fab submission pass.

It is not the development authority. For implementation rules, use `Docs/AGENT_DEVELOPMENT.md`.

## RC Baseline

- Branch: `codex/runtimeinspector-fab-rc`
- Commit: `1970ee707288bff2288785c7ca73981da89b6507`
- Package root:
  - `Saved/FabRelease/Package/RuntimeInspector_UE55/RuntimeInspector`
- Blank-install manual smoke host:
  - `Saved/FabRelease/BlankProjectValidation/RuntimeInspectorBlank_UE55/RuntimeInspectorBlank/RuntimeInspectorBlank.uproject`
- Screenshot set:
  - `Saved/RuntimeInspector/FabMediaCapture/`

All manual smoke notes, screenshots, and demo media must match this RC baseline. If code changes after this commit, regenerate package, validation evidence, and media before signoff.

## Manual Smoke

Use the preserved blank-install host project. Do not use the main `PluginMaker` project for this signoff.

### Launch

1. Open:
   - `Saved/FabRelease/BlankProjectValidation/RuntimeInspectorBlank_UE55/RuntimeInspectorBlank/RuntimeInspectorBlank.uproject`
2. Wait for the editor to finish loading.
3. Confirm the plugin is mounted and no startup error dialog appears.

### Minimum Path

1. Open Runtime Inspector.
2. Select a test actor in the level.
3. Verify `Actor` page opens and shows selection + component/property content.
4. Switch to `Changes` and confirm the page opens without embedded settings content.
5. Switch to `Settings` and confirm it is the only highlighted settings-style tab.
6. Switch to `Tools` and confirm it is the only highlighted tools-style tab.
7. Return to `Actor` and confirm basic interaction still works.

### Pass Criteria

- Panel opens successfully.
- Actor selection appears.
- `Actor / Changes / Settings / Tools` all open.
- No page is blank, blocked, or misrouted.
- No double-highlighted tabs.
- No blocking modal/input leak after normal use.

### Record

- Date:
- Operator:
- Result: `PASS` / `FAIL`
- Notes:

## Demo Capture

Use the RC baseline above. If the UI or workflow changes, re-record.

### Target Length

- 30 to 60 seconds

### Shot Order

1. Start PIE and open Runtime Inspector.
2. Select an actor and edit one supported property.
3. Show the staged change on `Changes`.
4. Show compare/audit or promote preview.
5. Show `Settings` briefly to establish the distinct settings page.
6. Show `Tools` briefly with workflow/self-test capability.
7. Show packaged-runtime pullback on `Changes` if the shot remains readable.

### Demo Rules

- Keep one clear idea per shot.
- Do not show internal dev-only logs unless they support a product-facing capability.
- Keep tab highlights correct and page routing obvious.
- Prefer the current deterministic screenshot state/theme when possible.

### Record

- Video or GIF file:
- Captured from commit:
- Checked by:
- Result: `PASS` / `FAIL`
- Notes:
