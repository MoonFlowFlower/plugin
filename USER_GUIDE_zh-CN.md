# Runtime Inspector 产品说明与操作指南

本文档是 **终端用户操作指南**，不是 agent 开发权威源。

如果你要修改 RuntimeInspector 的实现、workflow、self-test、验收链或 UI 结构，请看 `docs/AGENT_DEVELOPMENT.md`。

本文档面向首次接触 `Runtime Inspector` 的项目成员、测试人员和内容制作人员，目标是帮助你在 Unreal Engine 5.7 中完成安装、启用、基础操作和常见问题排查。

## 1. 产品定位

`Runtime Inspector` 是一个面向 Unreal Engine 5.7 的运行时检查与变更审阅插件。

它的核心用途不是“通用远程调试”，而是把运行时改动变成可观察、可记录、可对比、可审阅的工作流。

主要用途：

- 在 PIE 中快速查看当前 Actor 的运行时状态
- 对支持的属性做临时修改并立即观察结果
- 把运行时改动捕获为 snapshot、staged patch、preset 或导出 patch
- 对比“当前运行时”和“源数据默认值”的差异
- 在 Editor 权限下执行受控的 preview / apply / promote
- 通过内建 self-test、verification profile 和 workflow 验证整条链路是否正常
- 从 loopback packaged runtime 拉回 patch，再继续走 editor-side 审阅闭环

## 2. 适用范围

当前首发交付范围：

- Unreal Engine `5.7`
- 平台以 `Win64` 为主
- 主要工作流是 `Editor + PIE`
- 高级工作流支持 `loopback packaged runtime session`

当前不在支持范围内：

- Shipping 构建中的运行时检查
- 局域网多机自动发现
- 跨机器 remote discovery
- 在 packaged runtime 中直接做 source promote

## 3. 安装与启用

1. 将 `RuntimeInspector` 插件目录复制到项目的 `Plugins/` 目录。
2. 打开 Unreal Engine 5.7 项目。
3. 在 Plugins 浏览器中确认 `Runtime Inspector` 已启用。
4. 按提示重启编辑器。
5. 确认 `Project Settings -> Plugins -> Runtime Inspector` 中能看到设置项。

## 4. 快速开始

推荐第一次使用按下面顺序走一遍：

1. 启动 PIE。
2. 按 `O` 打开 Runtime Inspector。
3. 将鼠标悬停到目标 Actor 上后按 `P` 进行拾取，或者使用你在 `Settings` 中配置的鼠标位置拾取方式。当前拾取使用 object query，所以鼠标下的玩家角色和场景 Actor 都可以被选中。
4. 在 `Actor` 页查看当前对象和支持的属性。
5. 修改一个安全的运行时属性并确认结果。
6. 切到 `Changes` 页执行 `Stage Runtime Changes`。
7. 再执行一次审阅动作，例如 compare 或 audit。
8. 到 `Tools` 页运行一个 self-test 或 workflow，确认链路正常。

## 5. 界面总览

插件主面板分为四个页签：

- `Actor`
- `Changes`
- `Settings`
- `Tools`

页面职责：

- `Actor`
  - 围绕当前目标对象做检查和属性编辑
- `Changes`
  - 管理 staged patch、preset、compare、audit、preview 和 apply
- `Settings`
  - 调整插件行为、热键和外观
- `Tools`
  - 运行 self-test、workflow、remote session 和诊断能力

### 5.1 分辨率、DPI 与 UI Scale

