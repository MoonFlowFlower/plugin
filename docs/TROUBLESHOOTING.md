# RuntimeInspector Troubleshooting

本文件记录 **已经真实遇到过的问题、证据链和修复方式**。

它不是产品 authority，也不是 UI 规范；authority 仍然以 `docs/AGENT_DEVELOPMENT.md` 为准。
它的用途是：后续再遇到类似问题时，先查这里，避免重复走弯路。

如果你需要的是“当前项目正在做什么、下一步做什么”，先看：

- `docs/PROGRAM_STATE_UNIFIED.yaml`
- `docs/STATUS.md`
- `docs/codex/tasks/TASK_LANE_INDEX.md`

## 1. Inspect 页数据存在但界面空白

### 现象

- `Inspect` 页打开后，看起来几乎是空白的
- 顶部 tab、标题、底部按钮还在
- `Component / Star / Property / Functions` 不显示
- 但 bridge / automation 明明返回：
  - `propertyItemCount > 0`
  - `functionItemCount > 0`
  - `groupsEntryWidgetCount > 0`
  - `propertyRowWidgetCount > 0`

### 这类问题的真实含义

这通常不是“没数据”，而是：

- 行对象已经创建了
- section widget 也已经创建了
- 但宿主 UMG 容器没有拿到有效几何
- 最终表现成 `0x0` 尺寸的不可见 host

不要优先怀疑：

- selection 没选中
- property/function 数据没生成
- row widget 没实例化

这些都要先看证据，而不是猜。

### 本次根因

`Inspect` 页自定义工作台被注入到了旧 panel 容器链里，但旧链的宿主布局没有同步更新，导致：

- `RI_InspectWorkbenchBody`
- `RI_InspectWorkbenchContent`
- `RI_ActorPropertyFunctionHost`

虽然都已经挂上了，但运行时几何仍然是 `0x0`。

同时，旧 `Modified` 区和旧 `Body/Right` 结构仍然残留在同一层，进一步干扰了布局判断。

### 关键证据

当遇到“数据有但页面空”时，优先确认这几类证据：

1. 数据量

- `propertyItemCount`
- `functionItemCount`
- `rootGroupCount`
- `groupsEntryWidgetCount`
- `propertyRowWidgetCount`

如果这些都是正数，说明主问题已经不是数据源。

2. host 几何

- `propertyHostDebug`
- `propertyAnchorChain`
- `inspectBodyChildren`

本次典型坏态是：

- `Host=RI_ActorPropertyFunctionHost[parent=RI_InspectWorkbenchContent,size=0.0x0.0,vis=0]`
- `VerticalBox_123` 有正常尺寸
- 但 `RI_InspectWorkbenchBody` 和它下面的 content host 仍然是 `0x0`

这说明：

- panel 根链是活的
- 自定义工作台已经重挂
- 但重挂后的容器没有真正参与到本帧布局分配

3. live 截图

不要只看 automation 数字。
必须同时抓运行中窗口截图，确认用户实际看到的页面是否和数据一致。

### 推荐排查顺序

按这个顺序排，不要来回跳：

1. 确认是否真有数据
- 用 bridge 看 `propertyItemCount / functionItemCount / groupsEntryWidgetCount`

2. 确认是不是 row 没创建
- 看 `propertyRowWidgetCount / functionRowWidgetCount`

3. 确认是不是 host 为 `0x0`
- 看 `propertyHostDebug`

4. 确认挂到了哪条父链
- 看 `propertyAnchorChain`
- 看 `inspectBodyChildren`

5. 再抓 live 截图
- 如果截图空白，但 row/widget count 正常，优先判定为几何/宿主问题

### 本次有效修复动作

本次证明有效的动作是这类，不是“盲目刷新数据”：

- 在 `EnsureActorWorkbenchBodyInjected()` 中明确处理旧宿主
- 折叠旧 `Body`
- 折叠旧 `Modified`
- 对新旧宿主的 parent 链执行：
  - `InvalidateLayoutAndVolatility()`
  - `ForceLayoutPrepass()`

也就是说，真正有效的是：

- 改宿主结构
- 触发布局重算

而不是：

- 再跑一次 `RefreshActorPropertiesSection()`
- 再补一次 selection refresh
- 再强行创建一遍 row widget

这些动作在本问题里都不能解决 `0x0` 几何。

