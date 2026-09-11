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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "hongfan/interpolate.hpp"

#include <doctest.h>

#include <optional>
#include <string>
#include <vector>

#include "hongfan/rng.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/yaml.hpp"

using namespace hf;

namespace {

struct TravelerWorld {
  RuleSet rs;
  World world;
  std::vector<Expr> goal;
};

std::optional<TravelerWorld> load() {
  const std::string rules = R"YAML(ruleset: traveler
time:
  model: linear-tick
rules:
  - rule: def-traveler
    kind: definition
    entity: Traveler
    properties: { stamina: num, skill: num }
  - rule: traveler-nonneg
    kind: invariant
    forall: "t: Traveler"
    always: "t.stamina >= 0 && t.skill >= 0"
  - rule: trek
    kind: behavior
    when:
      - "p: Traveler { stamina >= 2 }"
    effects:
      - "update p.stamina - 1"
    cost: 1.0
    narration: "旅人冒风跋涉。"
  - rule: train
    kind: behavior
    when:
      - "p: Traveler { stamina >= 2 }"
    effects:
      - "update p.skill + 1"
      - "update p.stamina - 1"
    cost: 2.0
    narration: "旅人执剑习武。"
  - rule: rest
    kind: behavior
    when:
      - "p: Traveler"
    effects:
      - "update p.stamina + 2"
    cost: 0.5
    narration: "旅人枕石而歇。"
)YAML";
  const std::string state =
      "entities:\n  - { id: traveler, type: Traveler, props: { stamina: 10, skill: 0 } }\n";
  const std::string goal_text =
      "goal:\n  - \"traveler.skill == 3\"\n  - \"traveler.stamina >= 4\"\n";
  auto rd = yaml::parse(rules);
  if (!rd)
    return std::nullopt;
  auto sd = yaml::parse(state);
  if (!sd)
    return std::nullopt;
  auto gd = yaml::parse(goal_text);
  if (!gd)
    return std::nullopt;
  auto loaded = load_ruleset(*rd);
  if (!loaded.problems.empty())
    return std::nullopt;
  auto w = load_world(*sd, loaded.rs.schema);
  if (!w)
    return std::nullopt;
  auto g = load_goal(*gd, *w);
  if (!g)
    return std::nullopt;
  return TravelerWorld{std::move(loaded.rs), std::move(*w), std::move(*g)};
}

}  // namespace

TEST_CASE("旅人稽疑：解数与最省力解") {
  auto tw = load();
  REQUIRE(tw);
  auto sols = complete_between(tw->rs, tw->world, tw->goal, CompleteOptions{6, 1000});
  REQUIRE(sols);
  CHECK(sols->size() == 160);
  Rng rng(5);
  auto mc = select_solution(std::move(*sols), SelectSpec{SelectSpec::Kind::MinCost, ""}, rng);
  REQUIRE(mc);
  CHECK(mc->total_cost == doctest::Approx(7.5));  // 3×train(2.0) + 3×rest(0.5)
  const auto brief = solution_brief(*mc);
  CHECK(brief.find("trek") == std::string::npos);
}

TEST_CASE("旅人稽疑：prefer 解包含指定规则") {
  auto tw = load();
  REQUIRE(tw);
  auto sols = complete_between(tw->rs, tw->world, tw->goal, CompleteOptions{6, 1000});
  REQUIRE(sols);
  Rng rng(5);
  auto pref = select_solution(std::move(*sols), SelectSpec{SelectSpec::Kind::Prefer, "trek"}, rng);
  REQUIRE(pref);
  CHECK(pref->total_cost == doctest::Approx(8.0));  // 3×train + 1×trek + 2×rest
  CHECK(solution_brief(*pref).find("trek") != std::string::npos);
}

TEST_CASE("random 选取可复现") {
  auto tw = load();
  REQUIRE(tw);
  auto s1 = complete_between(tw->rs, tw->world, tw->goal, CompleteOptions{6, 1000});
  auto s2 = complete_between(tw->rs, tw->world, tw->goal, CompleteOptions{6, 1000});
  REQUIRE(s1);
  REQUIRE(s2);
  Rng r1(11), r2(11);
  auto a = select_solution(std::move(*s1), SelectSpec{SelectSpec::Kind::Random, ""}, r1);
  auto b = select_solution(std::move(*s2), SelectSpec{SelectSpec::Kind::Random, ""}, r2);
  REQUIRE(a);
  REQUIRE(b);
  CHECK(a->final_hash == b->final_hash);
  CHECK(solution_brief(*a) == solution_brief(*b));
}
