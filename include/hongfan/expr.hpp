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
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "hongfan/rng.hpp"
#include "hongfan/value.hpp"
#include "hongfan/world.hpp"

namespace hf {

enum class UnOp : uint8_t { Not, Neg };
enum class BinOp : uint8_t { And, Or, Add, Sub, Mul, Div, Eq, Ne, Lt, Le, Gt, Ge };

[[nodiscard]] std::string_view bin_op_name(BinOp op);

struct Path {
  std::string root;
  std::optional<std::string> prop;  // nullopt / "id" = 实体引用（EntityId）
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;
struct Unary {
  UnOp op;
  ExprPtr operand;
};
struct Binary {
  BinOp op;
  ExprPtr lhs, rhs;
};
struct RandIntExpr {
  ExprPtr lo, hi;
};  // 仅效果表达式合法（加载期静态拒绝其余位置）

struct Expr {
  std::variant<double, bool, std::string, Path, Unary, Binary, RandIntExpr> node;
};

// 变量绑定：匹配产出 → 实体槽位 / 关系下标
struct Binding {
  enum class Kind : uint8_t { Entity, Relation };
  Kind kind;
  uint32_t ref;
};
using Bindings = std::map<std::string, Binding>;

struct EvalCtx {
  const World* world;
  const Bindings* bindings;  // 可为 nullptr（切面/目标求值：仅命名实体根）
  Rng* rng;                  // nullptr = 禁随机（when/guard/切面求值）
};

[[nodiscard]] std::expected<Value, std::string> eval(const Expr& e, const EvalCtx& ctx);
[[nodiscard]] std::expected<bool, std::string> eval_bool(const Expr& e, const EvalCtx& ctx);
[[nodiscard]] std::expected<bool, std::string> compare(BinOp op, const Value& a, const Value& b);

[[nodiscard]] std::expected<Expr, std::string> parse_expr(std::string_view src);
[[nodiscard]] std::string to_canonical(const Expr& e);
[[nodiscard]] bool contains_rand(const Expr& e);
// 表达式引用的路径根集合（静态绑定封闭性检查用）
[[nodiscard]] std::vector<std::string> path_roots(const Expr& e);

}  // namespace hf
