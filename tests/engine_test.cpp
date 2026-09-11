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
#include "hongfan/engine.hpp"

#include <doctest.h>

#include <optional>
#include <string>

#include "hongfan/policy.hpp"
#include "hongfan/rng.hpp"
#include "hongfan/rule.hpp"
#include "hongfan/yaml.hpp"

using namespace hf;

namespace {

struct Mini {
  RuleSet rs;
  World world;
};

std::optional<Mini> load(std::string_view rules_text, std::string_view state_text) {
  auto rd = yaml::parse(rules_text);
  if (!rd)
    return std::nullopt;
  auto sd = yaml::parse(state_text);
  if (!sd)
    return std::nullopt;
  auto loaded = load_ruleset(*rd);
  if (!loaded.problems.empty())
    return std::nullopt;
  auto w = load_world(*sd, loaded.rs.schema);
  if (!w)
    return std::nullopt;
  return Mini{std::move(loaded.rs), std::move(*w)};
}

const std::string kRules = R"YAML(ruleset: mini
time:
  model: linear-tick
rules:
  - rule: def-food
    kind: definition
    entity: Food
    properties: { amount: num }
  - rule: food-nonneg
    kind: invariant
    forall: "f: Food"
    always: "f.amount >= 0"
  - rule: feast
    kind: behavior
    when:
      - "f: Food { amount >= 1 }"
    effects:
      - "update f.amount - 6"
    cost: 2.0
    narration: "大摆宴席。"
  - rule: grow
    kind: behavior
    when:
      - "f: Food { amount <= 4 }"
    effects:
      - "update f.amount + 1"
    narration: "补货 +1。"
)YAML";

const std::string kState = "entities:\n  - { id: pantry, type: Food, props: { amount: 3 } }\n";

}  // namespace

TEST_CASE("加载：规则集与状态校验通过") {
  auto m = load(kRules, kState);
  REQUIRE(m);
  CHECK(m->rs.rules.size() == 4);
  CHECK(m->rs.schedule[static_cast<uint8_t>(Phase::Behavior)].size() == 2);
  CHECK(m->rs.hash != 0);
}

TEST_CASE("不变量回滚：feast 被拒且世界不变") {
  auto m = load(kRules, kState);
  REQUIRE(m);
  auto pol = make_online("first");  // 规范序：feast < grow，First 总选 feast
  Rng rng(1);
  auto res = derive(m->rs, m->world, 3, *pol, rng);
  REQUIRE(res.traj.steps.size() == 3);
  CHECK(res.traj.steps[0].rejected);
  CHECK(res.traj.final_hash == res.init_state_hash);
  CHECK(res.traj.final.entities[0].props.at("amount") == Value{3.0});
}

TEST_CASE("min-cost 策略推动补货") {
  auto m = load(kRules, kState);
  REQUIRE(m);
  Rng rng(7);
  auto pol = make_online("min-cost");
  auto res = derive(m->rs, m->world, 3, *pol, rng);
  CHECK_FALSE(res.traj.steps[0].rejected);
  // 3→4→5；第 3 刻 grow 不再匹配（5>4），feast 被选且违反不变量被拒
  CHECK(res.traj.final.entities[0].props.at("amount") == Value{5.0});
  CHECK(res.traj.steps[2].rejected);
  CHECK(res.traj.total_cost == doctest::Approx(0.0));
}

TEST_CASE("确定性：同 Run 四元组产出完全一致") {
  auto m1 = load(kRules, kState);
  auto m2 = load(kRules, kState);
  REQUIRE(m1);
  REQUIRE(m2);
  auto p1 = make_online("random");
  auto p2 = make_online("random");
  Rng r1(42), r2(42);
  auto a = derive(m1->rs, m1->world, 8, *p1, r1);
  auto b = derive(m2->rs, m2->world, 8, *p2, r2);
  CHECK(a.traj.final_hash == b.traj.final_hash);
  REQUIRE(a.traj.steps.size() == b.traj.steps.size());
  for (size_t i = 0; i < a.traj.steps.size(); ++i) {
    CHECK(a.traj.steps[i].rule_id == b.traj.steps[i].rule_id);
    CHECK(a.traj.steps[i].narration == b.traj.steps[i].narration);
    CHECK(a.traj.steps[i].rejected == b.traj.steps[i].rejected);
  }
}

TEST_CASE("校验：变量封闭性拒绝未知变量") {
  const std::string bad = R"YAML(ruleset: bad
rules:
  - rule: def-food
    kind: definition
    entity: Food
    properties: { amount: num }
  - rule: eat
    kind: behavior
    when:
      - "f: Food { amount >= 1 }"
    effects:
      - "update g.amount - 1"
)YAML";
  auto rd = yaml::parse(bad);
  REQUIRE(rd);
  auto loaded = load_ruleset(*rd);
  REQUIRE_FALSE(loaded.problems.empty());
}
