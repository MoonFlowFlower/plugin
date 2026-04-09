# Runtime Inspector Fab Listing Copy

This file is Fab listing copy, not the RuntimeInspector agent development authority.

For implementation rules and development workflow authority, use `Docs/AGENT_DEVELOPMENT.md`.

## Product Title

Runtime Inspector

## Short Description

Inspect runtime actors, edit properties, capture patch bundles, audit runtime-to-source differences, and run promote workflows in Unreal Engine 5.5.

## One-Line Positioning

Runtime-state inspection and controlled patch-to-source workflows for Unreal Engine projects that need fast in-editor verification.

## Feature Highlights

- Inspect live actors and supported runtime properties during PIE
- Capture snapshots, staged patches, exported patch bundles, and presets
- Audit runtime changes against source defaults before applying them
- Preview and run supported promote-to-source workflows from the editor
- Use built-in self-tests, verification profiles, and curated workflows
- Pull runtime patch changes back from loopback packaged sessions into the editor authority flow

## Long Description

Runtime Inspector is a UE 5.5 plugin focused on one core job: make runtime changes observable, reviewable, and recoverable.

Inside PIE, the plugin lets you inspect supported actor properties, apply edits, capture staged patches, save presets, and build audit views that show how runtime state differs from the source baseline. When a change is approved, editor-side promote workflows can preview or apply supported source updates without leaving the review loop.

The plugin also includes built-in self-tests and workflow runners so teams can validate their inspection and patch pipeline instead of treating it as an ad hoc debugging tool.

For advanced editor-authority setups, Runtime Inspector can discover loopback packaged runtime sessions, query targets, pull patch bundles back into the editor, and continue the same staged audit workflow there. Source authority remains in the editor.

## Supported Release Scope

- Unreal Engine 5.5
- Win64 first-release submission target
- Primary supported workflow: editor + PIE
- Advanced supported workflow: loopback packaged-runtime session pullback into editor authority

## Explicit Non-Scope

- Shipping-build runtime inspection
- Multi-machine runtime discovery
- LAN session discovery
- Direct source promote from packaged runtime

## Listing Media Shot List

1. Main inspector panel open on a selected actor in PIE
2. File page showing staged patch summary and compare rows
3. Settings page showing hotkey/outline/apply controls
4. Test page showing workflows or self-tests passing
5. Promote preview or applied audit result
6. Packaged-runtime session pullback flow on the File page

## Demo Video Outline

1. Start PIE and open the inspector
2. Pick an actor and edit one supported property
3. Stage the change and build a compare/audit report
4. Preview promote and apply from the editor
5. Show a packaged-runtime patch pull returning into the editor workflow

## Links

- Docs: <https://github.com/pen364692088/plugin#readme>
- Support: <https://github.com/pen364692088/plugin/issues>
- Publisher: <https://github.com/pen364692088>
