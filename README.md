# Runtime Inspector

This file is a user and maintainer overview, not the agent development authority.

For RuntimeInspector implementation rules, validation boundaries, and the current development workflow, use `docs/AGENT_DEVELOPMENT.md`.

Runtime Inspector is an Unreal Engine 5.5 plugin for inspecting live actors and editable properties, capturing runtime changes as patch bundles, comparing runtime state against source defaults, and running editor-side audit and promote workflows.

## Documentation

- User overview: `README.md`
- Chinese product guide: `USER_GUIDE_zh-CN.md`
- Program state (structured): `docs/PROGRAM_STATE_UNIFIED.yaml`
- Current status: `docs/STATUS.md`
- Task lanes: `docs/codex/tasks/TASK_LANE_INDEX.md`
- Agent development authority: `docs/AGENT_DEVELOPMENT.md`
- UI appendix: `docs/UI_GUARDRAILS.md`

## Release Scope

- Supported engine: Unreal Engine 5.5
- Tested platform: Win64
- Primary supported workflow: local editor and PIE inspection
- Advanced workflow: loopback packaged-runtime session discovery and patch pull from the editor authority side
- Shipping builds are intentionally disabled via `RUNTIME_INSPECTOR_ENABLED=0`

## What The Plugin Does

- Inspect the currently selected or picked runtime actor
- Browse and edit supported runtime properties
- Capture snapshots, staged patches, exported patch bundles, and presets
- Compare runtime changes against source defaults and audit the delta
- Preview and apply supported promote-to-source operations from the editor
- Run built-in self-tests, verification profiles, and curated workflows

## What The Plugin Is Not

- Not a general-purpose remote debugger for arbitrary projects or networks
- Not a replacement for Unreal's editor asset tooling
- Not a multi-machine discovery tool
- Not a packaged-build source-authority tool; source promote remains editor-only

## Installation

1. Copy the `RuntimeInspector` plugin folder into your project's `Plugins/` directory.
2. Open the Unreal Engine 5.5 project.
3. Enable `Runtime Inspector` in the Plugins browser if it is not already enabled.
4. Restart the editor when prompted.
5. Confirm the settings page exists under `Project Settings -> Plugins -> Runtime Inspector`.

## Quick Start

1. Start PIE.
2. Press `O` to toggle the inspector panel.
3. Hover the mouse over the target actor and press `P`, or use `Ctrl + Right Mouse Button` to pick the actor currently hit under the mouse. Picking uses object queries, so both player characters and scene actors can be selected when they are under the cursor.
4. Use the actor/property panel to inspect or edit supported values.
5. Use the `Changes` page for staged patch, preset, audit, compare, and promote workflows.
6. Use the `Settings` page to rebind hotkeys, switch theme preset, and adjust outline/apply behavior.
7. Use the `Tools` page to run self-tests, curated workflows, remote packaged validation, and diagnostics.

## Supported Workflows

### Editor / PIE

- Runtime actor inspection and property editing
- Snapshot and staged patch workflows
- Patch export and preset save/apply
- Patch-vs-source and applied audit views
- Promote preview and supported editor-side promote apply
- File page compare, audit, and report export flows

### Packaged Runtime Session

- Loopback-only packaged session discovery from the editor
- Remote target listing and scoped target queries
- Remote runtime property mutation and patch pull back into the editor
- Editor-side workflow execution against a selected remote session

## Limitations

- UE 5.5 only for this release
- Win64 is the supported submission target for the first Fab release
- Loopback packaged session support is editor-authority only
- Packaged runtime sessions do not support source promote directly
- Test/sample assets are included because some built-in workflows and self-tests depend on them

## Support

- Documentation: <https://github.com/pen364692088/plugin#readme>
- Support and bug reports: <https://github.com/pen364692088/plugin/issues>

## Release And Validation Shortcuts

Use the release packaging script below to produce a clean Fab submission package from a staged copy of the plugin instead of packaging directly from the live repo checkout:

- `Scripts\\PackageFabRelease.cmd`
- `Scripts\\PackageFabRelease.ps1`

The release packaging script first rebuilds the stable `PluginMakerEditor Win64 Development` binary from the host project, then runs `BuildPlugin -NoHostPlatform`, and finally assembles a clean Fab package that keeps:

- the validated `Intermediate` precompile data emitted by `BuildPlugin`
- the stable editor-side precompiled `Binaries` required for a blank UE 5.5 blueprint project to load the plugin without a local C++ rebuild

Use the blank-project validation scripts below to install the packaged plugin into a generated minimal UE 5.5 host project and verify the plugin mounts cleanly before submission. This host is only for install/load smoke validation and is not used for bridge-driven runtime acceptance:

- `Scripts\\ValidateFabBlankProject.cmd`
- `Scripts\\ValidateFabBlankProject.ps1`

If you want to keep the generated validation host for manual smoke checks, run the validation script with `-KeepValidationProject` and then open it with:

- `Scripts\\OpenFabValidationProject.cmd`
- `Scripts\\OpenFabValidationProject.ps1`

Use the main `PluginMaker` project for loopback packaged-runtime acceptance and bridge-driven runtime workflows:

- `Scripts\\BuildPackagedRuntimeValidation.cmd`
- `Scripts\\RunPackagedRuntimeValidation.cmd`
- `Scripts\\StopPackagedRuntimeValidation.cmd`

Use the main editor screenshot helper below to open or normalize the clean Fab presentation state:

- `Scripts\\OpenFabScreenshotState.cmd`
- `Scripts\\OpenFabScreenshotState.ps1`

Use the automated Fab media capture helper to stage deterministic screenshots into the local capture output folder before copying the approved files into `FabMedia/`:

- `Scripts\\CaptureFabMedia.cmd`
- `Scripts\\CaptureFabMedia.ps1`
- `Saved\\RuntimeInspector\\FabMediaCapture\\`
