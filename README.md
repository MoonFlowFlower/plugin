# Runtime Inspector

Runtime Inspector is an Unreal Engine 5.7 plugin for inspecting live actors and editable properties during PIE, capturing runtime changes as staged patches, comparing runtime state against source defaults, and applying approved changes back to source through controlled editor-side workflows.

## Requirements

- Unreal Engine 5.7
- Windows (Win64)

## Installation

1. Copy the `RuntimeInspector` plugin folder into your project's `Plugins/` directory (or install from Fab via the Epic Games Launcher).
2. Open your Unreal Engine 5.7 project.
3. Enable `Runtime Inspector` in the Plugins browser if it is not already enabled, and restart the editor when prompted.
4. Confirm the settings page exists under `Project Settings -> Plugins -> Runtime Inspector`.

## Quick Start

1. Start PIE.
2. Press `O` to toggle the inspector panel.
3. Hover the mouse over a target actor and press `P`, or use `Ctrl + Right Mouse Button` to pick the actor under the cursor. Picking uses object queries, so both player characters and scene actors can be selected.
4. Use the `Actor` page to inspect or edit supported values.
5. Use the `Changes` page for staged patch, preset, audit, compare, and promote workflows.
6. Use the `Settings` page to rebind hotkeys, switch the theme preset, and adjust outline/apply behavior.
7. Use the `Tools` page to run built-in self-tests, curated workflows, and diagnostics.

## Responsive UI And DPI

- At the default `UIScale=1.0`, Runtime Inspector normalizes the host viewport DPI locally so its fonts, controls, panel widths, and spacing keep the same screen-pixel baseline across supported resolutions and project DPI curves.
- `UIScale` is the plugin's explicit user scale and supports `0.8-1.5`. Values other than `1.0` scale only Runtime Inspector; the plugin never changes the project's global DPI settings or the central gameplay viewport.
- The dock uses anchored/container layout (`Overlay`, `HorizontalBox`, `VerticalBox`, `ScrollBox`, and `ScaleBox`) rather than Canvas fixed coordinates. The center remains fill/pass-through, narrow windows compact the left panel, and overflow stays reachable through scrolling or ellipsis instead of shrinking the default fonts.
- The native runtime UI is intentionally not converted into a fully Designer-authored UMG asset. This keeps the existing runtime state and controller authority while still providing a maintainable responsive layout boundary.

## What The Plugin Does

- Inspect the currently selected or picked runtime actor
- Browse and edit supported runtime properties, including material parameters
- Capture snapshots, staged patches, exported patch bundles, and presets
- Compare runtime changes against source defaults and audit the delta
- Preview and apply supported promote-to-source operations from the editor
- Discover loopback packaged-runtime sessions from the editor and pull patch data back into the editor review flow (advanced)
- Run built-in self-tests, verification profiles, and curated workflows

## What The Plugin Is Not

- Not a general-purpose remote debugger for arbitrary projects or networks
- Not a replacement for Unreal's editor asset tooling
- Not a multi-machine discovery tool; packaged-session support is loopback (same machine) only
- Not a packaged-build source-authority tool; source promote remains editor-only

## Limitations

- Unreal Engine 5.7, Win64 for this release
- Inspector UI is intended for Editor/PIE and development builds; it is disabled in Shipping builds by design (`RUNTIME_INSPECTOR_ENABLED=0`)
- Packaged runtime sessions do not support direct source promote
- The release matrix covers `1280x720`, `1600x900`, `1920x1080`, `2560x1440`, `3840x2160`, and `900x1200` at `UIScale=0.8/1.0/1.25/1.5`; it does not prove every OS, display, or third-party DPI curve.

## Notes

- `Content/Test/` contains the fixture assets (`BP_TestVarsActor`, `M_Test`, `MI_Test`) used by the built-in self-tests on the Tools page. They are required for that feature and are not demo leftovers.
- Optional security: enable `Require Unlock` in the plugin settings to keep the panel locked until `ri.Unlock <code>` is entered in the console.

## Documentation And Support

- Chinese product guide: `USER_GUIDE_zh-CN.md` (included with the plugin)
- Online documentation: <https://github.com/MoonFlowFlower/plugin#readme>
- Support and bug reports: <https://github.com/MoonFlowFlower/plugin/issues>
