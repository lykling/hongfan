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

#include <format>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "hongfan/expr.hpp"
#include "hongfan/rule.hpp"

#include "ruleset_internal.hpp"

namespace hf {
namespace {

// 变量封闭性：roots ⊆ allowed
void check_roots(const std::vector<std::string>& roots, const std::set<std::string>& allowed,
                 const std::string& rid, const std::string& where,
                 std::vector<std::string>& problems) {
  for (const auto& root : roots) {
    if (!allowed.contains(root)) {
      problems.push_back(std::format("规则 {}: {} 引用未声明变量: {}", rid, where, root));
    }
  }
}

}  // namespace

// 第三遍：静态校验（变量封闭性 / 类型与属性存在 / rand 位置）
void validate_rules(const std::vector<Rule>& parsed, const TypeSchema& schema,
                    std::vector<std::string>& p) {
  for (const Rule& r : parsed) {
    if (r.kind == Kind::Definition) {
      continue;
    }
    if (r.kind == Kind::Invariant) {
      if (r.invariant && !schema.contains(r.invariant->type)) {
        p.push_back(std::format("规则 {}: 未知实体类型: {}", r.id, r.invariant->type));
      }
      continue;
    }
    std::set<std::string> declared;
    std::map<std::string, std::string> var_type;
    for (const Pattern& pat : r.when) {
      if (declared.contains(pat.var)) {
        p.push_back(std::format("规则 {}: 模式变量重复: {}", r.id, pat.var));
      }
      const auto check_expr = [&](const Expr& e, const char* where) {
        if (contains_rand(e)) {
          p.push_back(std::format("规则 {}: {} 禁止 rand_int", r.id, where));
        }
        check_roots(path_roots(e), declared, r.id, where, p);
      };
      if (pat.is_relation) {
        if (pat.src) {
          check_expr(*pat.src, "关系 src");
        }
        if (pat.dst) {
          check_expr(*pat.dst, "关系 dst");
        }
        for (const Cond& c : pat.conds) {
          check_expr(*c.rhs, "模式条件");
        }
      } else {
        const auto sit = schema.find(pat.type);
        if (sit == schema.end()) {
          p.push_back(std::format("规则 {}: 未知实体类型: {}", r.id, pat.type));
        } else {
          for (const Cond& c : pat.conds) {
            if (!sit->second.contains(c.prop)) {
              p.push_back(std::format("规则 {}: 类型 {} 无属性 {}", r.id, pat.type, c.prop));
            }
            check_expr(*c.rhs, "模式条件");
          }
        }
      }
      declared.insert(pat.var);
      var_type[pat.var] = pat.type;
    }
    if (r.guard) {
      check_roots(path_roots(*r.guard), declared, r.id, "guard", p);
    }
    for (const Effect& e : r.effects) {
      if (const auto* u = std::get_if<Update>(&e.op)) {
        check_roots(path_roots(*u->amount), declared, r.id, "效果", p);
        const auto it = var_type.find(u->var);
        if (it == var_type.end()) {
          p.push_back(std::format("规则 {}: 效果引用未声明变量: {}", r.id, u->var));
        } else if (it->second == "Relation") {
          p.push_back(std::format("规则 {}: 关系变量不可用于 update: {}", r.id, u->var));
        } else {
          const auto& props = schema.at(it->second);
          const auto pit = props.find(u->prop);
          if (pit == props.end()) {
            p.push_back(std::format("规则 {}: 类型 {} 无属性 {}", r.id, it->second, u->prop));
          } else if (pit->second != "num") {
            p.push_back(std::format("规则 {}: update 需要数值属性 {}.{}", r.id, u->var, u->prop));
          }
        }
      } else if (const auto* s = std::get_if<Set>(&e.op)) {
        check_roots(path_roots(*s->value), declared, r.id, "效果", p);
        const auto it = var_type.find(s->var);
        if (it == var_type.end()) {
          p.push_back(std::format("规则 {}: 效果引用未声明变量: {}", r.id, s->var));
        } else if (it->second == "Relation") {
          p.push_back(std::format("规则 {}: 关系变量不可用于 set: {}", r.id, s->var));
        } else if (!schema.at(it->second).contains(s->prop)) {
          p.push_back(std::format("规则 {}: 类型 {} 无属性 {}", r.id, it->second, s->prop));
        }
      } else {
        const auto& ra = std::get<RelAssert>(e.op);
        check_roots(path_roots(ra.src), declared, r.id, "关系 src", p);
        check_roots(path_roots(ra.dst), declared, r.id, "关系 dst", p);
      }
    }
  }
}

std::string canonical_ruleset(const RuleSet& rs) {
  std::string out = "hongfan-ruleset:v1|" + rs.name + "\n";
  for (const Rule& r : rs.rules) {
    out += std::format("{}|{}|{}|cost={:.6f};prob={:.6f};prio={};every={};narr={}\n", r.id,
                       kind_name(r.kind), r.layer, r.cost, r.probability, r.priority, r.every,
                       r.narration);
    for (const Pattern& p : r.when) {
      out += "  ?" + p.var + ":" + p.type;
      if (p.rel_type) {
        out += ":T=" + *p.rel_type;
      }
      for (const Cond& c : p.conds) {
        out += ';';
        out += c.prop;
        out += std::string(bin_op_name(c.op));
        out += to_canonical(*c.rhs);
      }
      const auto& src_expr = p.src;
      if (src_expr) {
        out += ";SRC=";
        out += to_canonical(*src_expr);
      }
      const auto& dst_expr = p.dst;
      if (dst_expr) {
        out += ";DST=";
        out += to_canonical(*dst_expr);
      }
      out += "\n";
    }
    if (r.guard) {
      out += "  guard=" + to_canonical(*r.guard) + "\n";
    }
    for (const Effect& e : r.effects) {
      if (const auto* u = std::get_if<Update>(&e.op)) {
        out += std::format("  fx {}.{} {} {}\n", u->var, u->prop, u->is_add ? "+" : "-",
                           to_canonical(*u->amount));
      } else if (const auto* fx = std::get_if<Set>(&e.op)) {
        out += std::format("  fx {}.{} = {}\n", fx->var, fx->prop, to_canonical(*fx->value));
      } else {
        const auto& ra = std::get<RelAssert>(e.op);
        out += std::format("  fx R{} {} {}\n", ra.type, to_canonical(ra.src), to_canonical(ra.dst));
      }
    }
    const auto& inv = r.invariant;
    if (inv) {
      out += "  inv " + inv->var + ":" + inv->type + " " + to_canonical(*inv->always) + "\n";
    }
    const auto& def = r.definition;
    if (def) {
      out += "  def " + def->entity;
      for (const auto& [prop, typ] : def->properties) {
        out += ';';
        out += prop;
        out += '=';
        out += typ;
      }
      out += "\n";
    }
  }
  return out;
}

}  // namespace hf
