# Runtime Inspector Fab Asset Checklist

This file is a Fab asset checklist, not the RuntimeInspector agent development authority.

For implementation rules and development workflow authority, use `Docs/AGENT_DEVELOPMENT.md`.

Capture only shipped UE 5.5 functionality. Avoid internal-only test views, debug logs, or obviously experimental setups.

## Cover Image

- Product name visible: `Runtime Inspector`
- Clear Unreal Editor context
- Show the inspector panel with readable actor/property content
- Prefer a clean scene with one selected actor

## Screenshot Shot List

1. Main actor/property inspection view
   Show the live property panel with a selected actor and readable values.

2. File workflow view
   Show staged patch or compare output with a concise, understandable diff summary.

3. Source workflow view
   Show promote/audit related UI that communicates runtime-to-source workflow.

4. Remote session workflow
   Show remote session selection or packaged-runtime workflow entry point.

5. Test/workflow page
   Show the curated workflow list, not internal raw diagnostics.

## Demo Video / GIF

- Start from enabling or opening the tool
- Select an actor
- Change a runtime value
- Show staged patch / compare
- Show editor-side workflow or verification result
- Keep it short, around 20-45 seconds

## Consistency Rules

- Use UE 5.5
- Use the same plugin name, wording, and support URLs as `RuntimeInspector.uplugin` and `FAB_LISTING.md`
- Do not show unreleased features
- Do not show broken test assets, compile logs, or temporary internal notes

## Staging Folder

- Final deliverables should be placed under `FabMedia/`
- Recommended filenames are documented in `FabMedia/README.md`
