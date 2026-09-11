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

#include "hongfan/interpolate.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "hongfan/chronicle.hpp"
#include "hongfan/engine.hpp"
#include "hongfan/expr.hpp"
#include "hongfan/id.hpp"
#include "hongfan/policy.hpp"
#include "hongfan/rng.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/world.hpp"

namespace hf {

std::expected<std::vector<Trajectory>, std::string> complete_between(const RuleSet& rs,
                                                                     const World& start,
                                                                     const std::vector<Expr>& goal,
                                                                     const CompleteOptions& opt) {
  if (opt.depth == 0 || opt.depth > 12) {
    return std::unexpected("depth 需在 1..12");
  }
  // 枚举要求全程无随机（可复现）：随机规则直接拒绝
  for (const auto& ph : rs.schedule) {
    for (size_t const ri : ph) {
      const Rule& r = rs.rules[ri];
      if (r.guard && contains_rand(*r.guard)) {
        return std::unexpected("界间补全禁止随机规则: " + r.id);
      }
      for (const Pattern& p : r.when) {
        for (const Cond& c : p.conds) {
          if (contains_rand(*c.rhs)) {
            return std::unexpected("界间补全禁止随机条件: " + r.id);
          }
        }
      }
      for (const Effect& e : r.effects) {
        const Expr* ex = nullptr;
        if (const auto* u = std::get_if<Update>(&e.op)) {
          ex = u->amount.get();
        } else if (const auto* s = std::get_if<Set>(&e.op)) {
          ex = s->value.get();
        }
        if (ex != nullptr && contains_rand(*ex)) {
          return std::unexpected("界间补全禁止随机效果: " + r.id);
        }
      }
    }
  }

  std::vector<Trajectory> sols;
  const auto goal_holds = [&](const World& w) {
    for (const Expr& g : goal) {
      EvalCtx const ctx{.world = &w, .bindings = nullptr, .rng = nullptr};
      const auto ok = eval_bool(g, ctx);
      if (!ok || !*ok) {
        return false;
      }
    }
    return true;
  };

  std::vector<Step> steps;
  std::function<void(const World&, double, uint32_t)> dfs = [&](const World& w, double cost,
                                                                uint32_t depth) {
    if (sols.size() >= opt.max_solutions) {
      return;
    }
    if (depth == 0) {
      if (!goal_holds(w)) {
        return;
      }
      Trajectory t;
      t.start = start;
      t.steps = steps;
      t.final = w;
      t.final_hash = w.state_hash();
      t.total_cost = cost;
      sols.push_back(std::move(t));
      return;
    }
    for (const auto& ph : rs.schedule) {
      for (size_t const ri : ph) {
        const Rule& r = rs.rules[ri];
        for (Candidate const& c : match_rule(rs, ri, w)) {
          const auto outcome = apply_rule(r, c.bindings, w, nullptr);
          if (!outcome.reject.empty()) {
            continue;
          }
          if (invariant_violation(rs, outcome.next)) {
            continue;
          }
          Step st;
          st.tick = Tick{};
          st.seq = static_cast<uint32_t>(steps.size() + 1);
          st.rule_id = r.id;
          st.kind = r.kind;
          st.bindings = render_bindings(c.bindings, w);
          st.narration = render_narration(r.narration, c.bindings, w);
          st.effects = outcome.records;
          steps.push_back(std::move(st));
          dfs(outcome.next, cost + r.cost, depth - 1);
          steps.pop_back();
          if (sols.size() >= opt.max_solutions) {
            return;
          }
        }
      }
    }
  };
  dfs(start, 0.0, opt.depth);
  return sols;
}

std::optional<Trajectory> select_solution(std::vector<Trajectory>&& sols, const SelectSpec& spec,
                                          Rng& rng) {
  if (sols.empty()) {
    return std::nullopt;
  }
  if (spec.kind == SelectSpec::Kind::Random) {
    return std::move(sols[static_cast<size_t>(rng.rand_int(0, sols.size() - 1))]);
  }
  const auto contains_rule = [&spec](const Trajectory& t) {
    return std::ranges::any_of(t.steps, [&](const Step& s) { return s.rule_id == spec.rule_id; });
  };
  std::optional<size_t> best;
  for (size_t i = 0; i < sols.size(); ++i) {
    if (spec.kind == SelectSpec::Kind::Prefer && !contains_rule(sols[i])) {
      continue;
    }
    if (!best || sols[i].total_cost < sols[*best].total_cost) {
      best = i;  // 平手取先者（规范序）
    }
  }
  if (!best) {
    return std::nullopt;
  }
  return std::move(sols[*best]);
}

std::string solution_brief(const Trajectory& t) {
  std::string out;
  for (size_t i = 0; i < t.steps.size(); ++i) {
    if (i > 0) {
      out += " → ";
    }
    out += t.steps[i].rule_id;
  }
  return out;
}

}  // namespace hf
