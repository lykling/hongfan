# ADR-0006：规则集不可变 + 内容寻址

- **状态**：已接受
- **日期**：2026-09-08
- **决策者**：Pride Leong
- **关联**：[00-design.md](../00-design.md) §5.6、D6；[0013](0013-scope-profile-composition.md)

## 背景

世界演化必须可归因于明确的规则版本——否则"这条轨迹是哪套规则跑出来的"无法回答，平行世界对比失去基准。同时，"世界改朝换代（换规则集）而状态延续"是上层叙事的重要原语。

## 决策

- 规则集 = 不可变目录结构：`ruleset://{name}@{content_hash}`（FNV-1a 64，M1；升级 xxhash3 不改接口）
- 序列化含全部语义字段（narration 参与哈希——它影响编年史字节序）
- 规则集可组合（import + override），override 需显式声明被覆盖的规则 ID
- 组合升格为 ScopeProfile（v0.3，见 [0013](0013-scope-profile-composition.md)）

## 后果

- 编辑体验多一步"发布"；换来的是任何 Run 的规则侧输入可精确锁定
- Run 四元组里的 `ruleset_hash` 由此而来，与状态哈希、策略配置、种子共同构成复现键
