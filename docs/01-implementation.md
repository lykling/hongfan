# 洪范落地实现设计

| | |
|---|---|
| 版本 | v0.1（实现设计） |
| 日期 | 2026-09-11 |
| 上游 | [00-design.md](./00-design.md) v0.3（概念设计）——本文把概念落为可实现的工程方案 |
| 状态 | ADR-1/ADR-2 已接受（正式编号 ADR-0015/0016）；M1 已实现 |

**版本记录**

| 版本 | 变更 |
|---|---|
| v0.1 | 首版：语言 ADR（C++20）、大模型角色 ADR、M1 范围裁剪、数据结构与子系统精确设计、确定性契约、样例世界、任务分解 |
| v0.1.1 | M1 实现落地勘误：YAML 依赖 ryml→自研受控子集解析器（分发资产不可得）；编译基准 C++20→C++23（仅用 expected/format/print）；候选规范序补 priority 降序；Symbol 内部化推迟（M1 直接用字符串）；注记 M1 语义（每段至多一次应用、效果求值基于应用前快照、库目标名 hongfan_core） |

---

## 0. 三问三答（速览）

1. **为什么不能是 C 或 C++？** C++20 完全可行且是本项目的推荐选择；C 缺 ADT/RAII，纯负担排除。此前 Rust 的建议优化的是"工件的一般性"，没优化"维护者契合"——修正。
2. **走大模型可不可行？** 当推衍核心：不可行（不可复现、不可验证、成本不规模，三杀）。当外围组件：可行且是将来核心竞争力——**大模型创作与讲述，引擎裁决与验证**。六个接缝见 ADR-2。
3. **落地实现方案？** 即本文档：M1 范围、精确数据结构、子系统设计、确定性契约、样例世界、任务清单。

---

## 1. ADR-1：实现语言（正式编号 [ADR-0015](./adr/0015-cpp-implementation-language.md)，已接受）

### 1.1 结论

- **核心引擎：C++**（实现以 `-std=c++23` 编译——仅用 std::expected / std::format / std::print 三项 C++23 设施，其余 C++20 内）。基准：GCC ≥ 13 / Clang ≥ 17（本机 GCC 15.3 / Clang 22.1，已验证）
- **编辑器/可视化/工具链：TypeScript**（远期，进程外或 WASM，不在 M1）
- **C 排除**：无 std::variant/std::expected/RAII 容器生态，洪范领域全是 tag union，手写 tag 结构是纯负担，无任何收益

### 1.2 C++20 vs Rust（针对洪范核心的逐维对比）

| 维度 | C++20 | Rust | 判定 |
|---|---|---|---|
| 稽疑搜索吞吐（克隆+匹配+求值+校验循环） | 同级 | 同级 | 平 |
| 确定性内核（有序容器、显式控制流） | std::map/排序可达，需约定 | BTreeMap 默认 | Rust 略优，C++20 用规范补齐（附录 A） |
| ADT 建模（Value/Expr/Effect/Kind） | std::variant + visit（缺一个匹配即编译失败，事实上穷尽） | enum + match 原生 | Rust 略优，C++20 足够 |
| DSL 边界解析 | ryml（快，API 偏原始） | serde 声明式 | Rust 优 |
| 错误处理 | tl::expected / 自备 Expected（C++23 才有 std::expected） | Result + `?` | Rust 优 |
| **维护者契合** | **主场：日常读写评审** | 需要爬坡 | **C++ 决定性优** |
| 库分发 | C ABI / 静态库顺；WASM 走 Emscripten 笨重 | cargo + wasm-bindgen 顺 | 看分发形态 |
| 依赖面 | 极少（ryml 一个） | serde 全家（serde_yaml 已归档） | 平 |

**判定逻辑**：性能与确定性两边同级；Rust 的优势（serde、match 语法）是"写起来更顺"，C++ 的优势（维护者契合）是"活得下去"——洪范是长期项目，核心引擎的每次 diff 都要过维护者评审，这个权重压倒其余。将来若出现 WASM-first 的硬需求（编辑器内嵌引擎），再评估核心转 Rust 或双前端；M1 不为未来可能性付今天的复杂度。

### 1.3 工具链与依赖

