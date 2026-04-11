# Runtime Inspector Fab Listing Copy

This file is Fab listing copy, not the RuntimeInspector agent development authority.

For implementation rules and development workflow authority, use `Docs/AGENT_DEVELOPMENT.md`.

## Product Title

Runtime Inspector

## Short Description

Inspect live actors in PIE, review staged runtime changes, and move approved patch data back through controlled editor workflows in Unreal Engine 5.5.

## One-Line Positioning

Runtime-state inspection and controlled review workflows for Unreal Engine teams that need fast in-editor verification without losing source authority.

## Feature Highlights

- Inspect selected live actors and supported runtime properties during PIE
- Stage runtime edits into reviewable patch data instead of relying on ad hoc debugging
- Compare runtime state against source defaults before previewing or applying approved updates
- Run built-in self-tests, verification profiles, and curated workflows inside the same tool
- Pull patch data back from supported loopback packaged-runtime sessions into the editor review flow

## Long Description

Runtime Inspector is a UE 5.5 plugin for teams that need to inspect live gameplay state, make controlled runtime edits, and keep review authority in the editor.

Inside PIE, you can select a live actor, inspect supported properties, apply edits, stage those edits into patch data, and audit runtime state against the source baseline before moving forward. The workflow is designed for verification and review, not blind mutation.

Runtime Inspector also includes built-in self-tests, verification profiles, and guided workflows so teams can validate the tool and the surrounding inspection pipeline instead of relying on one-off editor debugging habits.

For advanced setups, the plugin can discover supported loopback packaged-runtime sessions, query patch targets, pull runtime patch data back into the editor, and continue the same staged audit process there. Source authority remains in the editor workflow.

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
- General-purpose remote debugging

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
