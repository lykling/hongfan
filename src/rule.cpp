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

#include "hongfan/rule.hpp"

#include <cstddef>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "hongfan/expr.hpp"

namespace hf {

std::string_view kind_name(Kind k) {
  switch (k) {
    case Kind::Definition:
      return "definition";
    case Kind::Invariant:
      return "invariant";
    case Kind::Transformation:
      return "transformation";
    case Kind::Behavior:
      return "behavior";
    case Kind::Evolution:
      return "evolution";
    case Kind::Relation:
      return "relation";
  }
  return "?";
}

std::optional<Phase> phase_of(Kind k) {
  switch (k) {
    case Kind::Evolution:
      return Phase::Evolution;
    case Kind::Transformation:
      return Phase::Transformation;
    case Kind::Behavior:
    case Kind::Relation:
      return Phase::Behavior;
    case Kind::Definition:
    case Kind::Invariant:
      return std::nullopt;
  }
  return std::nullopt;
}

namespace {

[[nodiscard]] std::string_view trim(std::string_view s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\r')) {
    s.remove_prefix(1);
  }
  while (!s.empty() && (s.back() == ' ' || s.back() == '\r')) {
    s.remove_suffix(1);
  }
  return s;
}

// 条件解析：找最早出现的比较运算符，切成 属性 op 表达式
[[nodiscard]] std::expected<std::pair<std::string, std::pair<BinOp, std::string>>, std::string>
split_cond(std::string_view item) {
  for (size_t i = 0; i < item.size(); ++i) {
    BinOp op{};
    size_t len = 0;
    if (item.substr(i, 2) == "==") {
      op = BinOp::Eq;
      len = 2;
    } else if (item.substr(i, 2) == "!=") {
      op = BinOp::Ne;
      len = 2;
    } else if (item.substr(i, 2) == "<=") {
      op = BinOp::Le;
      len = 2;
    } else if (item.substr(i, 2) == ">=") {
      op = BinOp::Ge;
      len = 2;
    } else if (item[i] == '<') {
      op = BinOp::Lt;
      len = 1;
    } else if (item[i] == '>') {
      op = BinOp::Gt;
      len = 1;
    } else {
      continue;
    }
    const std::string prop{trim(item.substr(0, i))};
    const std::string rhs{trim(item.substr(i + len))};
    if (prop.empty() || rhs.empty()) {
      return std::unexpected(std::format("条件缺少属性或右值: {}", item));
    }
    return std::pair{prop, std::pair{op, rhs}};
  }
  return std::unexpected(std::format("条件缺少比较运算符: {}", item));
}

}  // namespace

std::expected<std::pair<std::string, std::string>, std::string> parse_binding_decl(
    std::string_view s) {
  const size_t colon = s.find(':');
  if (colon == std::string_view::npos) {
    return std::unexpected(std::format("声明缺少 ':'（应为 '变量: 类型'）: {}", s));
  }
  const std::string var{trim(s.substr(0, colon))};
  const std::string type{trim(s.substr(colon + 1))};
  if (var.empty() || type.empty()) {
    return std::unexpected(std::format("声明变量与类型不可为空: {}", s));
  }
  return std::pair{var, type};
}

std::expected<Pattern, std::string> parse_pattern(std::string_view s) {
  s = trim(s);
  const size_t colon = s.find(':');
  if (colon == std::string_view::npos) {
    return std::unexpected(std::format("模式缺少 ':'（应为 '变量: 类型 {{ 条件 }}'）: {}", s));
  }
  Pattern p;
  p.var = std::string(trim(s.substr(0, colon)));
  std::string_view const rest = trim(s.substr(colon + 1));
  if (p.var.empty() || rest.empty()) {
    return std::unexpected(std::format("模式变量与类型不可为空: {}", s));
  }
  std::string_view conds_str;
  const size_t brace = rest.find('{');
  if (brace == std::string_view::npos) {
    p.type = std::string(rest);
  } else {
    const size_t close = rest.rfind('}');
    if (close == std::string_view::npos || close < brace) {
      return std::unexpected(std::format("模式缺少 '}}': {}", s));
    }
    p.type = std::string(trim(rest.substr(0, brace)));
    conds_str = trim(rest.substr(brace + 1, close - brace - 1));
  }
  if (p.type.empty()) {
    return std::unexpected(std::format("模式类型为空: {}", s));
  }
  p.is_relation = p.type == "Relation";

  while (!conds_str.empty()) {
    const size_t comma = conds_str.find(',');
    const std::string_view item = trim(conds_str.substr(0, comma));
    conds_str =
        comma == std::string_view::npos ? std::string_view{} : trim(conds_str.substr(comma + 1));
    if (item.empty()) {
      continue;
    }
    auto cond = split_cond(item);
    if (!cond) {
      return std::unexpected(cond.error());
    }
    const auto& [prop, rhs_parts] = *cond;
    auto rhs = parse_expr(rhs_parts.second);
    if (!rhs) {
      return std::unexpected(rhs.error());
    }
    if (p.is_relation && prop == "type") {
      if (!std::holds_alternative<std::string>(rhs->node)) {
        return std::unexpected(std::format("关系模式 type 条件需为字符串字面量: {}", item));
      }
      p.rel_type = std::get<std::string>(rhs->node);
      continue;
    }
    if (p.is_relation && (prop == "src" || prop == "dst")) {
      (prop == "src" ? p.src : p.dst) = std::move(*rhs);
      continue;
    }
    p.conds.push_back(
        Cond{.prop = prop, .op = rhs_parts.first, .rhs = std::make_unique<Expr>(std::move(*rhs))});
  }
  return p;
}

