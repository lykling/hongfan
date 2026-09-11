# 洪范 · HONGFAN

> 规则为世界之大法：万物由规则定义，演变由规则推衍，历史由规则还原。
> 《尚书·洪范》：「天乃锡禹洪范九畴，彝伦攸叙。」

## 这是什么

洪范是一个**可编程的世界推衍引擎**：世界的一切——物质、资源、人物、行为、环境、乃至时间与引擎自身和世界之间的联系——都表达为规则；引擎沿规则正向推衍世界演化，也能在任意**观测切面**之间补全合法的演变轨迹（还原历史、解释现状、规划未来），多解情况下按可插拔策略选取。世界不必整体推衍：它在**作用域**中分区运行，随**注意力**展开或压缩。

它不直接提供"人物系统""经济系统"，而是提供**生成这些系统的机制**——人物发展是行为规则的推衍结果，资源流转是转化规则的推衍结果，性格可以是被规则描述的事实。

## 一句话

用规则集定义世界的物理法则，引擎是"世界之心"：同一种子必长出同一个世界，同一废墟可还原出多种历史。

## 核心特性

- **一切皆规则**：定义/约束/转化/行为/演变/联系六种 kind 共用一条匹配-调度-补全-叙事管线
- **统一补全原语 `complete()`**：正向推衍、目标规划、界间还原（布朗桥式内插）、单事实溯源，皆为边界条件变体
- **确定性内核**：Run 四元组（规则集哈希， 状态哈希, 策略, 种子）→ 逐字节重放；黄金文件回归锁定
- **解空间三态**：唯一解=计算，多解=策略选取，矛盾=松弛求解（可信度加权，输出违背报告）
- **切面一等类型**：部分事实 + 时刻 + 可信度——观测本就部分且可错
- **作用域与注意力**（v0.3 概念，M6/M7 落地）：分区并行 + 消息定序因果偏序；展开/压缩对偶，延迟还原与惰性坍缩
- **编年史**：推衍即叙事——每步带叙事模板，按刻渲染中文编年史

## 与同类系统的关系

| 系统 | 洪范取用 | 差异 |
|---|---|---|
| 产生式系统（CLIPS/Drools, RETE） | 匹配-消解-执行循环 | 增加时态图、切面补全、叙事输出 |
| 规划器（PDDL/SATPlan） | 反向搜索、目标回归 | 规划只是 complete() 的前向边界形态 |
| 溯因推理 | 多解释、择优 | 显式策略接口 + 正向重放验证 + 矛盾松弛 |
| 离散事件仿真 / Actor 模型 | 事件驱动时间、消息定序 | 时间模型可换；消息是带可信度的切面 |
| 随机过程 / 布朗桥 | P(轨迹\|观测) 的数学原型 | 离散、规则驱动、可叙事 |

完整对照见 [00-design.md](./docs/00-design.md) 附录 B。

## 项目状态

版本 0.1.0（见 [VERSION](./VERSION)）。路线图七里程碑，**M1 已完成**：

| 里程碑 | 内容 | 状态 |
|---|---|---|
| M1 骨架 | 确定性在线补全 + 界间补全 + CLI + 两个样例世界 | ✅ 已完成（GCC/Clang 双编译器 6/6 测试） |
| M2 言语 | DSL 全量校验、叙事渲染完善 | ⏳ |
| M3 稽疑 | 反向搜索 + 重放验证 + 矛盾松弛 | ⏳ |
| M4 通衢 | API/REPL、快照分支、explain 溯源 | ⏳ |
| M5 分层 | 分层补全（宏观骨架 + 微观细节） | ⏳ |
| M6 域界 | 作用域树、分区并行、ScopeProfile | ⏳ |
| M7 注视 | 注意力展开/压缩、个体规则、LLM 接缝原型 | ⏳ |

## 快速上手

要求：CMake ≥ 3.25、Ninja、GCC ≥ 13 或 Clang ≥ 17（本仓库以 GCC 15 基准、Clang 22 移植门禁验证）。

```bash
# 构建（预设见 CMakePresets.json：dev / release / asan / ubsan）
cmake --preset release && cmake --build --preset release

# Demo 1：村落生息——在线补全（正向推衍 8 刻，编年史 + 终态快照）
./build/release/hongfan derive \
  --rules examples/village/rules.yaml --state examples/village/initial.yaml \
  --ticks 8 --strategy random --seed 42

# Demo 2：旅人稽疑——界间补全（160 条合法历史，按策略选出一条）
./build/release/hongfan complete \
  --rules examples/traveler/rules.yaml --state examples/traveler/initial.yaml \
  --goal examples/traveler/goal.yaml --depth 6 --strategy min-cost --seed 7 --list 3

# 测试（5 个单测 + e2e：确定性双跑比对 + 黄金文件回归）
ctest --test-dir build/release --output-on-failure
```

同一命令跑两遍输出逐字节一致——这就是确定性契约。换 `--strategy min-cost` 或 `--seed` 看同一世界的不同命运。

## 文档索引

| 入口 | 内容 |
|---|---|
| [docs/README.md](./docs/README.md) | 文档导览（三条阅读路径） |
| [docs/00-design.md](./docs/00-design.md) | 概念设计 v0.4（补全原语/切面/作用域/个体规则/注意力/观测落盘/求解器层） |
| [docs/01-implementation.md](./docs/01-implementation.md) | 落地实现设计（M1 数据结构/确定性契约/任务分解） |
| [docs/adr/](./docs/adr/README.md) | 架构决策记录 ADR-0001…0019 |
| [CHANGELOG.md](./CHANGELOG.md) | 变更日志 |

## 命名与品牌（洪范九畴）

模块借用《尚书·洪范》九畴命名，中文取意写进各模块文档首行：

| 模块 | 取意 | 职责 |
|---|---|---|
| `wuxing` 五行 | 物质与转化 | transformation 语义、守恒校验 |
| `bazheng` 八政 | 行为与施为 | behavior 语义、actor 调度 |
| `shuzheng` 庶征 | 环境征候 | evolution 语义、时间驱动 |
| `wuji` 五纪 | 时间与历法 | TimeModel、作用域局部时钟、惰性跳步 |
| `wushi` 五事 | 观与注视 | 注意力（焦点/展开/压缩） |
| `huangji` 皇极 | 内核 | 在线补全、事务性状态转移 |
| `sande` 三德 | 裁决 | 冲突消解、策略组合 |
| `jiyi` 稽疑 | 溯因求解 | 离线补全、松弛、重放验证 |
| `chronicle` 编年史 | （增畴） | 推衍日志→叙事渲染 |

## 维护者

Pride Leong

## 许可证

Apache-2.0 —— 见 [LICENSE](./LICENSE) 与 [NOTICE](./NOTICE)
