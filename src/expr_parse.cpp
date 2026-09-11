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

#include <cstddef>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "hongfan/expr.hpp"

#include "expr_tok.hpp"

namespace hf {
namespace {

[[nodiscard]] Expr mk_num(double d) {
  Expr e;
  e.node = d;
  return e;
}
[[nodiscard]] Expr mk_str(std::string s) {
  Expr e;
  e.node = std::move(s);
  return e;
}
[[nodiscard]] Expr mk_un(UnOp op, Expr operand) {
  Expr e;
  e.node = Unary{.op = op, .operand = std::make_unique<Expr>(std::move(operand))};
  return e;
}
[[nodiscard]] Expr mk_bin(BinOp op, Expr l, Expr r) {
  Expr e;
  e.node = Binary{.op = op,
                  .lhs = std::make_unique<Expr>(std::move(l)),
                  .rhs = std::make_unique<Expr>(std::move(r))};
  return e;
}

struct Parser {
  const std::vector<Tok>* t;
  size_t i = 0;

  [[nodiscard]] const Tok& cur() const { return (*t)[i]; }
  void adv() { ++i; }
  [[nodiscard]] bool op_is(std::string_view o) const {
    return cur().k == Tok::K::Op && cur().text == o;
  }

  std::expected<Expr, std::string> parse_or() {
    auto l = parse_and();
    if (!l) {
      return l;
    }
    while (op_is("||")) {
      adv();
      auto r = parse_and();
      if (!r) {
        return r;
      }
      l = mk_bin(BinOp::Or, std::move(*l), std::move(*r));
    }
    return l;
  }

  std::expected<Expr, std::string> parse_and() {
    auto l = parse_cmp();
    if (!l) {
      return l;
    }
    while (op_is("&&")) {
      adv();
      auto r = parse_cmp();
      if (!r) {
        return r;
      }
      l = mk_bin(BinOp::And, std::move(*l), std::move(*r));
    }
    return l;
  }

  std::expected<Expr, std::string> parse_cmp() {
    auto l = parse_add();
    if (!l) {
      return l;
    }
    static constexpr std::pair<std::string_view, BinOp> k_cmps[] = {
        {"==", BinOp::Eq}, {"!=", BinOp::Ne}, {"<=", BinOp::Le},
        {">=", BinOp::Ge}, {"<", BinOp::Lt},  {">", BinOp::Gt},
    };
    for (const auto& [txt, op] : k_cmps) {
      if (op_is(txt)) {
        adv();
        auto r = parse_add();
        if (!r) {
          return r;
        }
        return mk_bin(op, std::move(*l), std::move(*r));
      }
    }
    return l;
  }

  std::expected<Expr, std::string> parse_add() {
    auto l = parse_mul();
    if (!l) {
      return l;
    }
    while (op_is("+") || op_is("-")) {
      const BinOp op = op_is("+") ? BinOp::Add : BinOp::Sub;
      adv();
      auto r = parse_mul();
      if (!r) {
        return r;
      }
      l = mk_bin(op, std::move(*l), std::move(*r));
    }
    return l;
  }

  std::expected<Expr, std::string> parse_mul() {
    auto l = parse_unary();
    if (!l) {
      return l;
    }
    while (op_is("*") || op_is("/")) {
      const BinOp op = op_is("*") ? BinOp::Mul : BinOp::Div;
      adv();
      auto r = parse_unary();
      if (!r) {
        return r;
      }
      l = mk_bin(op, std::move(*l), std::move(*r));
    }
    return l;
  }

  std::expected<Expr, std::string> parse_unary() {
    if (op_is("-")) {
      adv();
      auto u = parse_unary();
      if (!u) {
        return u;
      }
      return mk_un(UnOp::Neg, std::move(*u));
    }
    if (op_is("!")) {
      adv();
      auto u = parse_unary();
      if (!u) {
        return u;
      }
      return mk_un(UnOp::Not, std::move(*u));
    }
    return parse_primary();
  }

  std::expected<Expr, std::string> parse_primary() {
    const Tok& c = cur();
    switch (c.k) {
      case Tok::K::Num: {
        adv();
        return mk_num(c.num);
      }
      case Tok::K::Str: {
        adv();
        return mk_str(c.text);
      }
      case Tok::K::LParen: {
        adv();
        auto e = parse_or();
        if (!e) {
          return e;
        }
        if (cur().k != Tok::K::RParen) {
          return std::unexpected(std::string("表达式缺少 ')'"));
        }
        adv();
        return e;
      }
      case Tok::K::Ident: {
        if (c.text == "true") {
          adv();
          Expr e;
          e.node = true;
          return e;
        }
        if (c.text == "false") {
          adv();
          Expr e;
          e.node = false;
          return e;
        }
        if (c.text == "rand_int") {
          adv();
          if (cur().k != Tok::K::LParen) {
            return std::unexpected(std::string("rand_int 需要 '('"));
          }
          adv();
          auto lo = parse_or();
          if (!lo) {
            return lo;
          }
          if (cur().k != Tok::K::Comma) {
            return std::unexpected(std::string("rand_int 需要 ','"));
          }
          adv();
          auto hi = parse_or();
          if (!hi) {
            return hi;
          }
          if (cur().k != Tok::K::RParen) {
            return std::unexpected(std::string("rand_int 需要 ')'"));
          }
          adv();
          Expr e;
          e.node = RandIntExpr{.lo = std::make_unique<Expr>(std::move(*lo)),
                               .hi = std::make_unique<Expr>(std::move(*hi))};
          return e;
        }
        adv();
        Path p{.root = c.text, .prop = std::nullopt};
        if (cur().k == Tok::K::Op && cur().text == ".") {
          adv();
          if (cur().k != Tok::K::Ident) {
            return std::unexpected(std::string("路径 '.' 后缺少属性名"));
          }
          p.prop = cur().text;
          adv();
        }
        Expr e;
        e.node = std::move(p);
        return e;
      }
      case Tok::K::Op:
      case Tok::K::RParen:
      case Tok::K::Comma:
      case Tok::K::End:
        return std::unexpected(std::format("表达式意外符号: '{}'", c.text));
    }
    return std::unexpected(std::string("不可达词法符号"));
  }
};

}  // namespace

std::expected<Expr, std::string> parse_expr(std::string_view src) {
  std::string_view s = src;
  while (!s.empty() && (s.front() == ' ' || s.front() == '\r')) {
    s.remove_prefix(1);
  }
  while (!s.empty() && (s.back() == ' ' || s.back() == '\r')) {
    s.remove_suffix(1);
  }
  auto toks = lex(s);
  if (!toks) {
    return std::unexpected(toks.error());
  }
  Parser p{.t = &*toks, .i = 0};
  auto e = p.parse_or();
  if (!e) {
    return std::unexpected(e.error());
  }
  if ((*toks)[p.i].k != Tok::K::End) {
    return std::unexpected(std::format("表达式尾部有多余内容: '{}'", (*toks)[p.i].text));
  }
  return e;
}

}  // namespace hf