- 构建：CMake ≥ 3.24 + Ninja；`-std=c++23`，警告集 `-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion`（GCC/Clang 通用子集）
- 编译器：**GCC 15.3 为基准**；Clang 22.1 作可移植性门禁（双编译器 -Werror 全清、6/6 测试双通过；测试目标对 doctest 豁免 clang 22 新告警）
- 依赖（M1 全部）：**doctest**（单头测试库，入库）；YAML 为**自研受控子集解析器**（ryml 分发资产不可得；子集：块映射/块序列/单行 flow map/引号标量/整行注释）；哈希与随机数自实现（FNV-1a 64 / SplitMix64）
- 测试：doctest（已定案）；ctest 驱动（5 个单测二进制 + e2e 脚本：确定性双跑比对 + 黄金文件回归）
- 格式化：clang-format 入库（.clang-format）

---

## 2. ADR-2：大模型的角色（正式编号 [ADR-0016](./adr/0016-llm-boundary.md)，已接受）

### 2.1 为什么大模型不能当推衍核心

洪范的核心承诺（[00-design.md](./00-design.md) D3/D4）：**Run 四元组可复现、轨迹可验证、不变量必守**。大模型作为推衍器三者全破坏：

1. **不可复现**：同输入 ≠ 同输出（跨版本、跨部署、温度>0 必然，温度=0 也不保证稳定）；"同一种子长出同一个世界"失效
2. **不可验证**：模型输出无合规性保证，违反守恒/不变量时无机制发现——错误进入事实图后被无限放大
3. **成本不规模**：推衍是 O(tick)×O(规则) 量级的内循环；大模型调用是 O(1) 量级的外部服务。千刻推衍 × 每刻多候选 = 不可承受，且延迟把"世界之心"变成"世界之邮局"

### 2.2 大模型的正确位置：六个接缝

**原则：LLM 的输出永远是被校验的数据，或已定数据的投影，绝不充当合法性判据。**

| # | 接缝 | 输入 → 输出 | 校验边界（谁裁决） | 确定性影响 | 成本量级 |
|---|---|---|---|---|---|
| S1 | 规则创作 | 自然语言世界观 → YAML 规则草案 | 规则集加载校验器（类型/绑定封闭/守恒/因果序），拒收即拒收 | 无：产物是待发布规则集，走内容寻址 | O(1)/规则集 |
| S2 | 切面提取 | 史书/传说文本 → 结构化切面断言 + 可信度 | 切面 schema + 表达式静态检查 | 无：切面是输入数据 | O(1)/史料 |
| S3 | 叙事渲染 | 结构化 Step 序列 → 风格化编年史/人物传 | 无需裁决（数据已定，LLM 是投影器）；渲染文本**不参与**轨迹 hash | 无：渲染是投影，核心 hash 只算结构化 Step | O(编年史)，可缓存 |
| S4 | 偏好策略 | "还原一段战争史诗" → 打分/过滤谓词，或 LLM-as-ranker | 只能在**已验证的合法解集**内排序，错误上限=选了另一条合法解 | 选取记录进 Run 的 strategy_config（含模型/prompt 指纹） | O(解数)，受 k/beam 限界 |
| S5 | 搜索启发 | 目标 + 当前节点 → 优先尝试的规则/绑定序 | 引擎逐个正向验证；启发错=慢，不会=错 | 无：只影响效率 | O(扩展节点)，最贵的一个，须缓存+可降级到默认序 |
| S6 | 自由度填充 | 已定轨迹 + 空白自由细节（对话、形貌）→ 物化文本 | 显式分层：物化文本标记 ephemeral（可再生）或 commit-with-seed（带 provenance 入库） | 轨迹 hash 不含物化文本 | O(细节)，惰性触发 |

### 2.3 确定性保护条款

- 经典内核的 Run 复现域内**零 LLM 调用**
- LLM 产物入库一律走内容寻址工件 + provenance（模型标识/prompt hash/输出 hash），与规则集同等待遇——"这段历史是谁写的"可审计
- S4/S5 这类"影响选择"的接缝，其决策指纹（不是决策过程）记录进 Run 四元组的 strategy_config

**一句话：引擎是天道，大模型是史官与文人。天道管合法，史官管好看。**

---

