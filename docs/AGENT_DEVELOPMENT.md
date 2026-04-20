# RuntimeInspector Agent 开发说明

本文件是 **RuntimeInspector 唯一 agent 开发权威源**。

如果你在修改 RuntimeInspector 的 C++、UMG、workflow、self-test、verification profile、packaged runtime 验收链或 Fab 展示链，先看这里。

当前新会话默认入口固定为：

1. `docs/PROGRAM_STATE_UNIFIED.yaml`
2. `docs/STATUS.md`
3. `docs/codex/tasks/TASK_LANE_INDEX.md`
4. 然后再按需读本文件和 `docs/TROUBLESHOOTING.md`

以下文档 **不是** agent authority：

- `README.md`
- `USER_GUIDE_zh-CN.md`
- `FAB_LISTING.md`
- `FAB_ASSETS_CHECKLIST.md`
- `FAB_SUBMISSION_CHECKLIST.md`
- `FabMedia/README.md`

`docs/UI_GUARDRAILS.md` 仍然保留，但它只是 UI 结构附录，受本文件约束。

补充文档角色：

- `docs/PROGRAM_STATE_UNIFIED.yaml`
  - 当前项目状态的结构化 authority
  - 用于新会话快速建立当前 focus、风险、验证口径和任务系统入口
- `docs/STATUS.md`
  - 当前状态的人类可读总览
  - 用于快速判断“现在先做什么，不该做什么”
- `docs/codex/tasks/TASK_LANE_INDEX.md`
  - 当前 task lane/index authority
  - 用于把 work 拆到稳定分道，而不是继续靠 handoff 串联
- `docs/TROUBLESHOOTING.md`
  - 记录已真实遇到的问题、证据链和修复方式
  - 不是 authority，但后续排查同类问题时应先查它
- `docs/HANDOFF_*.md`
  - 只保留为历史会话记录
  - 不再作为新会话默认入口

## 1. 当前产品定位

RuntimeInspector 当前是一个 **运行时检查、变更审阅、验证与受控 promote 工作台**，不是通用远程调试器。

当前主价值：

- 在 `Editor + PIE` 中检查 Actor 与支持的属性
- 把运行时变化收成 snapshot、staged patch、preset、export
- 对比 runtime 与 source baseline
- 在 editor authority 下做受控 preview / apply / promote
- 用内建 self-test、verification profile、workflow 维持闭环
- 从 loopback packaged runtime 把 patch 拉回 editor 再继续审阅

当前明确非目标：

- 任意网络环境下的通用 remote debugger
- 多机自动发现
- Shipping 构建中的运行时检查
- 在 packaged runtime 中直接 source promote

## 2. 当前界面与主路径

当前顶层页签固定为：

- `Actor`
- `Changes`
- `Settings`
- `Tools`

当前页面职责：

- `Actor`
  - 只做对象检查与属性编辑
- `Changes`
  - 只做日常变更处理与 source 审阅主路径
  - 主路径是 `Inspect -> Edit -> Stage -> Preview -> Apply`
- `Settings`
  - 只做插件行为与外观配置
- `Tools`
  - 承接高级工程能力
  - 包括 workflow、self-test、remote session、diagnostics

共享 UI 事实：

- `Changes / Settings / Tools` 继续使用全局 `SharedContextStrip`
- `Actor` 页上下文摘要 authority 只能是 `ActorTopContextStrip`
- `SharedContextStrip` 不得作为 `Actor` 页首帧 fallback，也不负责纠正 `Actor` 页布局
- 高级能力默认收起，不应抢占首屏
- 结构性 UI 规则见 `docs/UI_GUARDRAILS.md`

## 3. 当前 authority 与代码锚点

当前系统 authority 固定如下：

- workflow / self-test / verification profile / page 切换 authority
  - `Source/RuntimeInspector/Private/InspectorWorldSubsystem.cpp`
  - `Source/RuntimeInspector/Public/InspectorWorldSubsystem.h`
