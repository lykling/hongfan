# ADR-0013：ScopeProfile 组合寻址

- **状态**：已接受（v0.3）
- **日期**：2026-09-11
- **决策者**：Pride Leong
- **关联**：[00-design.md](../00-design.md) §8.6、D13；[0006](0006-immutable-content-addressed-rulesets.md)、[0012](0012-instance-rules-as-data.md)

## 背景

作用域需要灵活的规则视图：房间用 person-base + furniture-aging 但剔除 feast，阿岩再挂个体性格分片。若靠复制粘贴组织规则集，版本与来源迅速失控，且与[不可变规则集](0006-immutable-content-addressed-rulesets.md)冲突。

## 决策

作用域的规则视图是**纯函数组合描述**，本身内容寻址：

```yaml
scope_profile: room-101@v3              # 组合描述的哈希
from:      [rules/person-base, rules/furniture-aging]   # 类规则包
without:   [feast]                       # 剔除
attach:    rock { from: rules/性格-坚毅 }  # 个体分片
```

- **切分**：按 layer/scope 过滤规则包；**组合/重组**：from/without/attach；**提取**：过滤后重新发布为独立规则包
- 与 D6 完全兼容：组合描述是新的不可变工件——Nix-closure 式的确定性闭包

## 后果

- 个体分片（[0012](0012-instance-rules-as-data.md)）计入组合哈希——个体差异被可复现性覆盖
- 组合描述成为新的版本管理对象；同一组合描述在任何机器上解析出同一规则视图
