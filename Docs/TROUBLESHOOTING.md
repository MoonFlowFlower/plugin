# RuntimeInspector Troubleshooting

本文件记录 **已经真实遇到过的问题、证据链和修复方式**。

它不是产品 authority，也不是 UI 规范；authority 仍然以 `Docs/AGENT_DEVELOPMENT.md` 为准。  
它的用途是：后续再遇到类似问题时，先查这里，避免重复走弯路。

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
