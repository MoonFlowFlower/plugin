# RuntimeInspector UI Guardrails

本文件 **不是** RuntimeInspector 的 agent 总文档。

唯一 agent 开发权威入口是 `docs/AGENT_DEVELOPMENT.md`。本文件只负责 UI 结构性改动的附录规则。

This document defines the minimum completion bar for RuntimeInspector UI work.

## Structural Vs. Style Changes

Treat a UI change as structural if it changes any of the following:

- parent panel type or child order
- `InsertChildAt`, `RemoveChild`, or reparenting
- `WidgetSwitcher` host or siblings
- `Overlay`, `VerticalBox`, `HorizontalBox`, or `ScrollBox` layout ownership
- any persistent header, footer, context strip, or global status band

Structural changes require stronger validation than style-only tweaks.

## Structural Change Review Level

Treat the following operations as structural by default and raise the review bar automatically:

- `InsertChildAt`
- `RemoveChild`
- reparenting into a different host panel
- `WidgetSwitcher`-adjacent injection or replacement
- changing `Overlay`, `VerticalBox`, `HorizontalBox`, or `ScrollBox` ownership
- adding any persistent header, footer, global band, or context strip

Structural changes are not done by compilation alone. They must add or update layout assertions.

## Non-Negotiable Layout Rules

- Current first-launch UI uses a top-level full-screen `Overlay` only as the dock shell root.
- The dock shell must keep the center viewport area transparent/pass-through; RuntimeInspector must not mount a viewport widget.
- Persistent summary or control rows may live in the dock overlay only when they belong to `UInspectorDockRootWidget` panels or the lightweight center overlay.
- Modal/toast content must stay in a separate modal/toast layer, not mixed into left/right panel document flow.
- Page content must either:
  - live in a vertical layout stack, or
  - own a page-level `ScrollBox` if it can exceed the viewport.
- Top-level content must not rely on overlap to stay visible.
- New dock widgets must route mutation through `URuntimeInspectorController`; no Widget code may directly call actor/component mutation APIs.

## Current Hard Assertions

The current self-test suite must keep enforcing these layout facts:

- `dock_layout`
  - native dock root is present
  - left and right panels exist
  - center area is transparent/pass-through and does not own a viewport widget
  - modal/toast layer is separate from dock panels
- `right_inspector_tabs`
  - `Actor`, `Changes`, `Settings`, and `Tools` switch inside right `TabContent`
  - only one tab is active/highlighted
- `transform_patch_gate`
  - transform edit creates a staged patch row
  - actor transform is not mutated before Apply
  - Apply enters `UInspectorWorldSubsystem` patch authority
- `context_strip`
  - parent host is `RI_SharedContextStripHost`
  - shared strip parent is a `VerticalBox`
  - shared strip is not mounted directly under `Overlay`
  - shared strip and `ContentSwitcher` share the same vertical host
  - shared strip appears before the `ContentSwitcher`
  - shared strip slot is `Automatic`
  - `ContentSwitcher` slot is `Fill`
- `file_page_injection`
  - injected page is the only visible child inside the `Changes` host
  - `Changes` owns a page-level scroll root
  - `Actor` page can be switched back to without the shared strip covering first-screen content
  - `ContentSwitcher` remains `Fill`
- `workflow_page_view`
  - `Tools` still owns page-level scroll
  - `Remote Session`, `Diagnostics`, `Workflows`, `Tests`, and `Report` sections still exist
- `test_page_layout`
  - `Tools` page keeps page-level scroll and advanced sections reachable
- `settings_page_layout`
  - `Settings` keeps page scroll, footer, status, and interaction surface intact

## Required Self-Test Coverage

Any structural UI change must keep these green:

- `dock_layout`
- `right_inspector_tabs`
- `transform_patch_gate`
- `context_strip`
- `file_page_injection`
- `workflow_page_view`
- `test_page_layout`
- `settings_page_layout`

Structural changes must also extend self-tests to validate:

- parent panel type
- slot type and size rule
- sibling order when order defines layout
- no visible legacy sibling leakage
- page-level scroll presence where required

## Done Definition

RuntimeInspector UI work is not complete until all of the following are true:

- code compiles
- affected UI self-tests pass
- `mainline_full_closure` has no new failures beyond known environment blockers
- manual checks were done on the affected pages
- manual checks were done in both:
  - a normal window
  - a narrow/tall window

## Manual Validation Sequence

For structural UI work, run this sequence instead of ad-hoc clicking:

1. Open `Actor`, `Changes`, `Settings`, and `Tools` once each.
2. Confirm the first screen of each page shows the page's primary purpose.
3. Confirm no shared strip, footer, or section appears to float above content.
4. Confirm lower content remains reachable with scroll in a narrow/tall window.
5. Confirm no legacy Blueprint sibling content leaks through the injected page host.

## Manual Check List

- shared top strip does not cover page content
- controls participate in layout instead of floating over it
- first screen shows the page's main action or primary content
- lower content remains reachable by scrolling
- no old blueprint sibling content is visible through injected pages
- center viewport area remains the real UE/PIE viewport behind the dock shell
- transform edit appears in `Changes` before Apply is enabled
- new dock widget source uses `RICompactUI` token helpers instead of local hardcoded colors

## Before Coding

Before implementing a structural UI change, explicitly lock these four decisions:

1. Which panel owns the widget?
2. Is it stacked with content or overlaid on content?
3. What slot rule should it use: `Automatic` or `Fill`?
4. How should it behave in a narrow/tall window: keep, collapse, or scroll?