- packaged runtime discovery / connect / patch pull / remote workflow authority
  - `Source/RuntimeInspector/Private/InspectorRemoteSession.cpp`
- UI 结构 guardrails authority
  - `docs/UI_GUARDRAILS.md`

`Actor` 页首开上下文条 authority 固定如下：

- 首帧准备只能收口到 `UInspectorWorldSubsystem::PrimeActorPageForInitialOpen()`
- `UInspectorWorldSubsystem::PrepareActorPageForPresentation()` 是 `Actor` 页进入可见态前的唯一包装 helper
- `UInspectorWorldSubsystem::ApplyActorSplitPresentation()` 是 `Actor` 页左右分体呈现的唯一 helper
- `UInspectorWorldSubsystem::RestoreActorSinglePanelBodyLayout()` 是 `Actor` 页退出 split 或窄屏 fallback 的唯一 helper
- `Open()` 与 `HandleActorTabClicked()` 必须共用这条 helper
- 首次 `AddToViewport()` 之前就必须先完成 `Actor` 页动态宿主和顶部条注入，不能等 panel 已经显示后再补
- `ScheduleDeferredOpenActorRefresh()` 只负责默认选中 actor、baseline 与结构刷新
- 不能再让 deferred refresh 承担“把底部条修回顶部”的职责
- `GetActorPropertyHostDebugSummaryForAutomation()` 是 `Actor` 页结构诊断入口
- `HideSharedContextStripForActorPage()` 是 `Actor` 页首开和切页时收起底部共享条的唯一 helper
- `Actor` 页 split 只允许把 `ActorWorkbenchSidebarHost` 挂到左侧副 panel；`ActorTopContextStrip` 必须作为 `ActorWorkbenchSidebarHost` 的 `child[0]` 跟随左栏移动，不能继续留在右侧主 panel
- `Actor` 页 split 默认几何是“左栏贴左边、右栏贴右边”，中间留白来自剩余视野，不再维护固定 center gap
- `Actor` 页 selection 顶部摘要唯一 authority 是左栏顶部 compact `ActorTopContextStrip`；右侧不再使用四栏 summary，也不能把 `SharedContextStrip` 当成 `Actor` 页 fallback
- `ActorTopContextStrip` 需要压缩时优先改它自己的 compact builder，不要误伤 `SharedContextStrip`
- `Changes / Settings / Tools` 不允许复用 `Actor` 页 split shell；切出 `Actor` 页后必须回退到单面板
- 非 `Actor` 单面板宽度 authority 现在和 `Actor` 右栏绑定，默认走同一套窄栏宽度规则，不再单独维护宽屏默认宽度
- `Changes / Settings / Tools` 的布局实现必须先满足 `320..420px` 窄栏可用，再考虑宽屏；新增 row/action UI 时优先复用 compact helper，而不是继续写固定 label 宽和宽屏横排

执行规则：

- 不要引入平行 registry、平行 workflow 表或平行 self-test authority
- 如果 workflow、verification profile、screenshot foundation、packaged validation 流程有变化，先改 `UInspectorWorldSubsystem` 这条 authority，再补 UI 或脚本
- agent 在响应需求时，必须优先理解用户的真实意图，而不是机械按表面文字逐项最小实现
- 如果用户指出的是某块 UI / 某条主路径 / 某类职责已经失去产品意义，应按完整结果收口相关遗留结构，不要只删除字面上被点名的单个控件或文本
- RuntimeInspector 的视觉验收不能靠猜窗口；先读 `panelHostWindowDebug`，再抓实际宿主窗口
- 运行时浮动面板如果走 `CanvasPanelSlot`，必须先确认 anchors 语义，不能把 stretch anchors 当成固定浮窗来写 offsets

## 4. 当前验证体系

当前最低质量门不是“编译过就算完”，而是闭环验证。