## 3. M1 范围裁剪（DESIGN v0.2 → M1 差异表）

M1 目标：**两个可运行样例世界，跑通"在线补全→编年史"与"界间补全→多解→策略选取"两条主线**，确定性契约自始生效。

| DESIGN v0.2 能力 | M1 | 说明 |
|---|---|---|
| 规则 kind | definition / invariant / transformation / behavior / evolution / relation | 全有；transformation 用 update 表达（无独立 consume/produce 语法） |
| effects | update ± / set / relation-assert | ✅；consume/produce 语法 + Petri 守恒校验 → M2 |
| 切面 | 目标切面（终态事实集） | ✅；部分切面散布锚点/可信度/矛盾松弛 → M3 |
| 时间模型 | linear-tick（`every: N` 已支持） | ✅；event-driven/multi-rate/branching → M5；ordering 因果序 → M2 |
| 策略 | 在线 Random/MinCost/First；全局 Random/MinCost/PreferContains/List | ✅（全局即离线补全的选取） |
| 离线补全 | **前向 DFS 枚举 + 目标测试**（本身就是验证） | 反向应用 + beam + 双向汇合 → M2，接口已预留 |
| 编年史 | 模板渲染 + 终态快照 + Run 四元组 | ✅；分层多分辨率渲染 → M4 |
| 内容寻址 | FNV-1a 64（规范见附录 A） | ✅；升级 xxhash3 不改接口 |
| 表达式 | 字面量/路径/比较/算术/逻辑/rand_int(仅效果) | ✅；forall/聚合函数 → M2 |
| LLM 接缝 | 无 | S1/S3 先做原型验证（非 M1 阻塞项） |

---

## 4. 仓库与构建

```
hongfan/
├── docs/
│   ├── 00-design.md         # 概念设计（上游）
│   ├── 01-implementation.md # 本文档
│   └── adr/                 # 架构决策记录（ADR-0001…0017）
├── CMakeLists.txt
├── third_party/
│   └── doctest.h           # 单头测试库入库，无包管理
├── include/hongfan/        # 库公共接口
│   ├── id.hpp              # 强类型 Id（Symbol/EntityId/RuleId/Tick）
│   ├── value.hpp           # Value / PropMap
│   ├── world.hpp           # Entity / Relation / World + clone/hash
│   ├── expr.hpp            # Expr AST + 求值
│   ├── rule.hpp            # Pattern/Cond/Effect/Rule/RuleSet
│   ├── rng.hpp             # SplitMix64
│   ├── policy.hpp          # sande：在线/全局策略
│   ├── engine.hpp          # huangji：在线补全
│   ├── interpolate.hpp     # jiyi：界间补全
│   └── chronicle.hpp       # Step/轨迹/渲染
├── src/
│   ├── value world yaml(+flow) expr(+lexer+parse) rule ruleset(+check) state_load policy chronicle engine interpolate（各 .cpp，均 ≤250 纯行）
│   ├── 内部头：yaml_util.hpp  expr_tok.hpp  ruleset_internal.hpp
│   └── main.cpp            # CLI
├── examples/
│   ├── village/            # 村落生息（在线补全样例）
│   └── traveler/           # 旅人稽疑（界间补全样例）
└── tests/
    ├── expr_test  yaml_test  world_test  engine_test  interpolate_test（各 .cpp）
    ├── e2e.sh                # 确定性双跑比对 + 黄金文件回归
    └── golden/               # 编年史黄金文件（确定性回归）
```

构建产物：`hongfan`（CLI 可执行）+ `libhongfan.a`（库形态自始保持，防 CLI 与库逻辑纠缠）。

---

## 5. 核心数据结构（C++20 精确设计）

> 以下为设计基准代码（非最终逐字符实现，字段与语义以此为准）。

### 5.1 强类型 Id 与符号（`id.hpp`）

```cpp
enum class Symbol : uint32_t {};     // 内部符号（类型名/属性名/变量名），SymbolTable 内串→id
enum class EntityId : uint32_t {};   // 世界槽位下标（M1 实体不删除，槽位稳定）
enum class RuleId   : uint32_t {};   // 规则集内下标
enum class Tick     : uint32_t {};   // 离散刻

class SymbolTable {                  // 全局符号唯一化：比较=整数比较，hash 友好
  std::unordered_map<std::string, Symbol> index_;
  std::vector<std::string> names_;
public:
  Symbol intern(std::string_view);
  std::string_view name(Symbol) const;
};
```

