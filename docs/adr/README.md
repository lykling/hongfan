# ADR 索引

> **编号规则**：ADR 编号是纯时间序流水号（Nygard 格式），动笔时分配，不预占位、不按域分段。
> 分类检索用本索引的域标签；ADR 正文互链靠编号稳定。
> 新增 ADR 后必须更新本表。

## 状态图例

✅ 已接受 · 🔁 已被修订/部分取代（见备注） · ⏸ 已搁置

## 索引

| ADR | 标题 | 域 | 状态 | 一句话摘要 |
|---|---|---|---|---|
| [0001](./0001-world-temporal-attribute-graph.md) | 世界状态 = 时态属性图 | state | ✅ | 联系一等公民；as-of 查询；有序容器是确定性契约的地基 |
| [0002](./0002-yaml-dsl-restricted-expr.md) | YAML DSL + 受限表达式 | dsl | ✅ | 禁宿主语言代码换可分析/可逆向/可复现；M1 自研受控 YAML 子集 |
| [0003](./0003-deterministic-kernel-seeded-rng.md) | 确定性内核 + 种子化 RNG | core | ✅ | Run 四元组重放；SplitMix64 唯一随机源，消耗点全量固定 |
| [0004](./0004-unified-completion-primitive.md) | 统一补全原语 complete() | core | ✅ | 推衍/规划/还原/explain/创世皆为边界条件变体 |
| [0005](./0005-scope-tree-layering.md) | 分层世界结构由作用域树承载 | core | ✅ | 层需要实体/时钟/信息三边界，纯元数据给不了（v0.3 修订） |
| [0006](./0006-immutable-content-addressed-rulesets.md) | 规则集不可变 + 内容寻址 | infra | ✅ | ruleset://{name}@{content_hash}；narration 参与哈希 |
| [0007](./0007-single-rule-execution-mechanism.md) | kind 元数据约定，执行机制唯一 | dsl | ✅ | 六种 kind 共用一条匹配/调度/补全/叙事管线 |
| [0008](./0008-slice-first-class-type.md) | 切面为一等类型 | state | ✅ | 部分事实 + 时刻 + 可信度；完整快照是特例 |
| [0009](./0009-three-state-solution-space.md) | 解空间三态 | core | ✅ | 唯一解=计算，多解=策略，矛盾=松弛（违背度×可信度加权） |
| [0010](./0010-pluggable-time-model.md) | 时间模型接口化 | core | ✅ | tick 仅为默认实现；因果序声明式；顺序本身可为自由度 |
| [0011](./0011-partitioned-determinism.md) | 分区确定性 | core | ✅ | 区内规范序不变；跨区消息定序编织因果偏序；无全局同时刻 |
| [0012](./0012-instance-rules-as-data.md) | 个体规则 = 数据 | dsl | ✅ | 规则分片附加于个体；可生成、可时变、计入寻址哈希 |
| [0013](./0013-scope-profile-composition.md) | ScopeProfile 组合寻址 | infra | ✅ | from/without/attach 纯函数组合，Nix-closure 式确定性闭包 |
| [0014](./0014-attention-expand-condense.md) | 注意力 = 展开/压缩对偶 | core | ✅ | 进入=细节推衍，离开=有损投影（佚失）；还原=稽疑+惰性坍缩 |
| [0015](./0015-cpp-implementation-language.md) | 实现语言 = C++ | infra | ✅ | C++23 编译（仅 expected/format/print）；GCC 基准 + Clang 移植门禁；零运行时依赖 |
| [0016](./0016-llm-boundary.md) | 大模型边界：创作与讲述，不裁决 | llm | ✅ | Run 复现域零 LLM 调用；六接缝各有校验边界 |
| [0017](./0017-language-policy.md) | 语言政策：中文之声，英文提交 | infra | ✅ | 文档/注释/字符串中文；提交英文（机制强制，此处只记决策） |
| [0018](./0018-observation-materialization.md) | 观测即落盘与世界线绑定 | core | ✅ | 坍缩写入史册层，重观测幂等；fork(k) 多元观测各线独立落盘 |
| [0019](./0019-solver-proposal-verification.md) | 求解器层：提案与验证分离 | core | ✅ | 求解器只提案、内核重放验证；代价模型扩展至转移代价/轨迹泛函 |
