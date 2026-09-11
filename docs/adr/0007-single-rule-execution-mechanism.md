# ADR-0007：kind 是元数据约定，执行机制唯一

- **状态**：已接受
- **日期**：2026-09-08
- **决策者**：Pride Leong
- **关联**：[00-design.md](../00-design.md) §2.1、§3.3、D7

## 背景

六种规则 kind（definition/invariant/transformation/behavior/evolution/relation）看起来像六个子系统。若每种 kind 各建一套执行机制，逆向还原、策略打分、叙事渲染都要乘六份实现，且新 kind 需要改引擎。

## 决策

- 所有规则共用一个骨架：`when → guard → effects` + 元数据（cost/probability/priority/reversible/narration）
- **kind 只影响两件事**：加载期走哪套校验器；运行期落入哪个调度段（每刻三段：演变→转化→行为；relation 编入行为段）
- 匹配、调度、事务性应用、不变量校验、补全、叙事化——全部 kind 无关，一条管线

## 后果

- 定义新"系统"只需写新规则，不需要改引擎（洪范不提供人物系统/经济系统，提供生成它们的机制）
- 某些 kind 的特化优化（如 RETE 索引对 transformation 的守恒结构）延后——统一性优先
- 个体规则分片（[0012](0012-instance-rules-as-data.md)）因此免费复用全部机制