> **M1 实现偏差**：Symbol/SymbolTable 未启用（直接用 std::string，M1 规模下无必要），推迟到匹配性能升级（§10 RETE）时一并引入。

### 5.2 值与世界（`value.hpp` / `world.hpp`）

```cpp
struct Value {
  std::variant<double, bool, std::string, EntityId> v;   // EntityId 供 src/dst 表达
};

using PropMap = std::map<std::string, Value>;            // 有序 → 序列化规范序

struct Entity   { std::string type; PropMap props; };
struct Relation { std::string type; EntityId src, dst; PropMap props; };

struct World {
  std::vector<Entity> entities;
  std::vector<Relation> relations;
  std::map<std::string, EntityId> named;   // 初始态命名实体（"pantry"）→ 槽位
  Tick now{0};

  World clone() const;                     // M1 事务应用 = 整体克隆（见 6.2 升级路径）
  uint64_t hash() const;                   // 规范序列化 + FNV-1a（附录 A.2）
};
```

**设计要点**：
- `std::map`（红黑树有序）而非 `unordered_map`——迭代序与序列化序天然规范，这是确定性契约的地基，不做任何"依赖 unordered 迭代序"的代码
- 实体删除与槽位置用（slotmap）→ M2（M1 无删除语义）
- 克隆成本：小世界（<10³ 实体）整克隆即可；COW/undo-log 见 §9

### 5.3 表达式（`expr.hpp`）

```cpp
struct Path { Symbol root; std::optional<Symbol> prop; };  // "p.stamina" / "p" / "p.id"

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

enum class UnOp { Not, Neg };
enum class BinOp { And, Or, Add, Sub, Mul, Div,
                   Eq, Ne, Lt, Le, Gt, Ge };

struct Expr {
  std::variant<
    double, bool, std::string,          // 字面量
    Path,
    std::pair<UnOp, ExprPtr>,
    std::pair<BinOp, std::pair<ExprPtr, ExprPtr>>,
    struct RandInt { ExprPtr lo, hi; }  // 仅允许出现在 effects 求值（加载期静态拒绝出现在 when/guard）
  > node;
};

struct Binding { enum class Kind { Entity, Relation }; Kind kind; uint32_t ref; };

struct EvalCtx {                        // 每次求值现场构造，不长期持有
  const World& world;
  const std::map<Symbol, Binding>& bindings;
  const Entity* implicit;               // 模式条件/不变量求值：裸属性名挂到隐式实体
  Rng* rng;                             // nullptr = 禁随机（when/guard/切面求值）
};

[[nodiscard]] tl::expected<Value, EvalError> eval(const Expr&, const EvalCtx&);
```

求值语义（normative）：
- 数值比较按 IEEE；`Eq` 对 string/string、bool/bool、EntityId/EntityId 同型比较；跨型 = EvalError
- `&&`/`||` 短路，操作数必须 bool
- 除零 = EvalError（不是 inf——确定性序列化不许 NaN/inf 出现）
- `rand_int(a,b)`：闭区间均匀整数，消耗一次 RNG（附录 A.3）

### 5.4 规则 IR（`rule.hpp` / `ruleset.cpp`）