## 2. Agent 看到的窗口和用户看到的不一致

### 现象

- agent 说“页面已经有内容/比例还行”
- 但用户实际看到的是另一块窗口，甚至是完全不同的布局结果
- 常见表现：
  - agent 看的是 `Preview`
  - 用户看的是主编辑器
  - 或者反过来

### 本次真实根因

这类错位不只是一种原因，本次实际遇到了两层：

1. 截图工具选错窗口

- `capture_window_screenshot` 过去在同标题多窗口时会取第一个匹配项
- 对 `PluginMaker - Unreal Editor` 这类标题，会误抓到一个很小的壳窗口，而不是真正主编辑器窗口
- 更早的一层坑是：bridge 里这个方法过去默认并不等于“抓窗口”；只有显式传 `scope=window` 才会走窗口抓图，否则会退回 viewport screenshot

2. 运行时 UI 实际宿主窗口被猜错

- RuntimeInspector 不是总在主编辑器 viewport 里
- 当前这套 `pie_control start` 流程下，它实际挂在 `PluginMaker Preview ...` 窗口里
- 如果仍然强行抓主编辑器，就会得到“用户说界面很丑，但截图里根本没有这个 UI”的错位证据

### 现在的正确做法

不要再猜“应该抓主编辑器还是 Preview”。

先读运行时字段：

- `panelHostWindowDebug`

它会返回：

- 实际宿主窗口标题
- 窗口尺寸
- 窗口位置

然后再用这个真实标题去做窗口截图。

当前修正后的口径是：

- `capture_window_screenshot`
  - 默认就是宿主窗口截图
  - 应显式传 `windowTitleContains = panelHostWindowDebug` 解析出的宿主标题
- `capture_screenshot`
  - 继续只表示 viewport / `FScreenshotRequest`

### 验收规则

凡是涉及：

- 比例
- 布局
- 层级
- 丑不丑
- 是否和用户看到的一样

都必须满足：

1. 先确认 `panelHostWindowDebug`
2. 再抓对应宿主窗口
3. 必要时同时抓主编辑器窗口作为对照

不能再只凭：

- `Preview` 窗口
- 主编辑器窗口
- geometry 数字
- “非空”判断

任意一个面就下结论。

## 3. Inspect 面板变成一条横带/高度异常

### 现象

- 面板顶部标题和 tab 还在
- 但主体内容区几乎消失
- live 图里像一条窄横带贴在窗口上方

### 本次根因

根 `Border` 的 `CanvasPanelSlot` 处于拉伸锚点模式：

- `Anchors=(0,0,1,1)`

同时又被当成固定大小浮窗去写：

- `Offsets.Left/Top = 位置`
- `Offsets.Right/Bottom = 宽高`

在这种拉伸锚点模式下，`Right/Bottom` 不是单纯的宽高，最终会把可见尺寸算成负值或极小值。

典型证据：

- `Root=Border`
- `CanvasSlotSize=1240x720`
- 但 `Root` 实际 cached size 是负宽或极小高

### 有效修复

把根 `CanvasPanelSlot` 改成固定锚点的浮动面板模型：

- `Anchors=(0,0,0,0)`
- `Alignment=(0,0)`
- `Position=(固定左上坐标)`
- `Size=(宽,高)`

也就是说：

- 位置和尺寸必须在同一种 slot 语义下设置
- 不能在 stretch anchors 上假装它是固定浮窗

### 本次无效或次要的尝试

下面这些动作在这类问题里容易浪费时间：

- 只修 selection/open flow
- 只检查 Actor 是否选中
- 只给 row widget 增加 `TakeWidget()`
- 只做数据层 fallback
- 只在 tab 点击时追加一轮 refresh

它们可以作为辅助，但不是主修复路径。

### 后续再遇到类似问题的准则

- 先证据，后猜测
- 先确认“有没有数据”，再确认“有没有尺寸”
- 如果 host 是 `0x0`，就直接转去查宿主链和布局 invalidation
- 不要把“空白页”默认当成 selection 或 data-source 问题
- 任何 UI 注入改动都要同时做：
  - automation 读数验证
  - live 窗口截图验证

## 4. Actor 页第一次打开时上下文条还在底部，第二次才到顶部

### 现象

