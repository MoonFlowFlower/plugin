# RuntimeInspector Task Lane Index

本文件是 RuntimeInspector 当前任务分道和任务入口的 index authority。
默认先看这里，再决定是否需要新建具体 task file。

## Lane Status Tags
- `proposed`
- `in_progress`
- `blocked`
- `pending_validation`
- `done`

## Task File Contract
- 任务文件路径：`docs/codex/tasks/<lane>/<task-id>.md`
- `task-id` 建议使用短横线英文短名，不加日期前缀
- 一个 task file 只承载一个可验证目标，不要把不同 blocker 混进同一文件
- 如果某个 lane 还没有 task file，就先在本 index 里维护当前最高优先任务

## Lanes

### `runtime-ui`
- Goal: 保持 Actor/Changes/Settings/Tools 的运行时 UI 可用、可读、可验证。
- Current status: `in_progress`
- Current top task: 收尾 recent narrow/split UI 改动，只在真实主流程和截图证据下继续调整。
- Authority docs:
  - `docs/AGENT_DEVELOPMENT.md`
  - `docs/UI_GUARDRAILS.md`
  - `docs/TROUBLESHOOTING.md`
- Task files: none yet

### `file-workflow`
- Goal: 保持 `Stage -> Preview -> Apply To Source` 主链稳定，尤其是 runtime 变更到 source promote 的真实闭环。
- Current status: `in_progress`
- Current top task: 验证并必要时继续修复 SceneComponent transform 的 source persistence loop。
- Authority docs:
  - `docs/PROGRAM_STATE_UNIFIED.yaml`
  - `docs/AGENT_DEVELOPMENT.md`
  - `docs/TROUBLESHOOTING.md`
- Task files: none yet

### `settings-persistence`
- Goal: 保持 settings authority、保存和 PIE 重新进入后的读取一致。
- Current status: `pending_validation`
- Current top task: 重新验证 settings persistence authority，尤其是 outline/theme 相关值是否在新 PIE 世界一致回读。
- Authority docs:
  - `docs/AGENT_DEVELOPMENT.md`
  - `docs/TROUBLESHOOTING.md`
  - `docs/HANDOFF_2026-04-11.md`
- Task files: none yet

### `validation-and-bridge`
- Goal: 保持 UE MCP / bridge / self-test / screenshot foundation 与真实用户路径一致。
- Current status: `in_progress`
- Current top task: 优先使用真实 `O` 路径、bridge state authority 和真实窗口截图，避免“自测通过但用户看到不一样”。
- Authority docs:
  - `docs/AGENT_DEVELOPMENT.md`
  - `docs/TROUBLESHOOTING.md`
- Task files: none yet

### `docs-and-ops`
- Goal: 维持文档 authority、状态入口和新会话接管成本处于可控状态。
- Current status: `in_progress`
- Current top task: 先把 dirty worktree 收成可重建、可追溯、无临时物的 RC 候选，再进入 clean RC freeze。
- Authority docs:
  - `docs/PROGRAM_STATE_UNIFIED.yaml`
  - `docs/STATUS.md`
  - `docs/AGENT_DEVELOPMENT.md`
- Task files:
  - `docs/codex/tasks/docs-and-ops/rc-freeze-preflight.md`

## Defaults For Future Tasks
- 如果任务会反复跨会话持续推进，先放到某个 lane，再决定是否拆出 task file。
- 如果问题只是一次性局部修复且不会形成持续主线，可不建 task file。
- 如果某条 lane 连续两轮都处于 `pending_validation` 而没有新证据，应优先补验证入口，而不是继续打补丁。