```cpp
enum class Kind { Definition, Invariant, Transformation, Behavior, Evolution, Relation };
enum class Phase { Evolution, Transformation, Behavior };   // Relation 规则编入 Behavior 段

struct Cond  { Symbol prop; BinOp op; ExprPtr rhs; };       // 条件统一为比较式（等值 = Eq）

struct Pattern {                                            // "b: BerryBush { amount >= 2 }"
  std::string var;
  std::string type;                 // 实体类型或 "Relation"
  bool is_relation;
  std::vector<Cond> conds;          // 属性条件（隐式实体求值）
  std::optional<Expr> src, dst;     // 关系模式：可引用已绑定变量（外层求值）
};

struct Effect {
  std::variant<
    struct Update { std::string var; Symbol prop; bool is_add; ExprPtr amount; },
    struct Set    { std::string var; Symbol prop; ExprPtr value; },
    struct RelAssert { std::string type; Expr src, dst; }
  > op;
};

struct Rule {
  std::string id;                   // 唯一，字典序参与规范序
  Kind kind; std::string layer;
  std::vector<Pattern> when;
  ExprPtr guard;                    // 默认 true
  std::vector<Effect> effects;
  double cost{0}, probability{1};
  int priority{0};
  uint32_t every{1};                // evolution：tick % every == 0 才参与匹配
  std::string narration;            // 模板："{{h.name}} 采回野莓"
};

struct TypeSchema { std::map<std::string, std::map<std::string, std::string>> types; };  // type → prop → num|str|bool

struct RuleSet {
  std::vector<Rule> rules;          // 按 (kind段序, id字典序) 规范排序后存储
  TypeSchema schema;                // definition 聚合产物
  uint64_t hash;                    // 内容寻址（附录 A.2）
};
```

**加载期校验（ruleset.cpp，拒绝即报错并列出全部问题）**：
1. 规则 id 唯一；kind/字段合法性（invariant 必有 forall+always；definition 必有 entity+properties）
2. 类型引用存在（when/produce 的类型在 schema）；属性名在类型 schema 内
3. 变量绑定封闭：when 先声明后引用；guard/effects/src/dst 只用已绑定变量
4. `rand_*` 不出现在 when/guard/Cond（静态扫描 AST）
5. 数值范围：probability ∈ [0,1]，every ≥ 1

### 5.5 步与轨迹（`chronicle.hpp`）

```cpp
struct EffectRecord { std::string rendered; };              // "pantry.amount: 3 → 5"

struct Step {
  Tick tick; uint32_t seq;
  RuleId rule; Kind kind;
  std::map<std::string, std::string> bindings;              // 渲染后的绑定快照（"h" → "rock"）
  std::vector<EffectRecord> effects;
  std::string narration;                                    // 模板渲染结果
  bool rejected{false}; std::string reject_reason;          // 事务回滚记录
};

struct Trajectory {
  World start; std::vector<Step> steps; World final;
  uint64_t final_hash; double total_cost;
};
```

### 5.6 RNG（`rng.hpp`）

```cpp
struct Rng {                                   // SplitMix64：~20 行，跨平台位一致
  uint64_t state;
  uint64_t next_u64();
  double next_unit();                          // [0,1)，53-bit 构造
  uint64_t rand_int(uint64_t lo, uint64_t hi); // 闭区间，拒绝采样消模偏
  bool bernoulli(double p);
};
```

---

## 6. 子系统设计（按九畴模块）

### 6.1 wuji 五纪（时间 + 匹配调度段）

- `Tick` 单调推进；每刻三段固定序：**Evolution → Transformation → Behavior**（Relation 规则编入 Behavior 段，同类竞争）
- `every: N` 规则仅在 `tick % N == 0` 参与匹配

### 6.2 huangji 皇极（在线补全引擎）

每段执行流（normative，顺序不可变）：

```
1. cands = match(ruleset, phase, world)          // §6.5 匹配算法
2. gate  = 按规范序过滤 probability               // RNG 消耗点①：按 (规则序, 绑定序) 逐个 bernoulli
3. pick  = strategy.pick(gate)                   // RNG 消耗点②（Random 策略）
4. 对 pick 重校验（when/guard 对当前状态）         // 段内先前的应用可能已使其失效 → 丢弃本段不再补选（M1 语义）
5. scratch = world.clone()
6. 在 scratch 上按效果序求值并应用（rand_int = RNG 消耗点③）
7. invariant 全量校验 scratch → 通过：world = std::move(scratch)，记 Step
                            → 违反：丢弃，记 rejected Step（世界不动，不重选）
```

匹配（M1：朴素全扫）：

```
for rule in 段内规则（规范序）:
  DFS 绑定 when 模式（实体按槽位序，关系按声明序）
    每层先过 Cond（隐式实体求值），全部绑定后过 guard
    命中 → candidate(rule, bindings)
复杂度 O(段内规则 × 实体^模式数)；M1 世界（<10² 实体, <10 规则）毫秒级
```

### 6.3 sande 三德（策略）