当前验证层级：

- 单项 `self-test`
- `verification profile`
- `workflow`
- `mainline_full_closure`

当前关键自测与布局护栏至少包括：

- `context_strip`
- `file_page_injection`
- `workflow_page_view`
- `test_page_layout`
- `settings_page_layout`
- `theme_preset_preview`
- `file_workflow`

当前主闭环要求：

- `mainline_full_closure` 应视为总回归 authority
- 如果 packaged runtime 环境不可用，不允许再用“手动先开 packaged runtime”做默认前提
- packaged runtime 验收现在应能自动确保本机 loopback validation session

当前 done definition：

- Unreal build 成功
- 受影响 self-tests 通过
- 受影响 verification profiles / workflows 通过
- `mainline_full_closure` 无新增失败
- UI 结构改动还必须满足：
  - `docs/UI_GUARDRAILS.md` 规则
  - 正常窗口人工检查
  - 窄高窗口人工检查

UI 验证默认路径：

- 优先使用 UE MCP
- UE bridge / runtime bridge 端口不能再硬编码假设为 `9877 / 9897-9901`
- 当前 authority 是读取 `Saved/UE_MCP_Bridge/bridge_state.json` 和 `bridge_state_runtime.json`
- 当前默认动态端口范围：
  - editor bridge: `12077-12086`
  - runtime bridge: `12097-12101`
- 页面打开入口固定优先：
  - 用户真实入口等价路径优先：`control_runtime_inspector { action = "toggle_input" }`
  - 只有在明确验证非首开切页时，才使用 `ri open actor` / `ri showpage actor`
- 拾取验证入口固定为 bridge 的 `control_runtime_inspector`：
  - `action = "pick_input"` 走真实 `PickKey` 输入链
  - `action = "right_mouse_pick_input"` 走与 `Ctrl+RMB` 同 authority 的右键拾取链
  - `action = "position_mouse_on_player_character"` 只用于验收前把鼠标准确投到本地玩家角色上
  - 验收时读 `lastPickDebug`，确认来源是 `PickKey` / `RightMouse`
- 截图固定优先使用 MCP 截图，不再默认依赖 OS 层切屏、热键模拟或窗口猜测
- `capture_window_screenshot` 当前默认语义就是宿主窗口截图；`capture_screenshot` 继续只代表 viewport / `FScreenshotRequest`
- RuntimeInspector 视觉验收必须先读取 `panelHostWindowDebug`，再把解析出的宿主标题传给 `capture_window_screenshot`
- 首开 UI 验证允许为了宿主窗口解析先读一次 automation summary；除这一步外，不要在首帧截图前额外插入无关调试读取
- 当前仓库内的 bridge 验证脚本入口是 `Scripts/ValidateActorPageLayout.ps1`
- `Scripts/RunChangesFirstOpenPerfCapture.ps1` 是 `changes_first_open_perf_capture` 的外部 runner authority：
  - 通过 editor bridge 进入 PIE
  - 运行 `capture_changes_first_open_perf`
  - 读取 `Saved/RuntimeInspector/Validation/<capture_id>/report.json`
  - 追加 bridge screenshot，并输出同目录 `external_runner.json`
- 如果 UE MCP 当前离线，结果口径必须收成 `pending-validation`，不能把 PowerShell / SendKeys fallback 包装成同等级证据
- 新增 helper、console command、debug summary 或验证入口时，必须同步记入本文件

## 5. 当前脚本入口矩阵

### 发布与安装 smoke

- `Scripts/PackageFabRelease.cmd`
- `Scripts/PackageFabRelease.ps1`

作用：

- 生成 Fab 发布包

### blank host 安装/加载 smoke

- `Scripts/ValidateFabBlankProject.cmd`
- `Scripts/ValidateFabBlankProject.ps1`
- `Scripts/OpenFabValidationProject.cmd`
- `Scripts/OpenFabValidationProject.ps1`

