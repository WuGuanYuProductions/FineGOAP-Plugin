# FineGOAP-Plugin

这是一份基于当前底层代码，专门为游戏策划（Game Designers）编写的蓝图配置指南。文档包含中文和英文双语版本，方便不同工作流的团队使用。

提示：由于底层代码已经为你配置了详细的英文 ToolTip，在虚幻引擎蓝图编辑器中，只要将鼠标悬停在对应的变量或函数上，即可随时查看功能提示。

📖 面向策划的 GOAP AI 配置文档 (中文版)
本系统是一个基于目标导向动作规划（GOAP）的 AI 框架。AI 会根据当前环境状态，自动从“动作库”中挑选一条代价最小的路径来达成“目标”。

一、 核心概念与状态字典 (States)
GOAP 的核心是状态 (States)，采用 [字符串 (FName) -> 整数 (int32)] 的键值对格式（例如："HasWeapon" : 1）。
系统包含两种状态：

局部状态 (Local World State)：由 GOAPComponent 维护，代表 AI 自身知晓的状态（如：自身血量、是否发现敌人）。
全局状态 (Global State)：由 GOAPGlobalSubsystem 维护，类似于游戏里的公共黑板。
经典用法（令牌分配）：用于限制同时攻击玩家的 AI 数量。例如定义全局状态 "AttackToken"，AI 准备攻击时 +1，攻击结束时 -1。
二、 如何配置目标 (Goal)
目标是 AI 想要达到的最终状态。你可以在蓝图中创建继承自 GOAPGoal 的蓝图类。

GoalName: 目标的唯一名称。
BasePriority: 基础优先级。数值越大，AI 越优先尝试完成此目标。
DesiredState: 期望状态。例如 {"TargetDead" : 1}。当满足这些状态时，目标视为达成。
GoalActions: 动作库（核心）。点击 + 号添加实例化动作，这些动作是专属于该目标的。
bDisable: 禁用开关。勾选后，系统将无视该目标（便于调试）。
可重写的蓝图函数：

CalculatePriority: 动态计算优先级（例如：血量越低，“逃跑”目标的优先级越高）。
IsGoalAchieved: 覆盖默认的字典检查逻辑，使用你自己的蓝图逻辑判断目标是否完成。
三、 如何配置动作 (Action)
动作是 AI 为了达成目标所执行的具体行为。在蓝图中创建继承自 GOAPAction 的类，并将它们添加到目标的 GoalActions 数组中。

ActionName: 动作名称。
BaseCost: 基础消耗。数值越低，AI 越倾向于使用这个动作。
Preconditions (前提条件)：执行此动作必须满足的状态字典。例如：{"HasWeapon" : 1}。
Effects (影响/结果)：动作执行成功后会改变的状态。例如：{"EnemyDead" : 1}。
InterruptCost (打断阈值)：默认为 -1（不可打断）。如果设置为 50，当另一个有效动作的动态 Cost 低于 50 时，当前动作会被强制打断。
Cooldown (冷却)：可以设置为直接填值 (Direct Value) 或者是通过修饰器 (Modifier) 动态计算冷却时间。
可重写的蓝图函数：

CheckProceduralPrecondition: 过程化前提检查（比如判断与目标的距离是否小于 200，返回布尔值）。
OnActionStart: 动作开始时执行一次。（规范：如果需要占用全局令牌，请在这里调用全局子系统的 ModifyGlobalState +1）。
OnActionTick: 动作执行期间每帧调用。返回 True 代表动作执行完毕。
OnActionEnd: 动作结束或被打断时调用一次。（规范：请务必在这里调用 ModifyGlobalState -1 释放令牌。底层已做防漏兜底，即使 AI 死亡也会强制触发此函数，绝不死锁）。
四、 动态消耗与修饰器 (Modifiers)
对于代价 (Cost) 和冷却 (Cooldown)，除了填固定数值，你还可以添加 GOAPModifier。