```cpp
struct OnlinePolicy {                                          // 在线：每段一次咨询
  virtual std::optional<size_t> pick(std::span<const Candidate>, Rng&) = 0;
};
struct FirstPolicy / RandomPolicy / MinCostPolicy : OnlinePolicy;   // MinCost 取 rule.cost 最小，平手取先者

struct GlobalPolicy {                                          // 离线：解集整体选取
  virtual std::vector<Trajectory> select(std::vector<Trajectory>&&, Rng&) = 0;
};
struct GlobalRandom / GlobalMinCost / PreferContains(ruleId)+MinCost / ListK(k) : GlobalPolicy;
```

### 6.4 jiyi 稽疑（界间补全）

M1 实现 = **前向 DFS 枚举 + 目标测试**（枚举即正向执行，验证内建）：

```
complete_between(ruleset, start, goal_facts, depth, max_solutions):
  DFS(step):
    if depth == 0: 全部 goal_facts 在当前世界求值为 true → 记一条 Trajectory
    else:          对全部规则 × 全部绑定（同 §6.5 匹配）逐个：clone→apply→invariant→递归
  复杂度 O(分支^depth)，depth ≤ 8、世界小即秒级；深度即"史料缺失的刻数"
```

M2 升级（接口不变）：反向应用生成前驱 + 约束传播 + beam 限界——`complete()` 签名从 M1 起即按 DESIGN §3.4 固定，实现可换。

### 6.5 chronicle 编年史

- 模板语法：`{{var}}` → 实体名/关系名；`{{var.prop}}` → 属性值；`{{var.id}}` → 命名 id
- 输出按刻分组，行前缀打 kind 汉签：`[庶征] [五行] [八政] [联系]`；rejected 步打 `[否]` 前缀单列
- 终态快照：命名实体按字典序输出属性

---

## 7. DSL 子集与样例世界（M1 精确语法）

规则集 YAML（字段全集即 5.4 结构，样例即规范）：

```yaml
ruleset: village
time: { model: linear-tick }        # M1 仅 linear-tick，字段占位
rules:
  - rule: turn-rainy
    kind: evolution
    layer: geography
    when:
      - "w: World { weather == sunny }"      # Cond 语法：属性 <op> 表达式
    probability: 0.35
    effects:
      - "set w.weather = 'rainy'"
    narration: "乌云聚拢，雨落村落。"
```

**两个样例世界全文见 `examples/`（实现时随代码提交，此处定语义要点）**：

- `examples/village/`：World/BerryBush/Food/Person + neighbor 关系；规则覆盖全部六 kind；`feast` 规则（priority 5，消耗超库存）演示 invariant 回滚；在线补全 8 刻，Random 与 MinCost 产出不同编年史
- `examples/traveler/`：Traveler{stamina,skill}；trek/train/rest 三规则；目标切面 `skill == 3 && stamina >= 4`，depth 6 → 多解（解数 > 40），Random/MinCost/PreferContains 展示不同选取

---

## 8. CLI 契约（`src/main.cpp`）

```
hongfan derive  --rules R --state S --ticks N --strategy first|random|min-cost --seed U64
hongfan complete --rules R --state S --goal G --depth N --max-solutions K
                 --strategy random|min-cost|prefer:<rule> --seed U64 [--list K]

输出（stdout，退出码 0/1）：
  Run 四元组行:  run: ruleset=<hash16> state=<hash16> strategy=<name> seed=<u64>
  编年史:        按刻分组的事件行（含 [否] 拒绝行）
  derive  终态:  快照 + final_hash
  complete:      解数 + 选中轨迹的编年史 + total_cost +（--list 时前 K 条规则序列）
```

---

## 9. 测试与 M1 验收（DoD）

| 类别 | 内容 |
|---|---|
| 单元 | 表达式求值（优先级/短路/类型错/rand_int 边界）；模式匹配（含关系交叉引用）；效果应用；RNG 分布冒烟 |
| 引擎 | invariant 回滚（feast 场景：世界不变 + rejected Step 记录）；probability 门（种子化后命中序列确定） |
| **确定性** | 同 Run 四元组两次运行 → final_hash 与编年史逐字节一致；进程内与 CLI 双验证；编年史黄金文件入库 `tests/golden/` |
| 稽疑 | traveler 目标解数 ≥ 40；GlobalMinCost 选中无 trek 解（成本 7.5）；PreferContains(trek) 解含 trek |
| E2E | `§8` 两条命令实际跑通，输出即文档样例 |