- 默认 `UIScale=1.0` 时，插件会在自身范围内抵消宿主 viewport DPI，使文字、按钮、间距和左右面板在所验收分辨率上维持相同的屏幕像素基线。
- `UIScale` 是用户主动控制 Runtime Inspector 整体大小的设置，范围为 `0.8-1.5`。它只缩放插件，不修改项目的全局 DPI 曲线，也不缩放中央游戏视口。
- 左右面板使用 Anchor、布局容器、ScaleBox 和 ScrollBox 适配；中央区域始终 Fill。窄窗口会优先收起左侧说明、启用滚动或省略过长文本，不会通过降低默认字号来塞入内容。
- UI 由运行时 native UMG 结构生成，不是完整的 Designer UMG 蓝图；这不影响响应式布局，但用户不能在 UMG Designer 中任意拖拽重排整个插件界面。
- 首发验收覆盖 `1280x720`、`1600x900`、`1920x1080`、`2560x1440`、`3840x2160` 和 `900x1200`，以及 `UIScale=0.8/1.0/1.25/1.5`。这不等于所有硬件或第三方 DPI 曲线都已证明。

## 6. 常见工作流

### 6.1 本地 PIE 中做一次运行时修改并审阅

1. 启动 PIE。
2. 打开面板并拾取目标 Actor。
3. 在 `Actor` 页修改一个支持的属性。
4. 切到 `Changes` 页执行 `Stage Runtime Changes`。
5. 执行 compare、audit 或 preview。
6. 查看 report / preview。
7. 如果确认变化合理，再考虑 editor-side apply / promote。

### 6.2 保存和重用一组运行时配置

1. 在 `Actor` 页完成运行时调整。
2. 到 `Changes` 页执行 `Stage Runtime Changes`。
3. 使用 `Save Preset`。
4. 后续对同类型对象使用 `Apply Preset`。

### 6.3 从 Packaged Runtime 拉回 Patch

1. 启动本地 loopback packaged runtime。
2. 在 Editor 中打开 Runtime Inspector。
3. 到 `Changes` 或 `Tools` 页选择 remote session。
4. 查询目标对象。
5. 在 remote runtime 侧完成必要变更。
6. 执行 patch pull。
7. 回到 Editor 侧继续 stage、audit、review 和 apply。

注意：

- packaged runtime 只负责提供运行时真相
- source authority 仍然在 Editor

## 7. 常见问题

### Q0. 为什么换分辨率后 Runtime Inspector 看起来仍保持同样大小？

这是默认行为。`UIScale=1.0` 使用固定屏幕像素语义；如果希望插件整体更大或更小，请在 Runtime Inspector 设置中显式调整 `UIScale`，而不是修改项目全局 DPI。

### Q1. 为什么我在 packaged runtime 里不能直接改 source asset？

因为当前支持的是 **editor authority** 工作流。packaged runtime 负责提供运行时真相，source 侧 preview / apply / promote 仍由 Editor 执行。

### Q2. 为什么我找不到 remote session？

优先确认：

- 你当前使用的是 loopback 环境
- packaged runtime 已启动
- 你是在主工程 `PluginMaker` 中做 packaged runtime 验收，而不是 blank validation host

### Q3. 提审前应该看哪些文档？

- 用户使用：`README.md`
- 中文操作：`USER_GUIDE_zh-CN.md`
- Fab 提审：`FAB_SUBMISSION_CHECKLIST.md`
- Fab 素材：`FabMedia/README.md`

## 8. 验收与提审边界

这里只保留对终端用户有帮助的边界说明：

- 主工程 `PluginMaker`
  - 用于运行时验收、packaged runtime 工作流、截图展示态
- blank validation host
  - 只用于插件包安装/加载 smoke
- Fab 截图素材
  - 先用 `Scripts\OpenFabScreenshotState.cmd` 打开展示态
  - 再用 `Scripts\CaptureFabMedia.cmd` 生成确定性的截图素材
  - 输出默认落在 `Saved\RuntimeInspector\FabMediaCapture\`

更详细的内部开发/验收规则见 `docs/AGENT_DEVELOPMENT.md`。

## 9. 支持与反馈

- 文档入口：`README.md`
- Issue / 支持：<https://github.com/MoonFlowFlower/plugin/issues>

如果你要把本插件交给团队里的其他同事使用，建议把本文档和 README 一起发给他们。
