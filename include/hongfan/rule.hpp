// Copyright 2026 Pride Leong.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <cstdint>
#include <expected>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "hongfan/expr.hpp"
#include "hongfan/world.hpp"
#include "hongfan/yaml.hpp"

namespace hf {

enum class Kind : uint8_t { Definition, Invariant, Transformation, Behavior, Evolution, Relation };
enum class Phase : uint8_t { Evolution, Transformation, Behavior };

[[nodiscard]] std::string_view kind_name(Kind k);
[[nodiscard]] std::optional<Phase> phase_of(Kind k);  // definition/invariant → nullopt

struct Cond {
  std::string prop;
  BinOp op;
  ExprPtr rhs;
};

struct Pattern {
  std::string var;
  std::string type;  // 实体类型，或 "Relation"
  bool is_relation = false;
  std::vector<Cond> conds;              // 属性条件
  std::optional<std::string> rel_type;  // 关系模式：类型约束
  std::optional<Expr> src, dst;         // 关系模式：端点约束（引用先声明变量）
};

struct Update {
  std::string var, prop;
  bool is_add;
  ExprPtr amount;
};
struct Set {
  std::string var, prop;
  ExprPtr value;
};
struct RelAssert {
  std::string type;
  Expr src, dst;
};
struct Effect {
  std::variant<Update, Set, RelAssert> op;
};

struct InvariantSpec {
  std::string var, type;
  ExprPtr always;
};
struct DefinitionSpec {
  std::string entity;
  std::map<std::string, std::string> properties;
};

struct Rule {
  std::string id;
  Kind kind{};
  std::string layer;
  std::vector<Pattern> when;
  ExprPtr guard;  // 默认 true
  std::vector<Effect> effects;
  double cost = 0, probability = 1;
  int priority = 0;
  uint32_t every = 1;
  std::string narration;
  std::optional<InvariantSpec> invariant;
  std::optional<DefinitionSpec> definition;
};

using TypeSchema =
    std::map<std::string, std::map<std::string, std::string>>;  // 类型 → 属性 → num|str|bool

struct RuleSet {
  std::string name;
  std::vector<Rule> rules;  // 规范序：definition → invariant → 运行时（phase, id）
  TypeSchema schema;
  std::vector<size_t> schedule[3];  // 按 Phase 的运行时规则索引（规范序）
  std::vector<size_t> invariant_ids;
  uint64_t hash = 0;
};

// ---- DSL 字符串解析（供加载与测试直接使用）----

[[nodiscard]] std::expected<Pattern, std::string> parse_pattern(std::string_view s);
[[nodiscard]] std::expected<Effect, std::string> parse_effect(std::string_view s);
[[nodiscard]] std::expected<std::pair<std::string, std::string>, std::string> parse_binding_decl(
    std::string_view s);  // "变量: 类型"

// ---- 加载（收集全部问题一次报出）----

struct LoadProblems {
  RuleSet rs;
  std::vector<std::string> problems;  // 非空 = 拒绝加载
};
[[nodiscard]] LoadProblems load_ruleset(const yaml::Node& root);

[[nodiscard]] std::expected<World, std::vector<std::string>> load_world(const yaml::Node& root,
                                                                        const TypeSchema& schema);
[[nodiscard]] std::expected<std::vector<Expr>, std::vector<std::string>> load_goal(
    const yaml::Node& root, const World& w);

}  // namespace hf