---

## 10. 性能边界与升级路径（M1 不做，接口已留）

| 瓶颈 | 触发条件 | 升级路径 |
|---|---|---|
| 朴素匹配 | 规则×实体² > 10⁵/刻 | 类型桶 + RETE 式增量匹配（alpha/beta 网） |
| 整体克隆 | 世界 > 10³ 实体 | undo-log 增量回滚（效果逆操作）/ COW |
| 前向枚举稽疑 | depth > 8 或分支 > 10³ | M2：反向应用 + 约束传播 + beam |
| FNV-1a | 哈希碰撞进入实用域 | xxhash3，接口不变 |
| 单线程 | 段间并行收益显现 | 段内候选并行生成（确定性不破：规范序合并） |

---

## 11. M1 任务分解

| # | 任务 | 产出 | 依赖 |
|---|---|---|---|
| T1 | 基建：CMake + doctest 入仓 + .clang-format + 自研 YAML 子集解析器 | 空壳可构建 | — |
| T2 | id/value/world + FNV + 规范序列化 | `world_test` | T1 |
| T3 | rng（SplitMix64） | 确定性金测试 | T1 |
| T4 | expr：词法/语法/求值 + 静态 rand 检查 | `expr_test` | T2,T3 |
| T5 | rule/ruleset：YAML 解析 + 加载校验（§5.4 五条全实现） | `ruleset_test` | T4 |
| T6 | huangji：匹配 + 三段调度 + 事务应用 + Step | `engine_test`（含回滚/门控） | T5 |
| T7 | sande：在线/全局策略族 | `policy_test` | T6 |
| T8 | jiyi：complete_between DFS | `interpolate_test` | T6,T7 |
| T9 | chronicle 模板渲染 + CLI 两个子命令 | 手跑样例 | T6,T8 |
| T10 | 样例世界 village/traveler + 黄金文件 + E2E 脚本 | `tests/golden/*` | T9 |

---

## 附录 A：确定性契约（normative）

### A.1 总则

**Run = (ruleset_hash, state_hash, strategy_config, seed) → 唯一轨迹（含编年史字节序）。**

一切影响输出的选择必须由以下规范序完全决定，禁止任何未列出的顺序来源（线程调度、哈希迭代、地址、时间）。

### A.2 规范序列化与哈希

- 实体序列：按**命名 id 字典序**（命名实体），未命名实体 M1 不允许出现在初始态
- 属性序列：`std::map` 键序；数值格式：有限小数 6 位定点（禁科学计数/NaN/inf，除零即 EvalError）
- 规则集序列：按 (Kind 段序, id 字典序)；序列化含全部语义字段（不含 doc/narration 的原文注释性内容除外——narration 参与序列化，因为影响编年史）
- 哈希：FNV-1a 64，输出 16 hex

### A.3 RNG 消耗点（全量，顺序固定）

1. 每段 probability 门控：按候选规范序（A.4，priority 降序稳定序）逐候选 `bernoulli(p)`（p=1 的候选**不消耗** RNG，p=0 直接淘汰不消耗）
2. 在线策略 Random：每次 pick 恰一次 `rand_int(0, n-1)`
3. 效果内 `rand_int(a,b)`：按效果声明序，各恰一次
4. 全局策略 GlobalRandom：解集上恰一次

### A.4 匹配与绑定的规范序

- 候选规范序：先生成（规则按段内 id 序；绑定 DFS 中实体按**槽位序**、关系按**声明序**；同规则多绑定按 DFS 产出序），再**稳定排序 priority 降序**——此序即门控消耗序与策略咨询输入序
- 段内一次应用后不再补选（M1 语义，M1 内不变更）
- 效果求值基于**应用前快照**：同规则多效果互不可见（M1 语义）

### A.5 失败路径

- 求值错误/不变量违反 → 该候选应用失败：世界不动、消耗的 RNG **不回退**（消耗序列连续性优先）、记 rejected Step——失败也确定性