std::expected<Effect, std::string> parse_effect(std::string_view s) {
  s = trim(s);
  const size_t sp = s.find(' ');
  if (sp == std::string_view::npos) {
    return std::unexpected(std::format("效果缺少参数: {}", s));
  }
  const std::string kw{trim(s.substr(0, sp))};
  const std::string_view rest = trim(s.substr(sp + 1));

  const auto parse_target = [&](std::string_view t, std::string& var,
                                std::string& prop) -> std::expected<void, std::string> {
    const size_t dot = t.find('.');
    if (dot == std::string_view::npos) {
      return std::unexpected(std::format("效果目标应为 '变量.属性': {}", t));
    }
    var = std::string(t.substr(0, dot));
    prop = std::string(t.substr(dot + 1));
    if (var.empty() || prop.empty()) {
      return std::unexpected(std::format("效果目标不完整: {}", t));
    }
    return {};
  };

  if (kw == "update" || kw == "set") {
    const size_t sp2 = rest.find(' ');
    if (sp2 == std::string_view::npos) {
      return std::unexpected(std::format("效果缺少运算: {}", s));
    }
    std::string var;
    std::string prop;
    if (auto t = parse_target(rest.substr(0, sp2), var, prop); !t) {
      return std::unexpected(t.error());
    }
    const std::string_view after = trim(rest.substr(sp2 + 1));
    if (kw == "update") {
      if (after.empty() || (after.front() != '+' && after.front() != '-')) {
        return std::unexpected(std::format("update 需要 '+' 或 '-': {}", s));
      }
      const bool is_add = after.front() == '+';
      auto amount = parse_expr(after.substr(1));
      if (!amount) {
        return std::unexpected(amount.error());
      }
      Effect e;
      e.op = Update{.var = std::move(var),
                    .prop = std::move(prop),
                    .is_add = is_add,
                    .amount = std::make_unique<Expr>(std::move(*amount))};
      return e;
    }
    if (after.empty() || after.front() != '=') {
      return std::unexpected(std::format("set 需要 '=': {}", s));
    }
    auto value = parse_expr(after.substr(1));
    if (!value) {
      return std::unexpected(value.error());
    }
    Effect e;
    e.op = Set{.var = std::move(var),
               .prop = std::move(prop),
               .value = std::make_unique<Expr>(std::move(*value))};
    return e;
  }

  if (kw == "relation") {
    std::vector<std::string_view> parts;
    std::string_view r = rest;
    while (!r.empty()) {
      const size_t p = r.find(' ');
      const std::string_view tok_s = trim(r.substr(0, p));
      if (!tok_s.empty()) {
        parts.push_back(tok_s);
      }
      r = p == std::string_view::npos ? std::string_view{} : r.substr(p + 1);
    }
    if (parts.size() != 3) {
      return std::unexpected(std::format("relation 需要 '类型 源 目标' 三个参数: {}", s));
    }
    auto src = parse_expr(parts[1]);
    if (!src) {
      return std::unexpected(src.error());
    }
    auto dst = parse_expr(parts[2]);
    if (!dst) {
      return std::unexpected(dst.error());
    }
    Effect e;
    e.op = RelAssert{.type = std::string(parts[0]), .src = std::move(*src), .dst = std::move(*dst)};
    return e;
  }

  return std::unexpected(std::format("未知效果类型 '{}': {}", kw, s));
}

}  // namespace hf
