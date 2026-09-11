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

#include "hongfan/engine.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "hongfan/chronicle.hpp"
#include "hongfan/expr.hpp"
#include "hongfan/id.hpp"
#include "hongfan/policy.hpp"
#include "hongfan/rng.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/value.hpp"
#include "hongfan/world.hpp"

namespace hf {
namespace {

[[nodiscard]] bool conds_hold(const std::vector<Cond>& conds, const PropMap& props,
                              const Bindings& b, const World& w) {
  for (const Cond& c : conds) {
    const auto it = props.find(c.prop);
    if (it == props.end()) {
      return false;  // 属性缺失 = 不匹配
    }
    EvalCtx const ctx{.world = &w, .bindings = &b, .rng = nullptr};
    const auto rhs = eval(*c.rhs, ctx);
    if (!rhs) {
      return false;  // 匹配期求值错误按不匹配处理（确定性）
    }
    const auto ok = compare(c.op, it->second, *rhs);
    if (!ok || !*ok) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::optional<std::string> slot_name(const World& w, uint32_t slot) {
  return w.name_of(EntityId{slot});
}

}  // namespace

std::vector<Candidate> match_rule(const RuleSet& rs, size_t rule_idx, const World& w) {
  const Rule& r = rs.rules[rule_idx];
  std::vector<Candidate> out;
  Bindings b;
  std::function<void(size_t)> dfs = [&](size_t pi) {
    if (pi == r.when.size()) {
      if (r.guard) {
        EvalCtx const ctx{.world = &w, .bindings = &b, .rng = nullptr};
        const auto ok = eval_bool(*r.guard, ctx);
        if (!ok || !*ok) {
          return;
        }
      }
      out.push_back(
          Candidate{.rule_idx = rule_idx, .bindings = b, .cost = r.cost, .priority = r.priority});
      return;
    }
    const Pattern& p = r.when[pi];
    if (!p.is_relation) {
      for (size_t slot = 0; slot < w.entities.size(); ++slot) {
        const Entity& e = w.entities[slot];
        if (e.type != p.type) {
          continue;
        }
        if (!conds_hold(p.conds, e.props, b, w)) {
          continue;
        }
        b[p.var] = Binding{.kind = Binding::Kind::Entity, .ref = static_cast<uint32_t>(slot)};
        dfs(pi + 1);
        b.erase(p.var);
      }
      return;
    }
    for (size_t ri = 0; ri < w.relations.size(); ++ri) {
      const Relation& rel = w.relations[ri];
      if (p.rel_type && rel.type != *p.rel_type) {
        continue;
      }
      EvalCtx ctx{.world = &w, .bindings = &b, .rng = nullptr};
      const auto endpoint_ok = [&](const std::optional<Expr>& ex, EntityId which) {
        if (!ex) {
          return true;
        }
        const auto v = eval(*ex, ctx);
        if (!v) {
          return false;
        }
        const auto* id = std::get_if<EntityId>(&v->v);
        return id != nullptr && *id == which;
      };
      if (!endpoint_ok(p.src, rel.src) || !endpoint_ok(p.dst, rel.dst)) {
        continue;
      }
      if (!conds_hold(p.conds, rel.props, b, w)) {
        continue;
      }
      b[p.var] = Binding{.kind = Binding::Kind::Relation, .ref = static_cast<uint32_t>(ri)};
      dfs(pi + 1);
      b.erase(p.var);
    }
  };
  dfs(0);
  return out;
}

ApplyOutcome apply_rule(const Rule& r, const Bindings& b, const World& w, Rng* rng) {
  ApplyOutcome out;
  out.next = w;  // 克隆（事务：失败即弃）
  EvalCtx const ctx{.world = &w,
                    .bindings = &b,
                    .rng = rng};  // 效果求值基于应用前快照（M1 语义：同规则效果互不可见）
  const auto entity_slot = [&](const std::string& var) -> std::expected<uint32_t, std::string> {
    const auto it = b.find(var);
    if (it == b.end()) {
      return std::unexpected(std::format("效果引用未绑定变量: {}", var));
    }
    if (it->second.kind != Binding::Kind::Entity) {
      return std::unexpected(std::format("关系变量不可用于此效果: {}", var));
    }
    return it->second.ref;
  };
  const auto name_or_slot = [&](uint32_t slot) {
    return slot_name(w, slot).value_or("#" + std::to_string(slot));
  };

  for (const Effect& e : r.effects) {
    if (const auto* u = std::get_if<Update>(&e.op)) {
      const auto slot = entity_slot(u->var);
      if (!slot) {
        out.reject = slot.error();
        return out;
      }
      const auto amount = eval(*u->amount, ctx);
      if (!amount) {
        out.reject = std::format("效果求值失败: {}", amount.error());
        return out;
      }
      const auto n = as_num(*amount);
      if (!n) {
        out.reject = std::format("update 幅度需为数值: {}.{}", u->var, u->prop);
        return out;
      }
      auto& ent = out.next.entities.at(*slot);
      const auto it = ent.props.find(u->prop);
      if (it == ent.props.end()) {
        out.reject = std::format("属性不存在: {}.{}", u->var, u->prop);
        return out;
      }
      const auto cur = as_num(it->second);
      if (!cur) {
        out.reject = std::format("update 需要数值属性: {}.{}", u->var, u->prop);
        return out;
      }
      const double before = *cur;
      const double after = u->is_add ? before + *n : before - *n;
      it->second = Value{after};
      out.records.push_back(
          EffectRecord{std::format("{}.{}: {} → {}", name_or_slot(*slot), u->prop,
                                   display(Value{before}), display(Value{after}))});
    } else if (const auto* s = std::get_if<Set>(&e.op)) {
      const auto slot = entity_slot(s->var);
      if (!slot) {
        out.reject = slot.error();
        return out;
      }
      const auto value = eval(*s->value, ctx);
      if (!value) {
        out.reject = std::format("效果求值失败: {}", value.error());
        return out;
      }
      auto& ent = out.next.entities.at(*slot);
      const auto [it, inserted] = ent.props.try_emplace(s->prop, *value);
      const std::string before = inserted ? "(无)" : display(it->second);
      it->second = *value;
      out.records.push_back(EffectRecord{
          std::format("{}.{}: {} → {}", name_or_slot(*slot), s->prop, before, display(*value))});
    } else {
      const auto& ra = std::get<RelAssert>(e.op);
      const auto src = eval(ra.src, ctx);
      if (!src) {
        out.reject = std::format("效果求值失败: {}", src.error());
        return out;
      }
      const auto dst = eval(ra.dst, ctx);
      if (!dst) {
        out.reject = std::format("效果求值失败: {}", dst.error());
        return out;
      }
      const auto* src_ent = std::get_if<EntityId>(&src->v);
      const auto* dst_ent = std::get_if<EntityId>(&dst->v);
      if (src_ent == nullptr || dst_ent == nullptr) {
        out.reject = "relation 端点需为实体引用";
        return out;
      }
      const bool exists = std::ranges::any_of(w.relations, [&](const Relation& rel) {
        return rel.type == ra.type && rel.src == *src_ent && rel.dst == *dst_ent;
      });
      if (!exists) {
        out.next.relations.push_back(
            Relation{.type = ra.type, .src = *src_ent, .dst = *dst_ent, .props = {}});
      }
      out.records.push_back(EffectRecord{
          std::format("R:{}: {} -> {}", ra.type, name_or_slot(static_cast<uint32_t>(*src_ent)),
                      name_or_slot(static_cast<uint32_t>(*dst_ent)))});
    }
  }
  return out;
}

std::optional<std::string> invariant_violation(const RuleSet& rs, const World& w) {
  for (size_t const ii : rs.invariant_ids) {
    const Rule& r = rs.rules[ii];
    if (!r.invariant) {
      continue;
    }
    const auto& inv = *r.invariant;
    for (size_t slot = 0; slot < w.entities.size(); ++slot) {
      const Entity& e = w.entities[slot];
      if (e.type != inv.type) {
        continue;
      }
      const Bindings b{
          {inv.var, Binding{.kind = Binding::Kind::Entity, .ref = static_cast<uint32_t>(slot)}}};
      EvalCtx const ctx{.world = &w, .bindings = &b, .rng = nullptr};
      const auto name =
          w.name_of(EntityId{static_cast<uint32_t>(slot)}).value_or("#" + std::to_string(slot));
      const auto ok = eval_bool(*inv.always, ctx);
      if (!ok) {
        return std::format("违反不变量 {}（{}）: {}", r.id, name, ok.error());
      }
      if (!*ok) {
        return std::format("违反不变量 {}（{}）", r.id, name);
      }
    }
  }
  return std::nullopt;
}

DeriveResult derive(const RuleSet& rs, World world, uint32_t ticks, OnlinePolicy& pol, Rng& rng) {
  DeriveResult res;
  res.ruleset_hash = rs.hash;
  res.init_state_hash = world.state_hash();
  Trajectory& traj = res.traj;
  traj.start = world;
  uint32_t seq = 0;
  for (uint32_t t = 1; t <= ticks; ++t) {
    world.now = Tick{t};
    for (const auto& ph : rs.schedule) {
      // 候选：规范序（schedule 规则序 × DFS 绑定序）
      std::vector<Candidate> cands;
      for (size_t const ri : ph) {
        const Rule& r = rs.rules[ri];
        if (r.kind == Kind::Evolution && t % r.every != 0) {
          continue;
        }
        for (auto& c : match_rule(rs, ri, world)) {
          cands.push_back(std::move(c));
        }
      }
      // 冲突消解规范序：priority 降序（稳定排序保持规范序）
      std::ranges::stable_sort(
          cands, [](const Candidate& a, const Candidate& b) { return a.priority > b.priority; });
      // probability 门（A.3：此序消耗 RNG；p=1 不消耗，p=0 直接淘汰）
      std::vector<Candidate> gated;
      gated.reserve(cands.size());
      for (auto& c : cands) {
        const double p = rs.rules[c.rule_idx].probability;
        if (p >= 1.0 || (p > 0.0 && rng.bernoulli(p))) {
          gated.push_back(std::move(c));
        }
      }
      if (gated.empty()) {
        continue;
      }
      const auto pick = pol.pick(gated, rng);
      if (!pick) {
        continue;
      }
      const Candidate& c = gated[*pick];
      const Rule& r = rs.rules[c.rule_idx];

      Step st;
      st.tick = Tick{t};
      st.seq = ++seq;
      st.rule_id = r.id;
      st.kind = r.kind;
      st.bindings = render_bindings(c.bindings, world);
      st.narration = render_narration(r.narration, c.bindings, world);

      auto outcome = apply_rule(r, c.bindings, world, &rng);
      if (!outcome.reject.empty()) {
        st.rejected = true;
        st.reject_reason = outcome.reject;
      } else if (auto vio = invariant_violation(rs, outcome.next)) {
        st.rejected = true;
        st.reject_reason = *vio;
      } else {
        world = std::move(outcome.next);
        st.effects = std::move(outcome.records);
        traj.total_cost += r.cost;
      }
      traj.steps.push_back(std::move(st));
    }
  }
  traj.final = world;
  traj.final_hash = world.state_hash();
  return res;
}

}  // namespace hf
