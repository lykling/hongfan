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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "hongfan/expr.hpp"
#include "hongfan/fnv.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/yaml.hpp"

#include "ruleset_internal.hpp"

namespace hf {

LoadProblems load_ruleset(const yaml::Node& root) {
  LoadProblems out;
  auto& p = out.problems;
  RuleSet& rs = out.rs;
  if (root.t != yaml::Node::T::Map) {
    p.emplace_back("规则集根节点必须是映射");
    return out;
  }
  if (const auto* name = scalar_field(root, "ruleset")) {
    rs.name = name->scalar;
  } else {
    rs.name = "unnamed";
  }
  if (const auto* time = map_field(root, "time")) {
    const auto* model = scalar_field(*time, "model");
    if (model == nullptr || model->scalar != "linear-tick") {
      p.emplace_back("M1 仅支持 time.model: linear-tick");
    }
  }
  const auto* rules = seq_field(root, "rules");
  if (rules == nullptr) {
    p.emplace_back("缺少 rules 序列");
    return out;
  }

  // 第一遍：definition 聚合类型系统
  for (const auto& item : rules->seq) {
    if (item.t != yaml::Node::T::Map) {
      p.emplace_back("rules 项必须是映射");
      continue;
    }
    const auto* kind = scalar_field(item, "kind");
    if (kind == nullptr || kind->scalar != "definition") {
      continue;
    }
    const auto* id = scalar_field(item, "rule");
    const auto* entity = scalar_field(item, "entity");
    const auto* props = map_field(item, "properties");
    if (id == nullptr || entity == nullptr || props == nullptr) {
      p.emplace_back("definition 规则需要 rule/entity/properties");
      continue;
    }
    std::map<std::string, std::string> schema_props;
    for (const auto& e : props->map) {
      if (e.t != yaml::Node::T::Scalar ||
          (e.scalar != "num" && e.scalar != "str" && e.scalar != "bool")) {
        p.push_back(std::format("规则 {}: 属性 {} 类型须为 num|str|bool", id->scalar, e.key));
        continue;
      }
      schema_props[e.key] = e.scalar;
    }
    if (rs.schema.contains(entity->scalar)) {
      p.push_back(std::format("实体类型重复定义: {}", entity->scalar));
    }
    rs.schema[entity->scalar] = std::move(schema_props);
  }

  // 第二遍：全量解析
  std::vector<Rule> parsed;
  std::set<std::string> ids;
  for (const auto& item : rules->seq) {
    if (item.t != yaml::Node::T::Map) {
      continue;
    }
    const auto* id = scalar_field(item, "rule");
    const auto* kindn = scalar_field(item, "kind");
    if (id == nullptr || kindn == nullptr) {
      p.emplace_back("规则缺少 rule/kind 字段");
      continue;
    }
    if (id->scalar.empty()) {
      p.emplace_back("规则 id 为空");
      continue;
    }
    if (!ids.insert(id->scalar).second) {
      p.push_back(std::format("规则 id 重复: {}", id->scalar));
      continue;
    }
    const auto k = kind_from(kindn->scalar);
    if (!k) {
      p.push_back(std::format("规则 {}: 未知 kind: {}", id->scalar, kindn->scalar));
      continue;
    }
    Rule r;
    r.id = id->scalar;
    r.kind = *k;
    if (const auto* f = scalar_field(item, "layer")) {
      r.layer = f->scalar;
    }
    if (const auto* f = scalar_field(item, "narration")) {
      r.narration = f->scalar;
    }
    if (const auto* f = scalar_field(item, "cost")) {
      const auto c = yaml::coerce(*f);
      if (c.is_num && c.num >= 0) {
        r.cost = c.num;
      } else {
        p.push_back(std::format("规则 {}: cost 需为非负数", r.id));
      }
    }
    if (const auto* f = scalar_field(item, "probability")) {
      const auto c = yaml::coerce(*f);
      if (c.is_num && c.num >= 0 && c.num <= 1) {
        r.probability = c.num;
      } else {
        p.push_back(std::format("规则 {}: probability 需在 [0,1]", r.id));
      }
    }
    if (const auto* f = scalar_field(item, "priority")) {
      const auto c = yaml::coerce(*f);
      if (c.is_num && c.num == std::round(c.num)) {
        r.priority = static_cast<int>(c.num);
      } else {
        p.push_back(std::format("规则 {}: priority 需为整数", r.id));
      }
    }
    if (const auto* f = scalar_field(item, "every")) {
      const auto c = yaml::coerce(*f);
      if (c.is_num && c.num >= 1 && c.num == std::round(c.num)) {
        r.every = static_cast<uint32_t>(c.num);
      } else {
        p.push_back(std::format("规则 {}: every 需为 >=1 的整数", r.id));
      }
    }

    if (*k == Kind::Definition) {
      const auto* entity = scalar_field(item, "entity");
      const auto* props = map_field(item, "properties");
      if (entity == nullptr || props == nullptr) {
        p.push_back(std::format("规则 {}: definition 需要 entity/properties", r.id));
        continue;
      }
      DefinitionSpec d;
      d.entity = entity->scalar;
      for (const auto& e : props->map) {
        d.properties[e.key] = e.scalar;
      }
      r.definition = std::move(d);
      parsed.push_back(std::move(r));
      continue;
    }
    if (*k == Kind::Invariant) {
      const auto* forall = scalar_field(item, "forall");
      const auto* always = scalar_field(item, "always");
      if (forall == nullptr || always == nullptr) {
        p.push_back(std::format("规则 {}: invariant 需要 forall/always", r.id));
        continue;
      }
      auto decl = parse_binding_decl(forall->scalar);
      if (!decl) {
        p.push_back(std::format("规则 {}: {}", r.id, decl.error()));
        continue;
      }
      auto always_expr = parse_expr(always->scalar);
      if (!always_expr) {
        p.push_back(std::format("规则 {}: always {}", r.id, always_expr.error()));
        continue;
      }
      if (contains_rand(*always_expr)) {
        p.push_back(std::format("规则 {}: always 禁止 rand_int", r.id));
        continue;
      }
      InvariantSpec inv;
      inv.var = decl->first;
      inv.type = decl->second;
      inv.always = std::make_unique<Expr>(std::move(*always_expr));
      r.invariant = std::move(inv);
      parsed.push_back(std::move(r));
      continue;
    }

    if (const auto* when = seq_field(item, "when")) {
      for (const auto& w : when->seq) {
        if (w.t != yaml::Node::T::Scalar) {
          p.push_back(std::format("规则 {}: when 项必须是字符串", r.id));
          continue;
        }
        auto pat = parse_pattern(w.scalar);
        if (!pat) {
          p.push_back(std::format("规则 {}: {}", r.id, pat.error()));
          continue;
        }
        r.when.push_back(std::move(*pat));
      }
    }
    if (const auto* guard = scalar_field(item, "guard")) {
      auto g = parse_expr(guard->scalar);
      if (!g) {
        p.push_back(std::format("规则 {}: guard {}", r.id, g.error()));
      } else if (contains_rand(*g)) {
        p.push_back(std::format("规则 {}: guard 禁止 rand_int", r.id));
      } else {
        r.guard = std::make_unique<Expr>(std::move(*g));
      }
    }
    if (const auto* effects = seq_field(item, "effects")) {
      for (const auto& e : effects->seq) {
        if (e.t != yaml::Node::T::Scalar) {
          p.push_back(std::format("规则 {}: effects 项必须是字符串", r.id));
          continue;
        }
        auto fx = parse_effect(e.scalar);
        if (!fx) {
          p.push_back(std::format("规则 {}: {}", r.id, fx.error()));
          continue;
        }
        r.effects.push_back(std::move(*fx));
      }
    }
    parsed.push_back(std::move(r));
  }

  validate_rules(parsed, rs.schema, p);
  if (!p.empty()) {
    return out;
  }

  // 规范排序：definition → invariant → 运行时（phase, id）
  std::vector<Rule> defs;
  std::vector<Rule> invs;
  std::vector<Rule> runtime;
  for (auto& r : parsed) {
    if (r.kind == Kind::Definition) {
      defs.push_back(std::move(r));
    } else if (r.kind == Kind::Invariant) {
      invs.push_back(std::move(r));
    } else {
      runtime.push_back(std::move(r));
    }
  }
  const auto by_id = [](const Rule& a, const Rule& b) {
    return a.id < b.id;
  };
  std::ranges::sort(defs, by_id);
  std::ranges::sort(invs, by_id);
  std::ranges::stable_sort(runtime, [](const Rule& a, const Rule& b) {
    const auto pa = static_cast<uint8_t>(*phase_of(a.kind));
    const auto pb = static_cast<uint8_t>(*phase_of(b.kind));
    if (pa != pb) {
      return pa < pb;
    }
    return a.id < b.id;
  });
  rs.rules.reserve(defs.size() + invs.size() + runtime.size());
  for (auto& r : defs) {
    rs.rules.push_back(std::move(r));
  }
  for (auto& r : invs) {
    rs.rules.push_back(std::move(r));
  }
  for (auto& r : runtime) {
    rs.rules.push_back(std::move(r));
  }
  for (size_t i = 0; i < rs.rules.size(); ++i) {
    const Rule& r = rs.rules[i];
    if (r.kind == Kind::Invariant) {
      rs.invariant_ids.push_back(i);
    }
    if (const auto ph = phase_of(r.kind)) {
      rs.schedule[static_cast<uint8_t>(*ph)].push_back(i);
    }
  }
  rs.hash = fnv1a64(canonical_ruleset(rs));
  return out;
}

}  // namespace hf