边界：

- 只验证插件包能安装、能加载、模块不缺失
- 不负责 runtime self-test
- 不负责 packaged runtime discovery
- 不负责 `mainline_full_closure`

### 主工程 packaged runtime 验收

- `Scripts/BuildPackagedRuntimeValidation.cmd`
- `Scripts/BuildPackagedRuntimeValidation.ps1`
- `Scripts/RunPackagedRuntimeValidation.cmd`
- `Scripts/RunPackagedRuntimeValidation.ps1`
- `Scripts/StopPackagedRuntimeValidation.cmd`
- `Scripts/StopPackagedRuntimeValidation.ps1`

边界：

- 运行在主工程 `PluginMaker`
- 负责 loopback packaged runtime validation
- 负责 packaged self-tests、packaged workflows、mainline packaged closure

### Fab 展示态与截图素材

- `Scripts/OpenFabScreenshotState.cmd`
- `Scripts/OpenFabScreenshotState.ps1`
- `Scripts/CaptureFabMedia.cmd`
- `Scripts/CaptureFabMedia.ps1`
- `Scripts/ValidateActorPageLayout.ps1`

边界：

- 运行在主工程 `PluginMaker`
- `fab_screenshot_foundation` 负责把 UI 收到展示态
- `CaptureFabMedia` 负责产出确定性的截图 staging set
- `ValidateActorPageLayout` 负责通过本地 UE bridge 执行 `Actor` 页首开自测并抓宿主窗口截图
- 最终截图产物默认输出到 `Saved/RuntimeInspector/FabMediaCapture/`

## 6. 文档边界

各文档当前角色固定为：

- `docs/AGENT_DEVELOPMENT.md`
  - 唯一 agent 开发权威源
- `docs/UI_GUARDRAILS.md`
  - UI 结构附录
- `docs/TROUBLESHOOTING.md`
  - 已知问题与排查记录
- `README.md`
  - 用户与维护者概览
- `USER_GUIDE_zh-CN.md`
  - 终端用户中文操作指南
- `FAB_LISTING.md`
  - Fab 上架文案
- `FAB_ASSETS_CHECKLIST.md`
  - Fab 素材拍摄清单
- `FAB_SUBMISSION_CHECKLIST.md`
  - Fab 提审清单
- `FabMedia/README.md`
  - Fab 媒体素材命名与捕获说明

规则：

- 不要再把 agent 开发规则塞回 `README`、用户指南或 Fab 文档
- 如果出现新的开发流程权威说明，只能加到本文件
- 如果出现新的 UI 结构约束，优先更新本文件并同步 `docs/UI_GUARDRAILS.md`
- 如果遇到新的运行时 UI / bridge / workflow 异常，整理成证据链和修复方法时应优先补到 `docs/TROUBLESHOOTING.md`

## 7. 当前默认执行顺序

当 agent 修改 RuntimeInspector 时，默认按这条顺序工作：

1. 先确认 authority 在哪里
2. 先改 authority 层，再改 UI / 脚本 / 文档
3. 先跑窄回归，再跑主闭环
4. UI 结构改动必须同时看 self-test 和实际页面
5. 对外文档只保留用户或发布信息，不承担开发 authority
6. 任何 UI 视觉结论都不能只基于 `Preview` 独立窗口或 geometry 数字
7. 如果需求涉及比例、层级、审美或“看起来对不对”，必须先确认 `panelHostWindowDebug`，再抓实际宿主窗口验收
8. 如有必要，再补主编辑器窗口截图做对照，避免把“宿主不同”误判成“布局不同”

## 8. 禁止回到旧状态

以下旧状态已经过期，不应再作为设计依据：

- `Actor / File / Setting / Test` 的旧顶层命名
- 旧 TaskPack 阶段计划
- 旧的“手动先开 packaged runtime 再验收”前提
- 旧的多文档并列 authority 方式
