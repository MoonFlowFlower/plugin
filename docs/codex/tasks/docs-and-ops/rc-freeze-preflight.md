# RC Freeze Preflight

- Lane: `docs-and-ops`
- Status: `in_progress`
- Stage goal: 把当前 dirty worktree 收成一个源码自洽、无明显临时物、并且能对应现有验证证据的 RC 候选，并确认该候选在 patch hygiene 与工程编译上都可成立。

## Why This Task Exists

当前主 blocker 已经不是 transform / screenshot / Actor UI 行为本身，而是：

- dirty worktree 里仍混有明显临时物和历史噪音
- 有未跟踪文件其实已经被产品代码直接依赖
- 当前 package / media / signoff 证据还没有绑定到一个新的 clean RC commit

如果不先做这一步，后续 freeze 出来的 commit 很可能既不完整，也不等价于本地已经验证过的状态。

## Must Be In The RC Candidate

这些未跟踪文件已经被当前源码或文档 authority 直接引用，不能再继续当作“也许以后再说”的散点：

- `Source/RuntimeInspector/Public/InspectorTouchScrollBox.h`
- `Source/RuntimeInspector/Private/InspectorTouchScrollBox.cpp`
- `Config/ToolsSelfTestsDefault.json`
- `Config/ToolsWorkflowsDefault.json`
- `Content/UI/WBP_ChangesPage.uasset`
- `Content/UI/WBP_SettingsPage.uasset`
- `Content/UI/WBP_ToolsPage.uasset`
- `Scripts/RunChangesFirstOpenPerfCapture.ps1`
- `Scripts/ValidateActorPageLayout.ps1`
- `Scripts/ValidateTransformSourcePersistence.ps1`
- `docs/PROGRAM_STATE_UNIFIED.yaml`
- `docs/STATUS.md`
- `docs/codex/tasks/TASK_LANE_INDEX.md`

## Strip From The RC Candidate

这些文件不应继续作为 RC 候选内容存在：

- `.tmp_inspect_runtimeinspector_assets.ps1`
- `.tmp_inspect_runtimeinspector_assets.py`
- `FabMedia/capture_fab_media.log`
- `FabMedia/capture_manifest.txt`
- `DProjectGameUEPluginMakerSavedfab_release_cmd.log`
- `Content/UI/WBP_SettingsPage_corrupt_2026-04-12.uasset`

## Still Needs A Boundary Decision

这些内容可能需要进入 RC，但不应在 freeze 前继续处于“默认都算”的模糊状态：

- `FAB_LISTING.md`
- `FAB_ASSETS_CHECKLIST.md`
- `FAB_SUBMISSION_CHECKLIST.md`
- `docs/FAB_RC_SIGNOFF.md`
- `docs/HANDOFF_2026-04-08.md`
- `docs/HANDOFF_2026-04-11.md`
- `FabMedia/README.md`

## Current Evidence Base

- `transform_source_persistence` latest passing artifact:
  - `Saved/RuntimeInspector/Validation/TransformSourcePersistence/BC8B047E-400B-566E-1347-2DA5AA231920/prepare_report.json`
  - `Saved/RuntimeInspector/Validation/TransformSourcePersistence/BC8B047E-400B-566E-1347-2DA5AA231920/verify_restore_report.json`
- `Actor` page structure latest passing screenshot:
  - `Saved/RuntimeInspector/Validation/actor_page_structure.png`

## Remaining Pre-Freeze Blockers

1. 冻结新的 clean RC commit。
2. 从那个 commit 重新生成 package / screenshots / signoff 证据。
3. 补 packaged-install manual smoke。
4. 录制并签收 demo 视频 / GIF。

Current note:

- preserved blank host does not ship `UE_MCP_Bridge`
- packaged-install smoke therefore remains a human/manual signoff gate

## Next Smallest Action

当前最小正确动作不再是继续缩 dirty 面，而是：

1. 保持当前 staged RC candidate 不再继续扩 scope
2. 做 clean RC commit freeze 决策
3. 冻结后重跑 package / media / blank-host manual smoke

## Current Preflight Progress

- 已纳入 RC candidate 的未跟踪硬依赖：
  - `InspectorTouchScrollBox.*`
  - `Config/Tools*.json`
  - `Content/UI/WBP_ChangesPage.uasset`
  - `Content/UI/WBP_SettingsPage.uasset`
  - `Content/UI/WBP_ToolsPage.uasset`
  - `Scripts/RunChangesFirstOpenPerfCapture.ps1`
  - `Scripts/ValidateActorPageLayout.ps1`
  - `Scripts/ValidateTransformSourcePersistence.ps1`
  - `docs/PROGRAM_STATE_UNIFIED.yaml`
  - `docs/STATUS.md`
  - `docs/codex/tasks/...`
- 已从 RC candidate 中剥离的明显 repo 噪音：
  - `.tmp_inspect_runtimeinspector_assets.*`
  - `FabMedia/capture_fab_media.log`
  - `FabMedia/capture_manifest.txt`
  - `DProjectGameUEPluginMakerSavedfab_release_cmd.log`
- 当前 RC candidate 已经进入单一 staged 候选面，不再混有额外 unstaged / untracked 发布级内容。
- `git diff --cached --check` 已通过，说明 patch hygiene 当前是干净的。
- `PluginMakerEditor Win64 Development` editor build preflight 已在同一候选面上通过。
- preserved blank host 的自动加载验证已刷新通过，但 manual smoke 仍是 human/manual gate。

这意味着当前 RC candidate 已经不是“边界不成形”，而是“技术候选已成形，但仍未 freeze 成 clean RC commit”。剩余未收口项主要是 release freeze 与 human signoff，而不是产品代码完整性问题。