- 在 PIE 里第一次按 `O` 打开 RuntimeInspector
- `Actor` 页底部还能看到旧的上下文条
- 顶部 `ActorTopContextStrip` 没有按预期出现在左栏搜索框上方
- 关闭再开一次，或者切去别的 tab 再回来，位置又会变正确

### 这类问题的真实含义

这类症状不要先当成：

- `ActorTopContextStrip` 没创建
- `SelectedActor` 没值
- `SharedContextStrip` 没隐藏

如果第二次打开会变正确，更高概率是：

- 第一次把 panel 加进 viewport 之后，才去做动态宿主重排
- 用户首帧已经看到了旧布局
- 第二次打开时复用了已经重排好的 widget tree，所以看起来“自动好了”

### 本次根因

`Open()` 过去是在 `PanelWidget->AddToViewport()` 之后，才调用 `PrimeActorPageForInitialOpen()`。
这会让 `Actor` 页首次可见帧先暴露旧的 panel shell 布局，再由后续动态注入去纠正。

也就是说，问题不是“第二次更会刷新”，而是：

- 第一次展示得太早
- 动态 `Actor` 页准备做得太晚

### 有效修复

本次有效修复动作是：

- 新增 `UInspectorWorldSubsystem::PrepareActorPageForPresentation()`
- `Open()` 在 `AddToViewport()` 之前先走一次这个 helper，先把 `Actor` 页宿主、顶部条和 legacy 折叠准备好
- `AddToViewport()` 之后再走一次带 layout forcing 的同 helper，确保首开可见帧和后续 Slate 布局一致
- `HandleActorTabClicked()` 也统一走这条 helper，避免首开和二开走不同 authority

### 正确验证方式

首开问题必须按用户真实入口验证，不要拿近路替代：

- 用 bridge / MCP 的 `control_runtime_inspector { action = "toggle_input" }`
- 先抓 viewport live 截图
- 再读 automation summary
- 最后再跑 self-test

拾取问题不要再靠“看起来像走到了”判断：

- `P` 路径用 `control_runtime_inspector { action = "pick_input" }`
- `Ctrl+RMB` 路径用 `control_runtime_inspector { action = "right_mouse_pick_input" }`
- 需要验证玩家角色时，先用 `control_runtime_inspector { action = "position_mouse_on_player_character" }` 把鼠标准确投到本地玩家角色上
- 验收读 automation summary 里的 `lastPickDebug`
- 只有当 `Source=PickKey` / `Source=RightMouse` 且 actor path 符合预期时，才算真的走到对应输入链
- 如果玩家角色仍然选不中，先检查拾取是不是又退回了 `ECC_Visibility`；UE 默认 `Pawn` / `CharacterMesh` 会忽略 `Visibility`

不要反过来先读 automation summary。
因为某些 summary helper 会强制 layout/prepass，可能把“首开错位”在调试读取时顺手矫正掉。

## 4. Actor 页上下文条第一次在底部，第二次才回到顶部

### 现象

- 第一次打开 `Actor` 页时，上下文条出现在底部
- 切到别的页再切回来，或者第二次打开后，才跑到顶部正确位置
- 这通常伴随“首开和二开不是同一条刷新链”

### 这类问题的真实含义

这不是单纯的 slot / padding / index 写错。
如果第二次能对，说明最终宿主链通常已经存在，真正有问题的是：

- 首开时序
- shared strip 和 actor strip authority 混用
- deferred refresh 被拿来补首帧结构

### 本次根因

`Open()` 首帧先走了 `SharedContextStrip` 更新，再进入 `Actor` 页结构准备；
`ActorTopContextStrip` 只是在后续 `RefreshPanel()` 或 deferred open refresh 中被动纠正。
结果就是：

- 第一次打开：先看到错误的底部链
- 第二次打开：因为 `Actor` 页宿主已经准备好，看起来才正确

### 有效修复

这类问题必须收成单一 authority：

- `Actor` 页只看 `ActorTopContextStrip`
- `SharedContextStrip` 只服务非 `Actor` 页
- `Open()` 和 `HandleActorTabClicked()` 共用 `PrimeActorPageForInitialOpen()`
- `PrimeActorPageForInitialOpen()` 负责：
  - `EnsureActorWorkbenchBodyInjected()`
  - `HideSharedContextStripForActorPage()`
  - `EnsureActorTopContextStripInjected()`
  - `RI_EnsureInspectBodyLayout()`
  - 首帧 layout invalidation / prepass
  - 首帧上下文条填充
