# RuntimeInspector Agent 开发说明

本文件是 **RuntimeInspector 唯一 agent 开发权威源**。

如果你在修改 RuntimeInspector 的 C++、UMG、workflow、self-test、verification profile、packaged runtime 验收链或 Fab 展示链，先看这里。

以下文档 **不是** agent authority：

- `README.md`
- `USER_GUIDE_zh-CN.md`
- `FAB_LISTING.md`
- `FAB_ASSETS_CHECKLIST.md`
- `FAB_SUBMISSION_CHECKLIST.md`
- `FabMedia/README.md`

`Docs/UI_GUARDRAILS.md` 仍然保留，但它只是 UI 结构附录，受本文件约束。

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

- 全局 `Context Strip` 是唯一共享上下文摘要源
- 高级能力默认收起，不应抢占首屏
- 结构性 UI 规则见 `Docs/UI_GUARDRAILS.md`

## 3. 当前 authority 与代码锚点

当前系统 authority 固定如下：

- workflow / self-test / verification profile / page 切换 authority
  - `Source/RuntimeInspector/Private/InspectorWorldSubsystem.cpp`
  - `Source/RuntimeInspector/Public/InspectorWorldSubsystem.h`
- packaged runtime discovery / connect / patch pull / remote workflow authority
  - `Source/RuntimeInspector/Private/InspectorRemoteSession.cpp`
- UI 结构 guardrails authority
  - `Docs/UI_GUARDRAILS.md`

执行规则：

- 不要引入平行 registry、平行 workflow 表或平行 self-test authority
- 如果 workflow、verification profile、screenshot foundation、packaged validation 流程有变化，先改 `UInspectorWorldSubsystem` 这条 authority，再补 UI 或脚本
- agent 在响应需求时，必须优先理解用户的真实意图，而不是机械按表面文字逐项最小实现
- 如果用户指出的是某块 UI / 某条主路径 / 某类职责已经失去产品意义，应按完整结果收口相关遗留结构，不要只删除字面上被点名的单个控件或文本

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
  - `Docs/UI_GUARDRAILS.md` 规则
  - 正常窗口人工检查
  - 窄高窗口人工检查

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

边界：

- 运行在主工程 `PluginMaker`
- `fab_screenshot_foundation` 负责把 UI 收到展示态
- `CaptureFabMedia` 负责产出确定性的截图 staging set
- 最终截图产物默认输出到 `Saved/RuntimeInspector/FabMediaCapture/`

## 6. 文档边界

各文档当前角色固定为：

- `Docs/AGENT_DEVELOPMENT.md`
  - 唯一 agent 开发权威源
- `Docs/UI_GUARDRAILS.md`
  - UI 结构附录
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
- 如果出现新的 UI 结构约束，优先更新本文件并同步 `Docs/UI_GUARDRAILS.md`

## 7. 当前默认执行顺序

当 agent 修改 RuntimeInspector 时，默认按这条顺序工作：

1. 先确认 authority 在哪里
2. 先改 authority 层，再改 UI / 脚本 / 文档
3. 先跑窄回归，再跑主闭环
4. UI 结构改动必须同时看 self-test 和实际页面
5. 对外文档只保留用户或发布信息，不承担开发 authority

## 8. 禁止回到旧状态

以下旧状态已经过期，不应再作为设计依据：

- `Actor / File / Setting / Test` 的旧顶层命名
- 旧 TaskPack 阶段计划
- 旧的“手动先开 packaged runtime 再验收”前提
- 旧的多文档并列 authority 方式
