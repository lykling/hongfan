# ADR-0015：实现语言 = C++（GCC 基准 + Clang 移植门禁）

- **状态**：已接受（原 [01-implementation.md](../01-implementation.md) §1 ADR-1，v0.3 起为此正式编号）
- **日期**：2026-09-11
- **决策者**：Pride Leong
- **关联**：[01-implementation.md](../01-implementation.md) §1；[0017](0017-language-policy.md)

## 背景

语言候选：C / C++ / Rust。此前曾建议 Rust，但逐维对比后修正——稽疑搜索吞吐与确定性两边同级（std::map/BTreeMap 皆可达）；Rust 的优势（serde、match 语法）是"写起来更顺"，C++ 的优势（**维护者契合**：核心引擎的每次 diff 都要过维护者评审）是"活得下去"。洪范是长期项目，这个权重压倒其余。C 排除：无 variant/expected/RAII，洪范领域全是 tag union，手写是纯负担。

## 决策

- **C++，以 `-std=c++23` 编译**——仅用 std::expected / std::format / std::print 三项 C++23 设施，其余 C++20 内）
- **编译器策略**：GCC 15.3 为基准；Clang 22.1 作可移植性门禁（双编译器 -Werror 全清、测试双通过；测试目标对 doctest 豁免 clang 22 新告警）
- **依赖（M1 全部）**：doctest（单头测试库，vendored，见 ALLOWED_DEPS.txt）；YAML 为自研受控子集解析器（ryml 分发资产不可得）；哈希 FNV-1a 与 RNG SplitMix64 自实现
- 编辑器/可视化/工具链 TypeScript（远期）

## 后果

- M1 双编译器 6/6 测试全绿，零第三方运行时依赖
- 若出现 WASM-first 硬需求（编辑器内嵌引擎），再评估核心转 Rust 或双前端——M1 不为未来可能性付今天的复杂度