- `ScheduleDeferredOpenActorRefresh()` 不再负责“把位置修正回来”

### 验证信号

最低可接受验证应同时包含：

1. 控制台入口
- `ri open actor`
- `ri showpage actor`
- 如果 app 侧 UE MCP 当前离线，使用 `Scripts/ValidateActorPageLayout.ps1`

2. 结构读数
- `GetActorPropertyHostDebugSummaryForAutomation()`
- `ActorTopContextStrip` 父级是 `ActorWorkbenchSidebarHost`
- `ActorTopContextStrip` 在 sidebar 索引 `0`
- 搜索框 direct child 在其后
- `SharedContextStrip` 在 `Actor` 页不可见

3. live 画面
- 首次打开就位于顶部
- 底部不再出现旧上下文条

### 后续准则

- 再遇到“第一次错、第二次对”，先查 authority 是否分叉
- 不要继续给 deferred refresh 加补丁
- 不要再让 `SharedContextStrip` 参与 `Actor` 页首帧

## 2. UE bridge 显示 active=true 但实际连不上

### 症状

- `Saved/UE_MCP_Bridge/bridge_state.json` 显示 `active=true`
- 但真实 `Test-NetConnection 127.0.0.1:<port>` 失败
- `ValidateActorPageLayout.ps1` 会卡在 bridge ready / screenshot / websocket 调用

### 这类问题的真实含义

这不是 RuntimeInspector UI 自身坏了，而是验证层 authority 失真。
只要 bridge state file 报活但真实端口没监听，所有基于它的截图、自测和远程调用结果都不可信。

### 本次根因

Windows 当前机器把 `9877-9976` 划进了 TCP excluded port range。
旧的 `UE_MCP_Bridge` editor 端固定绑 `9877`，runtime 端固定扫 `9897-9901`，都落在禁区里。
同时 editor bridge 旧逻辑只要线程创建成功就把 state file 写成 `active=true`，即使底层 `bind()` 已经失败。

### 有效修复

- editor bridge 改成动态扫描 `12077-12086`
- runtime bridge 改成动态扫描 `12097-12101`
- 所有验证脚本和 agent 读取实际 state file 里的端口，不再硬编码
- 遇到 `active=true` 时，必须再做一次真实端口探测，不能只信 json

### 验证信号

最低可接受验证应同时包含：

1. state file
- `bridge_state.json` 端口不再是 `9877`
- `bridge_state_runtime.json` 端口不再落在 `9897-9901`

2. 真实监听
- `Test-NetConnection 127.0.0.1:<bridge_state.port>` 为 `True`

3. 上层结果
- `ValidateActorPageLayout.ps1` 能走完并产出截图
- bridge RPC 能成功跑 `toggle_input` / `run_runtime_self_test`

## 2. 运行时 UMG 注入问题的最小证据包

后续如果再出现“改了 UI，但实际页不对”，最少要保留这几项证据：

- 当前 live 窗口截图
- `propertyHostDebug`
- `propertyAnchorChain`
- `inspectBodyChildren`
- `propertyItemCount`
- `propertyRowWidgetCount`

只要这套证据齐了，通常就能快速判断是：

- 数据问题
- 选择问题
- 宿主问题
- 布局问题

而不是靠反复试错。

## 3. 为什么 agent 看到的和用户看到的不一样

### 本次真实原因

这次不是单纯审美判断失误，而是 **验证画面本身就不是同一个窗口**。

之前 agent 主要看的是：

- `Preview` 独立窗口截图
- 或者 bridge 的 geometry / row-count 诊断

但用户实际看到的是：

- Unreal Editor 主窗口
- 中央 viewport 上方的浮动 RuntimeInspector 面板
- 同时带有 Outliner / Details / 其它编辑器 chrome

如果只看 `Preview`，会漏掉：

- 面板与主编辑器环境的比例关系
- 实际可见的拥挤感、空洞感、左右失衡
- runtime 面板与编辑器 chrome 同屏时的真实视觉效果

### 工具层根因

本次 `capture_window_screenshot` 还有一个具体问题：