创建继承自 GOAPModifier 的蓝图，重写 CalculateModifier 函数，根据血量、距离等返回一个浮点数。
在编辑器中，你可以重写 CalculateEditorPreviewValue 模拟一个假数据，这样在编辑器里不用运行游戏也能预览 Cost 变化。
五、 调试工具 (Debugging)
在控制台或蓝图中调用以下函数：

ToggleGOAPDebug: 开启/关闭屏幕全局打印。
SwitchGOAPDebugTarget: 切换当前正在监控的 AI 目标。
DebugPrintGlobalStates: 打印全局令牌池的使用情况。
📖 GOAP AI Configuration Guide for Game Designers (English Version)
This system is a Goal-Oriented Action Planning (GOAP) AI framework. Based on the current environmental states, the AI will automatically select the lowest-cost path of actions from an "action pool" to achieve its "goal."

1. Core Concepts & States
The core of GOAP revolves around States, formatted as a Dictionary/Map of [String (FName) -> Integer (int32)] (e.g., "HasWeapon" : 1).
The system contains two types of states:

Local World State: Maintained by the GOAPComponent. Represents what the individual AI knows (e.g., its own health, whether it sees an enemy).
Global State: Maintained by the GOAPGlobalSubsystem. Acts as a public blackboard.
Classic Workflow (Token System): Used to limit how many AIs can attack the player simultaneously. Define a global state "AttackToken". The AI modifies it by +1 before attacking, and -1 when finished.
2. How to Configure Goals
A goal is the final state the AI wants to achieve. Create a Blueprint class inheriting from GOAPGoal.

GoalName: Unique identifier for the goal.
BasePriority: Base priority. Higher values mean the AI will prioritize this goal.
DesiredState: The target states (e.g., {"TargetDead" : 1}). When these are met, the goal is achieved.
GoalActions: The Action Pool (Crucial). Click the + icon to add instanced actions. These actions are exclusive to this specific goal.
bDisable: If checked, the GOAP solver will completely ignore this goal (useful for debugging).
Blueprint Overrides:

CalculatePriority: Dynamically calculate priority (e.g., the lower the HP, the higher the priority of the "Flee" goal).
IsGoalAchieved: Override the default dictionary check to use your custom visual scripting logic to determine if the goal is met.
3. How to Configure Actions
Actions are the specific behaviors the AI executes to achieve goals. Create Blueprint classes inheriting from GOAPAction and add them to a Goal's GoalActions array.

ActionName: Name of the action.
BaseCost: Base execution cost. The pathfinder prefers actions with lower costs.
Preconditions: Required world states to execute this action (e.g., {"HasWeapon" : 1}).
Effects: World states that will be modified after this action successfully completes (e.g., {"EnemyDead" : 1}).
InterruptCost: Defaults to -1 (uninterruptible). If set to 50, this action will be aborted if another valid action's dynamic cost drops below 50.
Cooldown: Can be set to a Direct Value (seconds) or dynamically calculated using a Modifier.
Blueprint Overrides:

CheckProceduralPrecondition: Procedural checks (e.g., returning true if distance to target is < 200).
OnActionStart: Called once when the action begins. (Best Practice: If you need a global token, call ModifyGlobalState +1 here).
OnActionTick: Called every frame. Return True to tell the system the action is finished.
OnActionEnd: Called once when finished or aborted. (Best Practice: Always call ModifyGlobalState -1 here to release tokens. The underlying system guarantees this will be called even if the AI is suddenly destroyed, preventing deadlocks).
4. Dynamic Costs & Modifiers
For Cost and Cooldown, instead of fixed values, you can use GOAPModifier instances.

Create a Blueprint inheriting from GOAPModifier and override CalculateModifier to return a float based on real-time conditions (like distance or HP).
You can override CalculateEditorPreviewValue to provide mock data, allowing you to preview dynamic costs in the editor without pressing Play.
5. Debugging Tools
Use the following functions via Blueprints or console commands:

ToggleGOAPDebug: Toggle on-screen debug printing for the GOAP process.
SwitchGOAPDebugTarget: Cycle through available AI agents to monitor.
DebugPrintGlobalStates: Print the current values of all global states/tokens.
