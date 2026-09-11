# 洪范文档导览

> **文档定位**：docs/ 全树入口——不同读者按不同路径进入。
> 上层入口：[README.md](../README.md)（项目门面）。

---

## 三条阅读路径

| 你是谁 | 路径 |
|---|---|
| **5 分钟了解项目** | [README.md](../README.md) → [00-design.md](./00-design.md) §1–§2（愿景与核心理念） |
| **学习 / 参与开发** | [00-design.md](./00-design.md)（概念全景）→ [01-implementation.md](./01-implementation.md)（M1 数据结构与任务分解）→ 跑 [examples/](../examples/)（村落生息 + 旅人稽疑） |
| **查"当时为什么这么定"** | [adr/README.md](./adr/README.md)（按域检索）→ 具体 ADR |

## 目录结构

```
docs/
├── README.md                  ← 本导览
├── 00-design.md               概念设计 v0.3：补全原语 / 切面 / 作用域 / 个体规则 / 注意力
├── 01-implementation.md       落地实现设计：C++ ADR / LLM 边界 / M1 数据结构 / 确定性契约 / 任务分解
└── adr/                       架构决策记录（ADR-0001…0017）
    ├── README.md              索引（域标签 + 状态 + 一句话摘要）
    └── NNNN-slug.md           每条决策一页：背景 / 决策 / 后果
```
