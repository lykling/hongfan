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

#include "hongfan/expr.hpp"

#include <cmath>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "hongfan/id.hpp"
#include "hongfan/value.hpp"
#include "hongfan/world.hpp"

namespace hf {

std::string_view bin_op_name(BinOp op) {
  switch (op) {
    case BinOp::And:
      return "&&";
    case BinOp::Or:
      return "||";
    case BinOp::Add:
      return "+";
    case BinOp::Sub:
      return "-";
    case BinOp::Mul:
      return "*";
    case BinOp::Div:
      return "/";
    case BinOp::Eq:
      return "==";
    case BinOp::Ne:
      return "!=";
    case BinOp::Lt:
      return "<";
    case BinOp::Le:
      return "<=";
    case BinOp::Gt:
      return ">";
    case BinOp::Ge:
      return ">=";
  }
  return "?";
}

namespace {

// 路径解析：绑定优先，其次命名实体（切面/目标求值）
std::expected<Value, std::string> resolve_path(const Path& p, const EvalCtx& ctx) {
  if (ctx.world == nullptr) {
    return std::unexpected("求值上下文缺少世界");
  }
  const auto bind =
      ctx.bindings != nullptr ? ctx.bindings->find(p.root) : Bindings::const_iterator{};

  const auto entity_value = [&](const Entity& ent,
                                EntityId id) -> std::expected<Value, std::string> {
    if (!p.prop || *p.prop == "id") {
      return Value{id};
    }
    const auto it = ent.props.find(*p.prop);
    if (it == ent.props.end()) {
      return std::unexpected(std::format("属性不存在: {}.{}", p.root, *p.prop));
    }
    return it->second;
  };

  if (ctx.bindings != nullptr && bind != ctx.bindings->end()) {
    if (bind->second.kind == Binding::Kind::Entity) {
      const auto slot = bind->second.ref;
      if (slot >= ctx.world->entities.size()) {
        return std::unexpected("绑定槽位越界");
      }
      return entity_value(ctx.world->entities[slot], EntityId{slot});
    }
    const Relation& rel = ctx.world->relations.at(bind->second.ref);
    if (!p.prop) {
      return std::unexpected("关系绑定必须带属性（type/src/dst 或关系属性）");
    }
    if (*p.prop == "type") {
      return Value{rel.type};
    }
    if (*p.prop == "src") {
      return Value{rel.src};
    }
    if (*p.prop == "dst") {
      return Value{rel.dst};
    }
    const auto it = rel.props.find(*p.prop);
    if (it == rel.props.end()) {
      return std::unexpected(std::format("关系属性不存在: {}.{}", p.root, *p.prop));
    }
    return it->second;
  }
  if (const auto it = ctx.world->named.find(p.root); it != ctx.world->named.end()) {
    const auto slot = static_cast<uint32_t>(it->second);
    return entity_value(ctx.world->entities.at(slot), it->second);
  }
  return std::unexpected("未绑定标识: " + p.root);
}

}  // namespace

std::expected<bool, std::string> compare(BinOp op, const Value& a, const Value& b) {
  const bool same_type = a.v.index() == b.v.index();
  if (op == BinOp::Eq) {
    if (!same_type) {
      return false;
    }
    return a.v == b.v;
  }
  if (op == BinOp::Ne) {
    if (!same_type) {
      return true;
    }
    return !(a.v == b.v);
  }
  const auto x = as_num(a);
  const auto y = as_num(b);
  if (!x || !y) {
    return std::unexpected("序比较需要数值");
  }
  switch (op) {
    case BinOp::Lt:
      return *x < *y;
    case BinOp::Le:
      return *x <= *y;
    case BinOp::Gt:
      return *x > *y;
    case BinOp::Ge:
      return *x >= *y;
    default:
      break;
  }
  return std::unexpected("非法比较运算");
}

std::expected<Value, std::string> eval(const Expr& e, const EvalCtx& ctx) {
  if (const auto* d = std::get_if<double>(&e.node)) {
    return Value{*d};
  }
  if (const auto* b = std::get_if<bool>(&e.node)) {
    return Value{*b};
  }
  if (const auto* s = std::get_if<std::string>(&e.node)) {
    return Value{*s};
  }
  if (const auto* p = std::get_if<Path>(&e.node)) {
    return resolve_path(*p, ctx);
  }
  if (const auto* u = std::get_if<Unary>(&e.node)) {
    auto v = eval(*u->operand, ctx);
    if (!v) {
      return std::unexpected(v.error());
    }
    if (u->op == UnOp::Not) {
      if (const auto* bv = std::get_if<bool>(&v->v)) {
        return Value{!*bv};
      }
      return std::unexpected("'!' 需要布尔操作数");
    }
    if (const auto* dv = std::get_if<double>(&v->v)) {
      return Value{-*dv};
    }
    return std::unexpected("负号需要数值操作数");
  }
  if (const auto* b = std::get_if<Binary>(&e.node)) {
    if (b->op == BinOp::And || b->op == BinOp::Or) {
      auto l = eval_bool(*b->lhs, ctx);
      if (!l) {
        return std::unexpected(l.error());
      }
      if (b->op == BinOp::And && !*l) {
        return Value{false};  // 短路
      }
      if (b->op == BinOp::Or && *l) {
        return Value{true};
      }
      auto r = eval_bool(*b->rhs, ctx);
      if (!r) {
        return std::unexpected(r.error());
      }
      return Value{*r};
    }
    auto lv = eval(*b->lhs, ctx);
    if (!lv) {
      return std::unexpected(lv.error());
    }
    auto rv = eval(*b->rhs, ctx);
    if (!rv) {
      return std::unexpected(rv.error());
    }
    switch (b->op) {
      case BinOp::Add:
      case BinOp::Sub:
      case BinOp::Mul:
      case BinOp::Div: {
        const auto x = as_num(*lv);
        const auto y = as_num(*rv);
        if (!x || !y) {
          return std::unexpected("算术运算需要数值操作数");
        }
        if (b->op == BinOp::Div && *y == 0) {
          return std::unexpected("除零");
        }
        if (b->op == BinOp::Add) {
          return Value{*x + *y};
        }
        if (b->op == BinOp::Sub) {
          return Value{*x - *y};
        }
        if (b->op == BinOp::Mul) {
          return Value{*x * *y};
        }
        return Value{*x / *y};
      }
      case BinOp::Eq:
      case BinOp::Ne:
      case BinOp::Lt:
      case BinOp::Le:
      case BinOp::Gt:
      case BinOp::Ge: {
        auto r = compare(b->op, *lv, *rv);
        if (!r) {
          return std::unexpected(r.error());
        }
        return Value{*r};
      }
      case BinOp::And:
      case BinOp::Or:
        break;
    }
    return std::unexpected("不可达运算符");
  }
  const auto& r = std::get<RandIntExpr>(e.node);
  if (ctx.rng == nullptr) {
    return std::unexpected("此处禁止 rand_int（仅效果表达式可用）");
  }
  auto lo = eval(*r.lo, ctx);
  if (!lo) {
    return std::unexpected(lo.error());
  }
  auto hi = eval(*r.hi, ctx);
  if (!hi) {
    return std::unexpected(hi.error());
  }
  const auto lon = as_num(*lo);
  const auto hin = as_num(*hi);
  if (!lon || !hin) {
    return std::unexpected("rand_int 参数需要数值");
  }
  if (*lon < 0 || *hin < 0 || *lon > *hin) {
    return std::unexpected("rand_int 需满足 0 <= lo <= hi");
  }
  return Value{static_cast<double>(ctx.rng->rand_int(static_cast<uint64_t>(std::llround(*lon)),
                                                     static_cast<uint64_t>(std::llround(*hin))))};
}

std::expected<bool, std::string> eval_bool(const Expr& e, const EvalCtx& ctx) {
  auto v = eval(e, ctx);
  if (!v) {
    return std::unexpected(v.error());
  }
  if (const auto* b = std::get_if<bool>(&v->v)) {
    return *b;
  }
  return std::unexpected("需要布尔表达式");
}

std::string to_canonical(const Expr& e) {
  if (const auto* d = std::get_if<double>(&e.node)) {
    return std::format("{:.6f}", *d);
  }
  if (const auto* b = std::get_if<bool>(&e.node)) {
    return *b ? "true" : "false";
  }
  if (const auto* s = std::get_if<std::string>(&e.node)) {
    return "'" + *s + "'";
  }
  if (const auto* p = std::get_if<Path>(&e.node)) {
    return p->prop ? p->root + "." + *p->prop : p->root;
  }
  if (const auto* u = std::get_if<Unary>(&e.node)) {
    return std::format("{}{})", u->op == UnOp::Not ? "!(" : "-(", to_canonical(*u->operand));
  }
  if (const auto* b = std::get_if<Binary>(&e.node)) {
    return std::format("({} {} {})", to_canonical(*b->lhs), bin_op_name(b->op),
                       to_canonical(*b->rhs));
  }
  const auto& r = std::get<RandIntExpr>(e.node);
  return std::format("rand_int({}, {})", to_canonical(*r.lo), to_canonical(*r.hi));
}

namespace {

void walk(const Expr& e, bool& has_rand, std::vector<std::string>& roots) {
  if (const auto* p = std::get_if<Path>(&e.node)) {
    roots.push_back(p->root);
    return;
  }
  if (const auto* u = std::get_if<Unary>(&e.node)) {
    walk(*u->operand, has_rand, roots);
    return;
  }
  if (const auto* b = std::get_if<Binary>(&e.node)) {
    walk(*b->lhs, has_rand, roots);
    walk(*b->rhs, has_rand, roots);
    return;
  }
  if (std::holds_alternative<RandIntExpr>(e.node)) {
    has_rand = true;
  }
}

}  // namespace

bool contains_rand(const Expr& e) {
  bool has_rand = false;
  std::vector<std::string> roots;
  walk(e, has_rand, roots);
  return has_rand;
}

std::vector<std::string> path_roots(const Expr& e) {
  bool has_rand = false;
  std::vector<std::string> roots;
  walk(e, has_rand, roots);
  return roots;
}

}  // namespace hf