- 当 `windowTitleContains` 匹配到多个同名窗口时
- 旧实现直接取第一个匹配项
- 对 `PluginMaker - Unreal Editor` 这种情况，会命中一个很小的同名窗口壳
- 最终抓到的是 `237x39` 的错误截图，而不是主编辑器窗口

### 已修复

窗口匹配逻辑已经改为：

- 对同标题匹配结果，优先选择 **面积最大的可见窗口**

这能避免后续再把主编辑器截图错抓成一个很小的标题壳。

### 以后如何避免再次误判

如果需求涉及 **UI 视觉、比例、层级、丑不丑、奇不奇怪**，不能只看：

- `Preview` 独立窗口
- row count / geometry count
- “不是空白了”这类最低可用判断

必须至少补一张：

- Unreal Editor 主窗口截图

只有主编辑器窗口和用户实际使用场景一致时，才能对视觉质量下判断。

### 视觉验收的最低规则

后续只要是 RuntimeInspector UI 改动，agent 不应再仅凭 `Preview` 视图宣布“界面没问题”。

最低要同时看：

1. 主编辑器窗口截图
2. 运行态/PIE 实际截图
3. 几何与 row-count 诊断

三者一致，才允许下“视觉正确”结论。

## 4. Actor 根节点 transform source persist 看起来失败，但其实是运行时逻辑覆盖

### 现象

- `Actor` 根节点顶部 `Location / Rotation / Scale` 已经能编辑
- `Stage Runtime Edit` 和 `Apply To Source` 看起来也成功
- `source preview` 里能看到 root transform 已经变了
- 但下一次 PIE 时，`Actor` 的最终世界空间 transform 又回到 baseline
- 常见误判是：
  - “Blueprint source 没写进去”
  - “editor world placed instance 没同步”
  - “root component scale promote 还没修好”

### 本次真实根因

这次真正失败的不是 promote 管线，而是 **验收目标本身不稳定**。

`BP_TestVarsActor` 的 `Event Tick` 每帧都会执行：

- `SetActorScale3D(TestScale)`

所以即使 source 中的 `DefaultSceneRoot.RelativeScale3D` 已经被改对了，下一次 PIE 里 actor 的最终 scale 仍会被 gameplay 逻辑覆盖回 `TestScale`。

这意味着：

- `Actor` 根节点世界空间 transform UI 可以是对的
- source promote 也可以是对的
- 但如果验收目标本身在 Tick 中主动改 transform，最终运行时结果仍然会看起来像“没持久化”

### 关键证据链

遇到这类“source 预览对了，但 next PIE 最终值不对”的情况，不要先继续补 promote 代码。先看这几层：

1. source/template 是否已变

- 读取 Blueprint 模板或组件模板
- 确认 `RelativeLocation / RelativeRotation / RelativeScale3D` 是否已经是 patched 值

2. editor-world placed instance 是否已同步

- 看当前编辑器世界里的 actor / root scene component
- 如果 editor world 已经是 patched 值，问题通常就不在 `Apply To Source` 本身

3. Blueprint graph / runtime 逻辑是否又把它改回去

- 重点查：
  - `ConstructionScript`
  - `BeginPlay`
  - `Tick`
  - 显式 `SetActorLocation / Rotation / Scale`
  - 显式 `SetWorldLocation / Rotation / Scale`

如果这些逻辑存在，就不要把“最终运行时 transform 被覆盖”直接归因到 source persist 失败。

### 当前正确口径

- `Actor` 根节点顶部 world transform 块：是用户可见编辑入口
- transform source persistence 的发布级验收：仍应优先用 **稳定的 scene component target**
- 对会在 runtime 主动覆盖自身 transform 的 actor：
  - 允许运行时编辑
  - 不应拿它做“next PIE 最终值是否与 source 一致”的主验收对象

本项目当前稳定 acceptance target 是：

- `BP_TestVarsActor -> Cube (StaticMeshComponent)`

### 以后如何避免再误判

如果你看到：

- `source preview` 对
- `Apply To Source` 对
- 但下一次 PIE 最终 transform 不对

优先问的不是“是不是 promote 又坏了”，而是：

1. 这个 actor / component 会不会在 `ConstructionScript` / `Tick` 主动改 transform？
2. 当前验收 target 是不是稳定组件，而不是会被 gameplay 覆盖的 actor-root world transform？

只有先排掉运行时覆盖，后面的 promote 排障才有意义。
